# Aeolus
Pipe organ emulator using additive synthesis as a VST plugin.

Aeolus was originally developed Fons Adriaensen and presented in 2004. The original implementation is the Linux only application which can be found [here](https://kokkinizita.linuxaudio.org/linuxaudio/aeolus/) (or across Linux distribution packages). At present it looks like Aeolus development has been abandoned.

This project is my attempt to extract the systhesis part of the original Aeolus and frame it into a standard VST plugin using [JUCE](https://github.com/juce-framework/JUCE) framework, so that it can be run in Windows/macOS DAWs.

## Demo
- [Pachelbel - Ciacona in Fm](demo/Pachelbel_Ciacona_in_Fm.mp3)
- [Pachelbel - Chorale prelude](demo/Pachelbel_Chorale_prelude.mp3) 

## Implementation notes

From the original implementation I only ported `addsynth` and `rankwave` modules. These modules describe the 64-harmonics additive synth and organ pipes wavetables generation. Plus this repo includes the `ae0` files that contain the harmonics tables for various pipes (these are embedded into the plugin's resources).

But the most of the code (including voicing, spatialisation, reverb, etc.) is all new, and it is not based on the original Aeolus.

Convolution reverb uses IRs from the [Open AIR](https://www.openair.hosted.york.ac.uk/) project database.

## Current limitations
- No links between manuals (but you can assign manuals to the same MIDI channel).
- There is no swell control yet (but there is a gain control per division).
