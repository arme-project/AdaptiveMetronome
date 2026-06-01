# Building

AdaptiveMetronome is a JUCE project stored in
`AdaptiveMetronome.jucer`. The repository intentionally ignores generated build
folders, so you should regenerate the IDE projects locally.

## Requirements

- JUCE and Projucer.
- Xcode for macOS Audio Unit builds.
- Visual Studio 2022 for Windows VST3 builds.
- A DAW or plug-in host for testing the generated plug-in.

## Generate Project Files

1. Clone the repository.
2. Open `AdaptiveMetronome.jucer` in Projucer.
3. Check the module paths for your local JUCE installation.
4. Save the project in Projucer. This regenerates the local `Builds` and
   `JuceLibraryCode` folders.

Those generated folders are ignored by Git and should normally stay out of pull
requests.

## macOS Build

1. In Projucer, enable or inspect the **Xcode (macOS)** exporter.
2. Save the project.
3. Open the generated Xcode project in `Builds/MacOSX`.
4. Build the **Release** target.
5. Install the generated Audio Unit component into:

   ```text
   Macintosh HD/Library/Audio/Plug-Ins/Components
   ```

6. Restart or rescan your DAW.

## Windows Build

1. In Projucer, enable or inspect the **Visual Studio 2022** exporter.
2. Save the project.
3. Open the generated solution in `Builds/VisualStudio2022`.
4. Build the **Release** configuration.
5. Install the generated VST3 into:

   ```text
   C:\Program Files\Common Files\VST3
   ```

6. Restart or rescan your DAW.

## Project Format Notes

The current project is configured as an audio plug-in with these formats:

- Audio Unit MIDI effect on macOS.
- VST3 MIDI effect on Windows.

The project characteristics mark it as a MIDI effect that accepts MIDI input and
produces MIDI output. The plug-in itself does not produce audio.

## Suggested Smoke Test

After building:

1. Open the plug-in in a DAW.
2. Put a software instrument after it.
3. Load the Haydn example MIDI file or a small two-track test MIDI file.
4. Start DAW playback.
5. Confirm that four guide tones play, the score advances, tapping triggers the
   user player's notes, and a CSV log appears in the Documents folder.
