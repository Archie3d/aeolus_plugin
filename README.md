![aeolus](Resources/icons/icon256.png)
# Aeolus
Pipe organ emulator using additive synthesis as a VST plugin.

Aeolus was originally developed by Fons Adriaensen and presented in 2004. The original implementation is Linux only and can be found [here](https://kokkinizita.linuxaudio.org/linuxaudio/aeolus/) (or across Linux distribution packages). At present it looks like Aeolus development has been abandoned.

This project leverages the wavetables systhesis part of the original Aeolus, and frames it into a standard VST3 plugin using [JUCE](https://github.com/juce-framework/JUCE) framework, so that it can be run in Windows/macOS VST hosts. Additiona improvements to the sound include the pipes chiff noise on attack, and the new pipe models.

The original binary format for the pipe models and the organ configuration has been translated (partially) to JSON.

## Demo
:warning: These are a bit outdated.
- [Pachelbel - Ciacona in Fm](demo/Pachelbel_Ciacona_in_Fm.mp3)
- [Pachelbel - Chorale prelude](demo/Pachelbel_Chorale_prelude.mp3) 

## Implementation notes

From the original implementation this project takes only `addsynth` and `rankwave` modules. These modules describe the 64-harmonics additive synth and organ pipes wavetables generation. Plus this repo includes the original `ae0` files that contain the harmonics tables for various pipes in binary format (these are embedded into the plugin's resources). All the new pipe models are in JSON.

The rest and the most of the code (including voicing, spatialisation, reverb, etc.) is all new, and it is not based on the original Aeolus, so the sound this plugin produces is different.

The convolution reverb uses IRs from the [Open AIR](https://www.openair.hosted.york.ac.uk/) project database.
