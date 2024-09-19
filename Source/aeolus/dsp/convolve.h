// ----------------------------------------------------------------------------
//
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#pragma once

#include "aeolus/globals.h"
#include "aeolus/simd.h"
#include "aeolus/memory.h"
#include "aeolus/worker.h"
#include "aeolus/dsp/fft.h"

#include <cassert>
#include <atomic>
#include <vector>

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

struct ConvPartBase
{
    float* irBuffer = nullptr;
    float* inputBuffer = nullptr;
    size_t inputSize = 0;
    size_t readIndex = 0;

    void init(float* ir, float* input, size_t size)
    {
        assert(input != nullptr);
        assert(ir != nullptr);

        irBuffer    = ir;
        inputBuffer = input;
        inputSize   = size;
        readIndex   = (size - 1) % inputSize;
    }
};

template<size_t L>
struct ConvPart : public ConvPartBase
{
    constexpr static size_t Lendth = L;
    constexpr static size_t Delay = 0;

    static_assert(math::isPowerOfTwo(L), "Convolution part length must be a power of two");
};

template <size_t PrevLength, class ...Parts>
struct Conv;

template <size_t PrevLength, class Part, class ...Parts>
struct Conv<PrevLength, Part, Parts...>
{
    using Tail = Conv<PrevLength + Part::Length, Parts...>;

    constexpr static size_t PrevStageLength = PrevLength;
    constexpr static size_t Length = Part::Length + Tail::Length;

    static_assert(
        PrevLength >= Part::Delay,
        "Part must not be londer than total length of all  previous stages"
        );

    Part part;
    Tail tail;

    void reset()
    {
        part.reset();
        tail.reset();
    }

    void init(float* ir, float* input, size_t size)
    {
        assert(ir != nullptr);
        assert(input != nullptr);

        part.init(ir, input, size);
        tail.init(&ir[Part::Length], input, size);
    }

    inline float tick()
    {
        return part.tick() + tail.tick();
    }
};

template <size_t PrevLength, class Part>
struct Conv<PrevLength, Part>
{
    constexpr static size_t PrevStageLength = PrevLength;
    constexpr static size_t Length = Part::Length;

    static_assert(
        PrevLength >= Part::Delay,
        "Part must not be londer than total length of all  previous stages"
        );

    Part part;

    void reset()
    {
        part.reset();
    }

    void init(float* ir, float* input, size_t size)
    {
        assert(input != nullptr);
        assert(ir != nullptr);

        part.init(ir, input, size);
    }

    inline float tick()
    {
        return part.tick();
    }
};

//----------------------------------------------------------

template <size_t L>
struct FIR : public ConvPart<L>
{
    constexpr static size_t Delay = 0;
    constexpr static size_t Length = L;

    void reset()
    {
        ConvPartBase::readIndex = 0;
    }

    inline float tick()
    {
        float y = 0.0f;

        if (ConvPartBase::readIndex + Length < ConvPartBase::inputSize) {
            y = simd::mul_reduce_unaligned(ConvPartBase::irBuffer, &ConvPartBase::inputBuffer[ConvPartBase::readIndex], Length);
        } else {
            for (size_t i = 0; i < Length; ++i)
                y += ConvPartBase::irBuffer[i] * ConvPartBase::inputBuffer[(ConvPartBase::readIndex + i) % ConvPartBase::inputSize];
        }

        ConvPartBase::readIndex = (ConvPartBase::readIndex - 1) % ConvPartBase::inputSize;

        return y;
    }
};

template <size_t L>
struct FFT : public ConvPart<L>
{
    constexpr static size_t Delay = L;
    constexpr static size_t Length = L;
    constexpr static size_t Length2 = Length * 2;
    constexpr static size_t Length4 = Length * 4;

    float* complexInputBuffer;
    float* complexIrBuffer;

    bool irReady = false;
    float* tailBuffer;
    size_t tailIndex;

    using FftImpl = GFFT<Length2, float>;

    FFT()
    {
        complexInputBuffer = (float*) AlignedMemory<32>::alloc(Length4 * sizeof(float));
        complexIrBuffer    = (float*) AlignedMemory<32>::alloc(Length4 * sizeof(float));
        tailBuffer         = (float*) AlignedMemory<32>::alloc(Length * sizeof(float));

        reset();
    }

    ~FFT()
    {
        AlignedMemory<32>::free(tailBuffer);
        AlignedMemory<32>::free(complexIrBuffer);
        AlignedMemory<32>::free(complexInputBuffer);
    }

    void reset()
    {
        ::memset(complexInputBuffer, 0, sizeof(float) * Length4);
        ::memset(complexIrBuffer,    0, sizeof(float) * Length4);
        ::memset(tailBuffer,         0, sizeof(float) * Length);

        tailIndex = 0;
        irReady = false;
    }

    inline float tick()
    {
        float y = complexInputBuffer[tailIndex * 2];

        // Feed from input buffer
        const size_t idx = (ConvPartBase::readIndex - tailIndex) % ConvPartBase::inputSize;

        complexInputBuffer[tailIndex * 2] = ConvPartBase::inputBuffer[idx];
        complexInputBuffer[tailIndex * 2 + 1] = 0.0f;

        tailIndex = (tailIndex + 1) % Length;

        if (tailIndex == 0) {
            convolve();
            ConvPartBase::readIndex = (ConvPartBase::readIndex - Length) % ConvPartBase::inputSize;
        }

        return y;
    }

    void convolve()
    {
        if (! irReady) {
            size_t j = 0;

            for (size_t i = 0; i < Length; ++i) {
                complexIrBuffer[j++] = ConvPartBase::irBuffer[i];
                complexIrBuffer[j++] = 0.0f;
            }

            FftImpl::fft_real_padded(complexIrBuffer);

            irReady = true;
        }

        ::memset(&complexInputBuffer[Length2], 0, sizeof(float) * Length2);

        FftImpl::fft_real_padded(complexInputBuffer);

        simd::complex_mul_conj(complexInputBuffer, complexInputBuffer, complexIrBuffer, Length4);

        FftImpl::fft(complexInputBuffer);

        // Add tail buffer from previous convolution
        size_t tail = 0;

        for (size_t i = 0; i < Length2; i += 2) {
            constexpr float norm = 1.0f / Length2;

            // Put output samples together without interleaving
            complexInputBuffer[i] = norm * (complexInputBuffer[i] + tailBuffer[tail]);
            tailBuffer[tail] = complexInputBuffer[Length2 + i];
            ++tail;
        }
    }
};

//----------------------------------------------------------

template <class ...Parts>
struct CascadeConvolver : public Conv<0, Parts...>
{
    using Parent = Conv<0, Parts...>;
    constexpr static size_t Lenght = Parent::Length;

    float* irBuffer;
    size_t irIndex;

    float* inputBuffer;
    size_t inputIndex;
    size_t inputSize;

    CascadeConvolver()
        : irBuffer{nullptr}
        , irIndex{0}
        , inputBuffer{nullptr}
        , inputIndex{0}
        , inputSize{0}
    {
    }

    void init(float* ir, float* input, size_t size)
    {
        irBuffer    = ir;
        irIndex     = 0;
        inputBuffer = input;
        inputIndex  = 0;
        inputSize   = size;

        Parent::init(ir, input, size);
    }

    inline void reset()
    {
        irIndex = 0;
        inputIndex = inputSize - 1;

        Parent::reset();
    }

    inline float tick(float x)
    {
        inputIndex = (inputIndex - 1) % inputSize;
        inputBuffer[inputIndex] = x;

        return Parent::tick();
    }
};

//----------------------------------------------------------

template <size_t L>
class EquallyPartitionedConvolver final
{
public:

    static_assert(math::isPowerOfTwo(L), "Block length must be a power of two");

    constexpr static size_t Length = L;
    constexpr static size_t Length2 = 2 * Length;
    constexpr static size_t Length4 = 4 * Length;

    using FftImpl = GFFT<Length2>;

    EquallyPartitionedConvolver (size_t n = 0)
        : inputIndex{0}
        , inputSpectrumBuffer{nullptr}
        , inputSpectrumBufferSize{Length4 * n}
        , inputSpectrumIndex{0}
        , irSpectrumBuffer{nullptr}
        , irSpectrumBufferSize{Length4 * n}
        , irInputIndex{0}
        , irInputBlockIndex{0}
        , blocks{n}
    {

        inputSpectrumBuffer = (float*) AlignedMemory<32>::alloc(inputSpectrumBufferSize * sizeof(float));
        irSpectrumBuffer = (float*) AlignedMemory<32>::alloc(irSpectrumBufferSize * sizeof(float));

        reset();
    }

    ~EquallyPartitionedConvolver()
    {
        setWorker (nullptr);
        AlignedMemory<32>::free(irSpectrumBuffer);
        AlignedMemory<32>::free(inputSpectrumBuffer);
    }

    void resize(size_t n)
    {
        if (n * Length4 != irSpectrumBufferSize) {
            AlignedMemory<32>::free(irSpectrumBuffer);
            irSpectrumBufferSize = n * Length4;
            irSpectrumBuffer = (float*) AlignedMemory<32>::alloc(irSpectrumBufferSize * sizeof(float));
        }

        if (n * Length4 != inputSpectrumBufferSize) {
            AlignedMemory<32>::free(inputSpectrumBuffer);
            inputSpectrumBufferSize = Length4 * n;
            inputSpectrumBuffer = (float*) AlignedMemory<32>::alloc(inputSpectrumBufferSize * sizeof(float));
        }

        blocks.resize(n);

        reset();
    }

    void setWorker(Worker* w)
    {
        for (auto& block : blocks)
            block.worker = w;
    }

    void reset()
    {
        inputIndex = 0;

        ::memset(inputSpectrumBuffer, 0, sizeof(float) * inputSpectrumBufferSize);
        inputSpectrumIndex = 0;

        ::memset(irSpectrumBuffer, 0, sizeof(float) * irSpectrumBufferSize);
        irInputIndex = 0;

        irInputBlockIndex = 0;

        size_t preconvolveIndex = 0;
        const size_t preconvolveIndexStep = blocks.empty() ? 0 : Length / blocks.size();

        for (size_t i = 0; i < blocks.size(); ++i) {
            blocks[i].inputSpectrumPtr = &inputSpectrumBuffer[i * Length4];
            blocks[i].irSpectrumPtr = &irSpectrumBuffer[i * Length4];
            blocks[i].preconvolveIndex = preconvolveIndex;
            preconvolveIndex += preconvolveIndexStep;
            blocks[i].reset();
        }
    }

    void feedIr(float x)
    {
        assert(irInputIndex < irSpectrumBufferSize);
        assert(irInputBlockIndex < blocks.size());

        irSpectrumBuffer[irInputIndex] = x;
        irInputIndex += 2;

        if (irInputIndex % Length2 == 0) {
            // IR input chunk is ready - compute ir Chunk spectrum
            blocks[irInputBlockIndex].irFft();

            ++irInputBlockIndex;
            irInputIndex += Length2;
        }
    }

    float tick(float x)
    {
        float y = 0.0f;

        for (auto& block : blocks)
            y += block.tick();

        inputSpectrumBuffer[inputSpectrumIndex + 2 * inputIndex] = x;
        inputSpectrumBuffer[inputSpectrumIndex + 2 * inputIndex + 1] = 0.0f;
        ++inputIndex;

        if (inputIndex >= Length) {
            // Input is ready - compute input spectrum
            inputIndex = 0;
            inputFft();
        }

        return y;
    }

    void inputFft()
    {
        // Clear padding
        ::memset(&inputSpectrumBuffer[inputSpectrumIndex + Length2], 0, sizeof (float) * Length2);

        FftImpl::fft_real_padded(&inputSpectrumBuffer[inputSpectrumIndex]);

        // Perform blocks convolution
        convolveBlocks();

        // Rotate blocks input spectra
        float* lastBlockInputSpectrumPtr = blocks[blocks.size() - 1].inputSpectrumPtr;

        for (size_t i = blocks.size() - 1; i > 0; --i)
            blocks[i].inputSpectrumPtr = blocks[i - 1].inputSpectrumPtr;

        blocks[0].inputSpectrumPtr = lastBlockInputSpectrumPtr;

        // Move input spectrum index to the next chunk
        inputSpectrumIndex = inputSpectrumIndex == 0 ? inputSpectrumBufferSize - Length4
            : inputSpectrumIndex - Length4;

        // First block receives fresh input signal and cannot be dephased.
        blocks[0].dephase = false;
    }

    void convolveBlocks()
    {
        for (auto& block : blocks)
            block.convolve();
    }

private:
    //------------------------------------------------------

    struct Block final : public Worker::Job
    {
        Worker* worker = nullptr;
        float* inputSpectrumPtr = nullptr;
        float* irSpectrumPtr = nullptr;

        float* convolutionBuffer;
        float* outputBuffer;
        float* tailBuffer;

        size_t tailIndex = 0;
        bool irReady = false;

        bool dephase = false;
        size_t preconvolveIndex = 0;
        std::atomic<bool> preconvolved = false;

        Block()
        {
            convolutionBuffer = (float*) AlignedMemory<32>::alloc(Length4 * sizeof(float));
            outputBuffer      = (float*) AlignedMemory<32>::alloc(Length * sizeof(float));
            tailBuffer        = (float*) AlignedMemory<32>::alloc(Length * sizeof(float));
        }

        Block(const Block& other)
            : worker{other.worker}
            , inputSpectrumPtr{other.inputSpectrumPtr}
            , irSpectrumPtr{other.irSpectrumPtr}
            , preconvolved{false}
        {
            convolutionBuffer = (float*) AlignedMemory<32>::alloc(Length4 * sizeof(float));
            outputBuffer      = (float*) AlignedMemory<32>::alloc(Length * sizeof(float));
            tailBuffer        = (float*) AlignedMemory<32>::alloc(Length * sizeof(float));
        }

        ~Block()
        {
            AlignedMemory<32>::free(tailBuffer);
            AlignedMemory<32>::free(outputBuffer);
            AlignedMemory<32>::free(convolutionBuffer);
        }

        void reset()
        {
            ::memset(convolutionBuffer, 0, sizeof(float) * Length4);
            ::memset(outputBuffer,      0, sizeof(float) * Length);
            ::memset(tailBuffer,        0, sizeof(float) * Length);
            tailIndex = 0;
            irReady = false;

            dephase = false;
            preconvolved =false;
        }

        void irFft()
        {
            assert(irSpectrumPtr != nullptr);
            FftImpl::fft_real_padded(irSpectrumPtr);
            irReady = true;
        }

        void convolve()
        {
            // IR is not ready yet - nothig to do
            if (! irReady)
                return;

            if (dephase) {
                postconvolve();
                return;
            }

            preconvolve();
            postconvolve();

            dephase = true;
        }

        void preconvolve()
        {
            assert(inputSpectrumPtr != nullptr);
            assert(irSpectrumPtr != nullptr);

            if (preconvolved)
                return; // Already preconvolved or overload?

            simd::complex_mul_conj(convolutionBuffer, inputSpectrumPtr, irSpectrumPtr, Length4);

            FftImpl::fft(convolutionBuffer);

            preconvolved = true;
        }

        void postconvolve()
        {
            if (! preconvolved)
                return; // Not ready :(

            // Add tail buffer from previous convolution
            size_t tail = 0;

            for (size_t i = 0; i < Length2; i += 2) {
                constexpr float norm = 1.0f / Length2;

                // Put output samples together without interleaving
                outputBuffer[tail] = norm * (convolutionBuffer[i] + tailBuffer[tail]);
                tailBuffer[tail] = convolutionBuffer[Length2 + i];
                ++tail;
            }

            preconvolved = false;
        }

        float tick()
        {
            float y = outputBuffer[tailIndex];
            tailIndex = (tailIndex + 1) % Length;

            if (dephase && tailIndex == preconvolveIndex) {
                if (worker != nullptr)
                    worker->addJob (this);
                else
                    preconvolve();
            }

            return y;
        }

        // Job
        void run() override
        {
            preconvolve();
        }
    };

    //------------------------------------------------------

    size_t inputIndex;

    float* inputSpectrumBuffer;
    size_t inputSpectrumBufferSize;
    size_t inputSpectrumIndex;

    float* irSpectrumBuffer;
    size_t irSpectrumBufferSize;
    size_t irInputIndex;
    size_t irInputBlockIndex;

    std::vector<Block> blocks;
};

} // namespace dsp

AEOLUS_NAMESPACE_END
