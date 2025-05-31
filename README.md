![build](https://github.com/Archie3d/aeolus_plugin/actions/workflows/build.yml/badge.svg)

# ![aeolus](Resources/icons/icon64.png) Aeolus
Pipe organ emulator using additive synthesis as a **VST, AU, or CLAP plugin** (or a stand-alone executable).

Aeolus was originally developed by Fons Adriaensen and presented in 2004. The original implementation is Linux only and can be found [here](https://kokkinizita.linuxaudio.org/linuxaudio/aeolus/) (or across Linux distribution packages). At present it looks like Aeolus development has been mostly abandoned (but [Organnery](https://organnery.com/) picked up the original Aeolus project to make it run on a Raspberry Pi).

This project leverages the wavetable systhesis part of the original Aeolus, improves on it, and delivers it as a virtual instrument plugin using [JUCE](https://github.com/juce-framework/JUCE) framework, so that it can be run in Windows/macOS/Linux VST3/AU/CLAP hosts.

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
Sequencer steps are controlled via the program change messages sent on the control MIDI channel (program `0 `corresponds to the first step of the sequencer, `1` - to the second and so on).
> In a DAW you may use `Program` parameter to control the sequencer steps.
> Another way to control the sequencer is via key switches (MIDI notes). The default configuration uses keys `22` and `23` to control the sequencer. These can be changed via the organ configuration file (multiple keys can be assigned for forward and backward actions).

### CCs:
- `CC 1` modulation wheel enables/disables tremulant
- `CC 7` controls the global volume (on the control MIDI channel) and the volume of each division which has `swell` config flag enabled (on the swell MIDI channel)
- `CC 91` controls the reverb output level
- `CC 98` Stops control
- `CC 123` All notes off (on control MIDI channel or inidividual divisions).

> :point_right: The stops control follows the original Aeolus convention. The control mode is set by the message `01mm0ggg`, where `mm` is the control mode, and `ggg` is the control group (division number, counter from the top starting from `0`).
> Control modes are:
> - `00` Disable the division (all stops are off)
> - `01` Set stop off
> - `10` Set stop on
> - `11` Toggle
>
> Once the control message is received the value of the format `000bbbbb` will execute the selected control mode action on specified stop buton `bbbbb`, counted from `0`.

## Custom organ configuration
Custom organ configuration will be loaded by the plugin if found at `Documents/Aeolus/organ_config.json` location.
> :point_right: The `Documents` folder's exact location depends on the operating system and the user profile.

To create the `organ_config.json` start with [default one embedded into the plugin](Resources/configs/default_organ.json) by copying it to `Documents/Aeolus` folder and renaming to `organ_config.json`.

It is also possible to use custom pipe configs in `.ae0` or `.json` format. These have to be placed to the `Documents/Aeolus` folder for then can be referenced from the `organ_config.json` (use pipe file name without the extension in the`"pipe"`attribute of the `organ_config.json`).

## MTS support
When enabled, use a MIDI Tuning Standard (MTS) master (like [MTS-ESP Mini](https://oddsound.com/mtsespmini.php)) to change the tuning.
> :point_right: Tuning change will affect newly played notes.
> All plugin instances are affected by the tuning change.

## Multibus output
When compiled with the `WITH_MULTIBUS_OUTPUT` CMake option enabled, the generated plugin will ouput to the `8` separate _monofonic_ buses. Each bus corresponds to the pipes groups placement in space according to the internal horizontal arrangement of the pipes.

In multibus configuration there is no reverb applied, and there is no spatialization performed.

> :point_right: The multibus mode is indended for the object based mixing, where you could place individual pipe groups in space yourself and apply a reverb of your preference.

Pipes are arranged starting from the lowest key from the sides (buses 0 and 7) to the center in the middle of the range (buses 3, 4), and then going back from the centre towards the sides. For the pedal pipes, they go from the outside towards the centre only.

Corresponding pipe position jumps between left and right following the keys (C will be on the left, C# on the right, D of the left, D# on the right and so on).

> :point_right: This very same pipes spatial arrangement is used in the stereo version of the plugin to perform spatialized rendering followed by a stereo convolutional reverb.

## CLAP
CLAP plugn format currently uses the [JUCE Unofficial CLAP Plugin Support](https://github.com/free-audio/clap-juce-extensions).

When compiling, make sure to pull all the submodules recursively:
```shell
git submodule update --init --recursive
```
