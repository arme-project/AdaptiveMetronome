# Contributing

Thanks for helping improve AdaptiveMetronome. The most useful contributions are
clear bug reports, DAW setup notes, documentation fixes, reproducible MIDI test
cases, and focused pull requests.

## Reporting A Problem

Please include:

- Operating system and version.
- DAW and version.
- Plug-in format, for example AU or VST3.
- AdaptiveMetronome version or Git commit.
- The MIDI file shape, especially number of note-bearing tracks.
- What you expected to happen.
- What actually happened.
- Whether a CSV log was generated in the Documents folder.

Small MIDI files that reproduce a problem are especially helpful, provided you
have permission to share them.

## Pull Requests

- Keep changes focused.
- Do not commit generated `Builds` or `JuceLibraryCode` folders.
- Update documentation when user-facing behaviour changes.
- Mention which DAW or host you used for a smoke test.
- Avoid unrelated formatting churn in files you are not changing.

## Development Notes

The project is maintained as a JUCE `.jucer` project. Open
`AdaptiveMetronome.jucer` in Projucer, update local JUCE module paths, and save
the project to regenerate IDE files.
