# AdaptiveMetronome

AdaptiveMetronome is a JUCE MIDI-effect plug-in for studying adaptive timing in
ensemble performance. It loads a multi-track MIDI score, turns each note-bearing
track into a player, and lets those players perform together using a linear
phase-correction timing model. Players can be virtual, human-operated, or a mix
of both.

The plug-in was developed as part of the EPSRC-funded
[Augmented Reality Music Ensemble (ARME)](https://arme-project.co.uk/) project,
with project context on the
[ARME Adaptive Metronome demo page](https://arme-project.co.uk/demos/adaptive-metronome).
ARME's immersive rehearsal work is also connected with the University of
Birmingham [Virtual Reality Lab](https://virtualrealitylab.netlify.app/).
It is useful for demonstrations, experiments, and teaching around musical
synchronisation, human-machine ensemble interaction, and timing variability in
chamber music.

## What It Does

- Runs as an Audio Unit MIDI effect on macOS and as a VST3 MIDI effect on
  Windows.
- Loads a MIDI score and creates one player per MIDI track that contains notes.
- Supports 0-4 human-operated players, with the remaining players simulated.
- Lets the user set per-player MIDI channel, volume, delay, motor-noise standard
  deviation, timekeeper-noise standard deviation, and pairwise alpha/beta timing
  correction values.
- Starts each performance with four guide tones, then plays the loaded score in
  time with the host DAW.
- Records onset timing, intervals, delays, model parameters, and user-input flags
  to CSV files in the user's Documents folder.

## Quick Start

1. Download a packaged build from the repository's
   [GitHub Releases](https://github.com/arme-project/AdaptiveMetronome/releases).
2. Install the plug-in for your DAW:
   - macOS Audio Unit: `Macintosh HD/Library/Audio/Plug-Ins/Components`
   - Windows VST3: `C:\Program Files\Common Files\VST3`
3. Download the example Haydn MIDI file from
   [maxdiluca/haydn_midi](https://github.com/maxdiluca/haydn_midi/archive/refs/tags/download.zip).
4. Add AdaptiveMetronome as a MIDI effect before a software instrument.
5. Set the number of human players, press **Load MIDI**, and choose the score.
6. Start DAW playback. After the four guide tones, tap along to trigger the
   human player's notes.

Detailed DAW walkthroughs are in [Installation And DAW Setup](docs/INSTALLATION.md).

## Typical Research Workflow

1. Prepare a MIDI file with one note-bearing track per ensemble player.
2. Load the file into AdaptiveMetronome.
3. Choose how many players are human-operated. Human players are assigned to the
   first note-bearing tracks in the MIDI file.
4. Set the timing parameters for each virtual player and the pairwise alpha/beta
   matrices.
5. Run the performance from the DAW transport.
6. Collect the generated `Log_HH-MM-SS_DDMonYYYY.csv` file from the Documents
   folder and analyse onset timing, asynchronies, and model settings.

See [User Guide](docs/USER_GUIDE.md) for parameter definitions and log-column
notes, and [Timing Model](docs/MODEL.md) for equations, defaults, units, and
citations.

## Building From Source

AdaptiveMetronome is a JUCE project defined by
[AdaptiveMetronome.jucer](AdaptiveMetronome.jucer). To build it yourself:

1. Install JUCE and open the project in Projucer.
2. Check that the JUCE module paths match your local JUCE installation.
3. Save the project to generate the IDE files.
4. Build the Release target in Xcode on macOS or Visual Studio 2022 on Windows.

More detailed build notes are in [Building](docs/BUILDING.md).

## Citing

If you use AdaptiveMetronome in a publication, performance study, teaching
resource, or derived software project, please cite the software and the exact
version or commit you used.

This repository includes [CITATION.cff](CITATION.cff), so GitHub can generate a
citation from the **Cite this repository** button. Additional wording for papers
and methods sections is in [Citing AdaptiveMetronome](docs/CITING.md).

This repository also includes a local copy of the related RPPW19 workshop
abstract:
[Adaptive Metronome: A MIDI Plug-In for Modelling Cooperative Timing in Music
Ensembles](docs/references/rppw19_abstract_Sean_Enderby.pdf). Full citation
wording is in [Citing AdaptiveMetronome](docs/CITING.md).

## Authors

- Sean Enderby
- Ryan Stables
- [Massimiliano Di Luca](https://massimilianodiluca.info/) (<m.diluca@bham.ac.uk>)

## Project Links

- [ARME project](https://arme-project.co.uk/)
- [ARME Adaptive Metronome demo](https://arme-project.co.uk/demos/adaptive-metronome)
- [University of Birmingham Virtual Reality Lab](https://virtualrealitylab.netlify.app/)
- [Virtual Reality Lab research-portal entry](https://research.birmingham.ac.uk/en/publications/virtual-reality-lab/)
- [University of Birmingham ARME/virtual orchestra rehearsal news](https://www.birmingham.ac.uk/news/2024/dr-massimiliano-di-luca-leads-development-of-software-for-virtual-orchestra-rehearsals)
- [Massimiliano Di Luca](https://massimilianodiluca.info/)
- [Massimiliano Di Luca at the University of Birmingham](https://www.birmingham.ac.uk/staff/profiles/psychology/diluca-massimiliano)

## Repository Guide

- [Installation And DAW Setup](docs/INSTALLATION.md): Logic Pro and REAPER setup.
- [User Guide](docs/USER_GUIDE.md): controls, MIDI preparation, and log output.
- [Timing Model](docs/MODEL.md): equations, defaults, units, and citations.
- [Building](docs/BUILDING.md): source-build workflow for JUCE, Xcode, and Visual
  Studio.
- [Troubleshooting](docs/TROUBLESHOOTING.md): common installation, audio, MIDI,
  tapping, logging, and build issues.
- [Contributing](CONTRIBUTING.md): what to include in issues and pull requests.
- [Authors](AUTHORS.md): software authors and project cross-references.
- [Maintainer Checklist](docs/MAINTAINER_CHECKLIST.md): steps that would make the
  project easier to redistribute, archive, and cite.

## License Status

No software license is currently declared in this repository. Until a license is
added, assume that redistribution and modification require permission from the
maintainers or the relevant institution. The maintainer checklist includes this
as a high-priority repository task.

## Acknowledgements

AdaptiveMetronome was developed by the
[ARME project](https://arme-project.co.uk/) team. The project describes and
supports simulation of synchronisation among musicians, including quartet
performance scenarios based on a Haydn MIDI score. Related immersive rehearsal
research is supported by the University of Birmingham
[Virtual Reality Lab](https://virtualrealitylab.netlify.app/).
