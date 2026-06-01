# User Guide

AdaptiveMetronome is a MIDI-effect plug-in. It does not generate audio by
itself. Place it before a software instrument so the MIDI output can be heard.

## How The Ensemble Is Created

When you load a MIDI file, AdaptiveMetronome reads each track and creates one
player for every track that contains note-on events. Tracks without notes are
ignored.

The **No. User Players** selector determines how many of those players are
human-operated. Human players are assigned to the first note-bearing tracks in
the MIDI file. Change this value before pressing **Load MIDI**. If you change it
after loading a file, reload the MIDI file so the ensemble is recreated.

## Running A Performance

1. Add AdaptiveMetronome as a MIDI effect before a software instrument.
2. Select the number of user players.
3. Press **Load MIDI** and choose a MIDI score.
4. Set player and coupling parameters.
5. Start playback in the host DAW.
6. Listen for four guide tones.
7. Tap MIDI notes for each human-operated player. If no tap arrives in the
   current timing window, the plug-in automatically advances the user player's
   next note so the score can continue.
8. Press **Reset** to rewind the loaded score and restart the guide-tone count.

## Controls

| Control | Meaning |
| --- | --- |
| No. User Players | Number of human-operated players. User players are assigned from the first note-bearing MIDI tracks. |
| Load MIDI | Loads a MIDI score and recreates the ensemble. |
| Reset | Rewinds the current score and restarts the guide-tone count. |
| Player | Player number created from the MIDI file. |
| MIDI Channel | MIDI channel used for that player's outgoing notes. |
| Volume | Scales the outgoing note velocity for that player. |
| Delay | Adds a per-player output delay, in milliseconds. |
| Motor Noise STD | Standard deviation of the player's motor-noise component, in milliseconds. |
| Time Keeper Noise STD | Standard deviation of the player's timekeeper-noise component, in milliseconds. |
| Alpha | Pairwise phase-correction coefficient from one player to another. |
| Beta | Pairwise correction coefficient that adjusts the timekeeper mean. |

The alpha and beta controls are arranged as a player-by-player matrix. Rows
represent the player being updated. Columns represent the player whose timing is
being compared.

## MIDI File Preparation

For the clearest results:

- Put each ensemble part in a separate MIDI track.
- Use the intended player order in the MIDI track order.
- Keep note durations meaningful, because the plug-in preserves each note's
  MIDI-file duration when it schedules note-off events.
- Avoid extra note-bearing tracks unless they should become ensemble players.
- Test the file in a DAW before using it in a study.

AdaptiveMetronome currently discards the original MIDI onset times and plays
notes sequentially according to the timing model and host tempo.

For the full timing equations, parameter defaults, units, and citations, see
[Timing Model](MODEL.md).

## Log Files

Each loaded or reset performance starts a logging thread and writes a CSV file to
the user's Documents folder. File names use this pattern:

```text
Log_HH-MM-SS_DDMonYYYY.csv
```

The log contains one row per ensemble onset update. The columns include:

- Onset time for each player, in seconds.
- Played onset interval for each player, in seconds.
- Whether a user player's latest onset came from actual user input.
- Per-player delay, in seconds.
- Motor-noise and timekeeper-noise values.
- Pairwise asynchronies, in seconds.
- Alpha and beta values active at that onset.
- Noise standard deviations.
- Output volume.

Keep these CSV files with your experiment data and record the plug-in version,
host DAW, tempo, MIDI file, and parameter settings used for each run.

## Notes On Current Behaviour

- The plug-in uses the DAW playhead tempo. If no tempo is available from the
  host, it falls back to 60 BPM.
- The plug-in sends all-notes-off, all-sound-off, and all-controllers-off events
  when playback stops or the ensemble resets.
- Parameter state is not currently saved by the plug-in host. For reproducible
  work, write down the parameter settings or capture screenshots before closing
  a project.
