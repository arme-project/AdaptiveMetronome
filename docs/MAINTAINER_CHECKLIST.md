# Maintainer Checklist

These repository tasks would make AdaptiveMetronome easier for other people to
install, reuse, and cite.

## High Priority

- Choose and add a clear software license after confirming institutional and
  project requirements.
- Add a DOI for stable public releases, for example through Zenodo's GitHub
  integration.
- Keep `CITATION.cff` up to date with release version, DOI, authors, and release
  date.
- Add release notes for every packaged build, including operating system, plug-in
  format, DAW versions tested, and known issues.
- Attach checksums to release binaries so users can verify downloads.

## Documentation

- Add screenshots of the plug-in window and example DAW routing.
- Add a small example MIDI file whose license permits redistribution in this
  repository.
- Add a sample CSV log and a short analysis notebook or script.
- Keep the timing model documentation aligned with source-code changes.
- Add validation examples that compare expected model behaviour with generated
  logs.

## Engineering

- Add automated formatting or linting for C++ files.
- Add a small non-realtime test harness for MIDI-file parsing and model timing.
- Save and restore plug-in parameter state through the host.
- Surface MIDI-load errors in the UI.
- Add an in-plug-in setting for the log output folder.

## Release Checklist

Before publishing a release:

1. Build the macOS AU and Windows VST3 in Release mode.
2. Smoke-test each build in at least one DAW.
3. Load the example MIDI file and confirm that a CSV log is generated.
4. Tag the commit.
5. Upload binaries, checksums, and release notes.
6. Update `CITATION.cff`.
7. Archive the release if a DOI is required.
