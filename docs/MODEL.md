# Timing Model

This document describes the timing model implemented in the current
AdaptiveMetronome source. It is intended as a practical reference for researchers
who need to report the model, reproduce parameter settings, or compare log files
with the plug-in behaviour.

## Scope

AdaptiveMetronome is a MIDI-effect plug-in. It reads a MIDI file, creates one
player for each note-bearing track, and schedules the notes in each track as an
ordered sequence. The original MIDI onset times are not used as performance
times. Note durations are preserved and used when scheduling note-off events.

The host DAW supplies the transport state and tempo. The nominal beat period is:

```text
B = 60 * Fs / BPM
```

where:

- `B` is the nominal beat interval in samples.
- `Fs` is the audio sample rate in samples per second.
- `BPM` is the host tempo in beats per minute.

If the host does not provide a tempo, the processor falls back to 60 BPM.

For the broader immersive rehearsal context around ARME, see the University of
Birmingham [Virtual Reality Lab](https://virtualrealitylab.netlify.app/) and the
[ARME virtual orchestra rehearsal news item](https://www.birmingham.ac.uk/news/2024/dr-massimiliano-di-luca-leads-development-of-software-for-virtual-orchestra-rehearsals).

## Player Timing

For player `i` at ensemble update `n`, let:

- `t_i,n` be the latest onset time for player `i`, in samples.
- `I_i,n+1` be the interval from the latest onset to the next onset, in samples.
- `a_i,j,n = t_i,n - t_j,n` be the asynchrony between players `i` and `j`, in
  samples.
- `alpha_i,j` be the phase-correction coefficient from player `j` to player `i`.
- `beta_i,j` be the coefficient that updates player `i`'s timekeeper mean from
  the asynchrony with player `j`.

All virtual players update once every player has produced a note for the current
ensemble event. Non-user players update first; user-operated players update
afterward.

## Virtual Player Equation

For a virtual player, the phase-correction and beta sums are:

```text
A_i,n = sum_j alpha_i,j * a_i,j,n
K_i,n = sum_j beta_i,j  * a_i,j,n
```

The timekeeper mean is stored in seconds and updated as:

```text
mu_i,n+1 = mu_i,n - K_i,n / Fs
```

Motor and timekeeper noise are sampled in seconds:

```text
m_i,n  ~ Normal(0, sigma_M_i)
tk_i,n ~ Normal(mu_i,n+1, sigma_T_i)
```

The implementation combines these as:

```text
h_i,n = tk_i,n + m_i,n - m_i,n-1
```

The next onset interval for virtual player `i` is then:

```text
I_i,n+1 = B - A_i,n + Fs * h_i,n
```

This follows the plug-in implementation in `Player::recalculateOnsetInterval()`.

## User-Operated Players

User-operated players advance when MIDI note input arrives during the accepted
timing window. In the current implementation, a note-on event is accepted when no
note has yet been played for the current ensemble event and the score counter is
greater than half the current onset interval. After this initial threshold has
passed, the first note-on for each user-player event triggers the next score
note.

If no input arrives before the current interval expires, the user player's next
note is advanced automatically so the ensemble can continue. The log marks
whether the latest user-player onset came from actual user input.

When virtual players are present, a user player's fallback interval is set from
the mean timing of the non-user players:

```text
mean_t = mean latest onset time of non-user players
mean_I = mean next interval of non-user players
I_user,n+1 = mean_t - t_user,n + 1.5 * mean_I
```

When all players are user-operated, there are no non-user players to average.
Version 1.0.3 uses the user's most recently played interval as the next fallback
interval. If no previous interval exists yet, it uses the nominal host beat
interval `B`.

## Delay Handling

The per-player delay slider is specified in milliseconds and converted to
samples:

```text
D_i = Fs * delay_ms_i / 1000
```

For virtual players, the output note is delayed by `D_i`. The stored onset time
used in logs and asynchrony calculations subtracts this delay, so delay and onset
timing can be analysed separately.

## Defaults And Units

| Parameter | Range | Default | Unit | Notes |
| --- | --- | --- | --- | --- |
| Number of user players | 0-4 | 1 | players | Selected before loading MIDI. |
| MIDI channel | 1-16 | Cycled by player | channel | Player 1 starts on channel 1. |
| Volume | 0-1 | 1.0 | velocity scale | Scales outgoing MIDI velocity. |
| Delay | 0-200 | 0.0 | ms | Output delay for a player. |
| Motor Noise STD | 0-10 | 0.1 | ms | Converted to seconds before sampling. |
| Time Keeper Noise STD | 0-50 | 1.0 | ms | Converted to seconds before sampling. |
| Alpha | 0-1 | 0.25 for column P1, otherwise 0.0 | dimensionless | Pairwise phase correction. |
| Beta | 0-1 | 0.25 for column P1, otherwise 0.0 | dimensionless | Updates timekeeper mean. |
| Host tempo fallback | n/a | 60 | BPM | Used only if the DAW does not report tempo. |
| Intro tones | n/a | 4 | tones | MIDI note 69 on channel 1, velocity 100. |

The alpha and beta defaults reflect the current source code: every player row is
initialized with a value of 0.25 in the first player column and 0.0 elsewhere.
Users can change these values in the plug-in UI after loading a MIDI file.

## Log Interpretation

Log files are written as CSV files in the user's Documents folder. Timing values
derived from sample counters are written in seconds. The main columns are:

- Onset time for each player.
- Played onset interval for each player.
- Whether a user-player note was triggered by user input.
- Per-player delay.
- Motor-noise and timekeeper-noise sample values.
- Pairwise asynchronies.
- Alpha and beta values at the onset.
- Noise standard deviations.
- Output volume.

For reproducible experiments, keep the CSV log with the MIDI file, DAW tempo,
sample rate, plug-in version, alpha/beta matrix, noise settings, and number of
user players.

## Current Limitations

- Random noise is seeded from `std::random_device`, so runs are not yet
  deterministic even with identical visible settings.
- Plug-in parameter state is not currently saved and restored by the host.
- Original MIDI onset times are discarded; only note order and duration are used.
- The network/server hooks for adaptive alpha updates are placeholders in the
  current source.

## References

- Enderby, S., Stables, R., Hockman, J., Tomczak, M., Wing, A., Elliott, M., and
  Di Luca, M. (2023). Adaptive Metronome: A MIDI Plug-In for Modelling
  Cooperative Timing in Music Ensembles. In Rhythm Production and Perception
  Workshop, Nottingham, United Kingdom. See
  [the included abstract PDF](references/rppw19_abstract_Sean_Enderby.pdf).
- Wing, A. M., and Kristofferson, A. B. (1973). Response delays and the timing of
  discrete motor responses. Perception & Psychophysics, 14(1), 5-12.
  <https://doi.org/10.3758/BF03198607>
- Wing, A. M., and Kristofferson, A. B. (1973). The timing of interresponse
  intervals. Perception & Psychophysics, 13(3), 455-460.
  <https://doi.org/10.3758/BF03205802>
- Wing, A. M., Endo, S., Bradbury, A., and Vorberg, D. (2014). Optimal feedback
  correction in string quartet synchronization. Journal of The Royal Society
  Interface, 11(93), 20131125. <https://doi.org/10.1098/rsif.2013.1125>
