# Troubleshooting

## The Plug-In Does Not Appear In My DAW

- Confirm that you installed the correct format for your operating system.
- Restart the DAW after copying the plug-in.
- Rescan plug-ins in the DAW preferences.
- On macOS, check **System Settings > Privacy & Security** if the component was
  blocked by Gatekeeper.
- Confirm that the plug-in is being searched as a MIDI effect, not only as an
  audio instrument.

## The Plug-In Opens But I Hear No Sound

- Add a software instrument after AdaptiveMetronome on the same track.
- Confirm that the software instrument is receiving MIDI.
- Check the player MIDI channels and the instrument's channel settings.
- Increase the player volume controls.
- Start playback from the DAW transport. The plug-in processes the score while
  the host playhead is running.

## Load MIDI Does Nothing

- Use a `.mid` file.
- Make sure the file contains note-on events.
- Try a simple test MIDI file with one or two short note tracks.
- Check whether the file opens and plays in the DAW without AdaptiveMetronome.

## Tapping Does Not Trigger Notes

- Make sure the track is receiving MIDI input from your keyboard or controller.
- In Logic Pro, open the musical typing keyboard and verify that pressing the
  intended key produces instrument sound.
- Start DAW playback and wait for the four guide tones before tapping.
- The plug-in advances the user player's next note on the first note-on event in
  the current timing window. Very early taps may be ignored.

## The Performance Does Not Restart From The Beginning

Press **Reset** in the plug-in window. This rewinds the loaded score and restarts
the four guide tones.

## I Cannot Find The CSV Log

Logs are written to the user's Documents folder with names like:

```text
Log_14-32-08_01Jun2026.csv
```

If no file appears:

- Load a MIDI file before starting playback.
- Check that the Documents folder is writable.
- Stop and restart playback after pressing **Reset**.

## Build Fails Because JUCE Modules Cannot Be Found

Open `AdaptiveMetronome.jucer` in Projucer and update the module paths to your
local JUCE installation. Save the project again before building in Xcode or
Visual Studio.

## Parameter Settings Are Gone After Reopening The Project

The current plug-in code does not save parameter state through the DAW host.
Record settings manually, export screenshots, or keep the generated CSV logs with
your experiment notes.
