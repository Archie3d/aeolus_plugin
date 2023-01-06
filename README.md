![build](https://github.com/Archie3d/aeolus_plugin/actions/workflows/build.yml/badge.svg)

# ![aeolus](Resources/icons/icon64.png) Aeolus
Pipe organ emulator using additive synthesis as a **VST or AU plugin** (or a stand-alone executable).

Aeolus was originally developed by Fons Adriaensen and presented in 2004. The original implementation is Linux only and can be found [here](https://kokkinizita.linuxaudio.org/linuxaudio/aeolus/) (or across Linux distribution packages). At present it looks like Aeolus development has been mostly abandoned (but [Organnery](https://organnery.com/) picked up the original Aeolus project to make it run on a Raspberry Pi).

This project leverages the wavetable systhesis part of the original Aeolus, improves on it, and delivers it as a standard VST plugin using [JUCE](https://github.com/juce-framework/JUCE) framework, so that it can be run in Windows/macOS VST3/AU hosts.

This implementation contains additional improvements to the sound generation including
- pipes chiff noise on attack;
- new pipe models;
- convolutional reverb.

The original binary format for the pipe models and the organ configuration has been translated (partially) to JSON.

## Implementation notes

This project takes only `addsynth` and `rankwave` modules from the original implementation (the source's been modified though). These modules describe the 64-harmonics additive synth and organ pipes wavetables generation. Plus this repo includes the original `ae0` files that contain the harmonics tables for various pipes in binary format (these are embedded into the plugin's resources). All the new pipe models are encoded in equivalent JSON format.

The rest and the most of the code (including voicing, spatialisation, reverb, etc.) is all new, and it is not based on the original Aeolus, so the sound this plugin produces is different.

The convolution reverb uses impulse responses from the [Open AIR](https://www.openair.hosted.york.ac.uk/) project database.

## MIDI Control
Sequencer steps are controlled via the program change messages sent on the control MIDI channel (program 0 corresponds to the first step of the sequencer, 1 - to the second and so on).
> In a DAW use `Program` parameter to control the sequencer steps.

Control MIDI channel:
- `CC 7` controls the global volume;
- `CC 91` controls the reverb output level;

## Custom organ configuration
:warning: _Since version 0.1.13_

Custom organ configuration will be loaded by the plugin if found at `Documents/Aeolus/organ_config.json` location.
> :point_right: The `Documents` folder's exact location depends on the operating system.

To create the `organ_config.json` start with [default one embedded into the plugin](Resources/configs/default_organ.json) by copying it to `Documents/Aeolus` folder and renaming to `organ_config.json`.

## Multibus output
:warning: _Since version 0.1.13_

When compiled with the `WITH_MULTIBUS_OUTPUT` CMake option enabled, the generated plugin will ouput to the `8` separate _monofonic_ buses. Each bus corresponds to the pipes groups placement in space according to the internal horizontal arrangement of the pipes.

In multibus configuration there is no reverb applied, and there is no spatialization performed.

> :point_right: The multibus mode is indended for the object based mixing, where you could place individual pipe groups in space yourself and apply a reverb of your preference.

Pipes are arranged starting from the lowest key from the sides (buses 0 and 7) to the center in the middle of the range (buses 3, 4), and then going back from the centre towards the sides. For the pedal pipes, they go from the outside towards the centre only.

Corresponding pipe position jumps between left and right following the keys (C will be on the left, C# on the right, D of the left, D# on the right and so on).

> :point_right: This very same pipes spatial arrangement is used in the stereo version of the plugin to perform spatialized rendering followed by a stereo convolutional reverb.
