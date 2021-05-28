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

#include "aeolus/audioparam.h"
#include "aeolus/dsp/convolve.h"
#include "aeolus/dsp/convolver.h"

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

constexpr size_t ConvBlockSize = 4096;

using ConvHead = dsp::CascadeConvolver<
                      dsp::FIR<32>,
                      dsp::FFT<32>,
                      dsp::FFT<64>,
                      dsp::FFT<128>,
                      dsp::FFT<256>,
                      dsp::FFT<512>,
                      dsp::FFT<1024>,
                      dsp::FFT<2048>
                  >;

static_assert (ConvHead::Length == ConvBlockSize, "Header block size is wrong");

using ConvBlock = dsp::FFT<ConvBlockSize>;

struct Convolver::Impl
{
    enum State
    {
        Idle,
        Init,
        FeedHeadIR,
        ProcessWithIRStream,
        Process
    };

    AudioParameterPool params;
    Worker worker;
    size_t length;

    State state;

    ConvHead headL;
    ConvHead headR;
    bool zeroDelay;

    dsp::EquallyPartitionedConvolver<ConvBlockSize> convL;
    dsp::EquallyPartitionedConvolver<ConvBlockSize> convR;

    std::vector<ConvBlock> blocksL;
    std::vector<ConvBlock> blocksR;

    // For zero-delay convolution
    AudioBuffer<float> input;
    AudioBuffer<float> ir;
    size_t irSamplesRead;

    size_t inputSize;
    size_t framesProcessed;

    Impl ()
        : params (Convolver::NUM_PARAMS),
          length (0),
          state (Idle),
          headL(),
          headR(),
          zeroDelay (true),
          convL(),
          convR(),
          input (2, ConvHead::Lenght),
          ir (2, ConvHead::Lenght),
          irSamplesRead (0),
          inputSize (0),
          framesProcessed (0)
    {
        params[DRY].setName ("dry");
        params[DRY].setValue (DefaultDry, true);

        params[WET].setName ("wet");
        params[WET].setValue (DefaultWet, true);

        params[GAIN].setName ("gain");
        params[GAIN].setValue (DefaultGain, true);

        worker.start();
    }

    ~Impl()
    {
        worker.stop();
    }

    void init ()
    {
        size_t numBlocks = length < ConvBlockSize ? 1 : (length - 1) / ConvBlockSize + 1;
        inputSize = numBlocks * ConvBlockSize;

        convL.resize (numBlocks);
        convR.resize (numBlocks);

        headL.init (ir.getWritePointer (0), input.getWritePointer (0), ConvBlockSize);
        headR.init (ir.getWritePointer (1), input.getWritePointer (1), ConvBlockSize);

        updateRealtime (false);

        reset();
    }

    void updateRealtime (bool isNonRealtime)
    {
        // Run convolution on a side thread for real-time processing.
        if (isNonRealtime)
        {
            convL.setWorker (nullptr);
            convR.setWorker (nullptr);
        }
        else
        {
            convL.setWorker (&worker);
            convR.setWorker (&worker);
        }
    }

    void reset()
    {
        input.clear();
        ir.clear();
        irSamplesRead = 0;

        headL.reset();
        headR.reset();

        convL.reset();
        convR.reset();

        framesProcessed = 0;
    }

    void setDryWet (float dry, float wet, bool force)
    {
        params[DRY].setValue (dry, force);
        params[WET].setValue (wet, force);
    }

    float isAudible() const
    {
        return params[WET].target() > 0.0f
            || params[WET].value() > 0.0f;
    }

    void setIR (const AudioBuffer<float>& buffer)
    {
        ir = buffer;
        
        // Reset the convolver to the initial state
        irSamplesRead = 0;
        framesProcessed = 0;
        input.clear();

        headL.init (ir.getWritePointer (0), input.getWritePointer (0), ConvBlockSize);
        headR.init (ir.getWritePointer (1), input.getWritePointer (1), ConvBlockSize);

        headL.reset();
        headR.reset();

        convL.reset();
        convR.reset();

        int i = zeroDelay ? ConvBlockSize : 0;
        const float* irL = ir.getReadPointer (0);
        const float* irR = ir.getReadPointer (1);

        while (i < jmin ((int)inputSize, ir.getNumSamples()))
        {
            convL.feedIr (irL[i]);
            convR.feedIr (irR[i]);
            ++i;
        }

        while (i < inputSize)
        {
            convL.feedIr (0.0f);
            convR.feedIr (0.0f);
            ++i;
        }

        state = Process;
    }

    void prepareToPlay ()
    {
        state = Init;
        init();
    }

    void process (const float *inL, const float *inR, float *outL, float *outR, size_t numFrames)
    {
        if (state == Init)
            state = zeroDelay ? FeedHeadIR : ProcessWithIRStream;

        if (state == FeedHeadIR)
        {
            // ir buffer is ready at this point
            if (ir.getNumSamples() <= ConvBlockSize)
            {
                irSamplesRead = ir.getNumSamples();
                state = Process;
            }
            else
            {
                irSamplesRead = ConvBlockSize;
                state = ProcessWithIRStream;
            }
        }

        if (state == ProcessWithIRStream)
        {
            for (size_t i = 0; i < numFrames; ++i)
            {
                float l = convL.tick (inL[i]);
                float r = convR.tick (inR[i]);

                if (zeroDelay)
                {
                    l += headL.tick (inL[i]);
                    r += headR.tick (inR[i]);
                }

                float dry = params[Convolver::DRY].nextValue();
                float wet = params[Convolver::WET].nextValue();

                outL[i] = l * wet + inL[i] * dry;
                outR[i] = r * wet + inR[i] * dry;
            }

            framesProcessed += numFrames;

            if (framesProcessed >= inputSize || irSamplesRead >= ir.getNumSamples())
            {
                // The entire IR has been read, switch to procesing without IR streaming
                state = Process;
            }

            return;
        }

        if (state == Process)
        {
            for (size_t i = 0; i < numFrames; ++i)
            {
                float l = convL.tick (inL[i]);
                float r = convR.tick (inR[i]);

                if (zeroDelay)
                {
                    l += headL.tick (inL[i]);
                    r += headR.tick (inR[i]);
                }

                float dry = params[Convolver::DRY].nextValue();
                float wet = params[Convolver::WET].nextValue();

                outL[i] = l * wet + inL[i] * dry;
                outR[i] = r * wet + inR[i] * dry;
            }

            return;
        }
    }
};

//----------------------------------------------------------

Convolver::Convolver()
    : d (std::make_unique<Impl>())
{
}

Convolver::~Convolver() = default;

void Convolver::setIR (const AudioBuffer<float>& ir)
{
    d->setIR (ir);
}

void Convolver::setDryWet (float dry, float wet, bool force)
{
    d->setDryWet (dry, wet, force);
}

bool Convolver::isAudible() const
{
    return d->isAudible();
}

void Convolver::prepareToPlay (float /* sampleRate */, size_t /* nFrames */)
{
    d->prepareToPlay();
}

void Convolver::process (const float *inL, const float *inR, float *outL, float *outR, size_t numFrames)
{
    d->process (inL, inR, outL, outR, numFrames);
}

void Convolver::setNonRealtime (bool nonRealtime)
{
    d->updateRealtime (nonRealtime);
}

int Convolver::length() const noexcept
{
    return (int) d->length;
}

void Convolver::setLength (int len) noexcept
{
    d->length = len;
}

bool Convolver::zeroDelay() const noexcept
{
    return d->zeroDelay;
}

void Convolver::setZeroDelay (bool v) noexcept
{
    d->zeroDelay = v;
}

} // namespace dsp

AEOLUS_NAMESPACE_END
