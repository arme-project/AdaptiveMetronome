# Installation And DAW Setup

This guide explains how to install a packaged AdaptiveMetronome release and run
the example Haydn MIDI score in Logic Pro on macOS or REAPER on Windows.

## Before You Start

You need:

- A downloaded AdaptiveMetronome release from
  <https://github.com/arme-project/AdaptiveMetronome/releases>.
- A DAW that can host MIDI-effect plug-ins.
- A software instrument on the same track as AdaptiveMetronome, because the
  plug-in outputs MIDI rather than audio.
- A MIDI score. The example Haydn score is available from
  <https://github.com/maxdiluca/haydn_midi/archive/refs/tags/download.zip>.

The MIDI file should contain one note-bearing track per ensemble player. The
plug-in ignores tracks that do not contain note-on events.

## Logic Pro On macOS

1. Download the latest macOS release archive.
2. Extract `AdaptiveMetronome.component`.
3. Move the component to:

   ```text
   Macintosh HD/Library/Audio/Plug-Ins/Components
   ```

4. Download and extract the example Haydn MIDI file, or prepare your own
   multi-track MIDI score.
5. Open Logic Pro.
6. Create an empty project with a software instrument track.
7. Choose an instrument sound, for example **Library > Orchestral > Strings >
   String Ensemble**.
8. In the track's MIDI FX slot, choose **Audio Units > ARME > AdaptiveMetronome**.
9. If macOS blocks the plug-in, open **System Settings > Privacy & Security** and
   allow `AdaptiveMetronome.component`. Then close and reopen Logic Pro.
10. In the AdaptiveMetronome window, choose the number of user players.
11. Press **Load MIDI** and select the MIDI file.
12. Start playback from the Logic transport.
13. After the four guide tones, use the Logic musical typing keyboard to trigger
    the human player's notes. The original demo uses the `J` key.

Video walkthroughs:

- [Logic Pro installation video](https://www.youtube.com/watch?v=2CeIm4auh44)
- [Logic Pro operation video](https://www.youtube.com/watch?v=HKUYVPlAp8E)

## REAPER On Windows

1. Download the latest Windows release archive.
2. Extract the VST3 plug-in.
3. Move the VST3 file or folder to:

   ```text
   C:\Program Files\Common Files\VST3
   ```

4. Download and extract the example Haydn MIDI file, or prepare your own
   multi-track MIDI score.
5. Open REAPER.
6. Create a new project and add a track.
7. Open the FX browser and add **VST3: AdaptiveMetronome (ARME)** to the track.
8. Add a software instrument after AdaptiveMetronome in the same FX chain.
9. In the AdaptiveMetronome window, choose the number of user players.
10. Press **Load MIDI** and select the MIDI file.
11. Start playback from the REAPER transport.
12. Send MIDI note input to the track to trigger the human player's notes.

If the plug-in does not appear, rescan VST3 plug-ins from REAPER's preferences
after copying the file into the VST3 folder.

## Installing Updates

Stop your DAW before replacing an installed plug-in file. After replacing the
file, restart the DAW and rescan plug-ins if needed.

For reproducible research, record the AdaptiveMetronome release version or Git
commit in your notes and publications.
