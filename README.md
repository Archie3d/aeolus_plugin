# Aeolus
Pipe organ emulator using additive synthesis as a VST plugin.

Aeolus was originally developed Fons Adriaensen and presented in 2004. The original implementation is the Linux only application which can be found [here](https://kokkinizita.linuxaudio.org/linuxaudio/aeolus/) (or across Linux distribution packages). At present it looks like Aeolus development has been abandoned.

This project is my attempt to extract the systhesis part of the original Aeolus and frame it into a standard VST plugin using [JUCE](https://github.com/juce-framework/JUCE) framework, so that it can be run in Windows/macOS DAWs.

## Demo
- [Pachelbel - Ciacona in Fm](demo/Pachelbel_Ciacona_in_Fm.mp3)
- [Pachelbel - Chorale prelude](demo/Pachelbel_Chorale_prelude.mp3) 

## Implementation notes

From the original implementation I only ported `addsynth` and `rankwave` modules. These modules describe the 64-harmonics additive synth and organ pipes wavetables generation. Plus this repo inclused the `ae0` files that contain the harmonics tables for various pipes (these are embedded into the plugin during compilation).

## Current limitations
- The ouput is dry, there is no reverb in the plugin. Though I've added simple stereo spatialisation of the pipes. You can use whatever reverb you want from your DAW.
- No links between manuals.
- There is no swell control yet.
