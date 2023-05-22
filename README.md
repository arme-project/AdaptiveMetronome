The EPSRC-funded ARME project is proud to release a plug-in designed to simulate the harmonious synchronisation of musicians within a violin quartet playing a piece by Haydn. Such tool gives the ability to observe and analyse event times recorded during simulated ensemble performance. Virtual musicians perform their notes using a linear phase correction model, meticulously adjusting their timing to maintain a harmonious collective. The plug-in supports the configuration of the quartet's composition, accommodating any combination of virtual and human players Key to the virtual musicians' performances is the understanding of music cohesion which is reflected in various control parameters that empower the user to tailor their experience. 

**Instruction for installation on Logic Pro for Mac:**

1) Download the latest release (on the right side of this page)
2) Extract the AU plugin contained in the zip file and move it to the following folder: Finder\Macintosh HD\Library\Audio\Plugins\Components
3) Download the midi file of the Haydn piece from this repo: https://github.com/maxdiluca/haydn_midi/archive/refs/tags/download.zip
4) Extract the midi contained in the zip file
5) Open Logic Pro
6) If an error occurs with the plugin, go to Settings>'privacy and security', uder the “allow applications downloaded from” Select "App Store and identified developers", and under “AdaptiveMetronome.component was blocked from use because it is not from an identified developer”, click “Allow anyways”. Enter the password of an account with admin privileges. Close and re-open Logic Pro
7) Create a new empty project. Choose Software Instrument or once the project is open select Track>New software instrument track.
8) In the Library, select Orchestral, Strings, String ensemble (or another instrument of your choice)
9) In the midi section of the new track select MIDI FX->Audio Units->ARME->Adaptive metronome
10) Press Load MIDI in the new window the midi file

Video instructions:

[![Installation Video](https://img.youtube.com/vi/2CeIm4auh44/0.jpg)](https://www.youtube.com/watch?v=2CeIm4auh44)




**Instructions for playing on Logic Pro for Mac:**

1) Change the parameters of the simulation on the console
2) Hit the “play” triangle in the main window to start the simulated performance
3) After the initial 4 metronome beats, press J to play each note (if this doesn’t work go to Window>Show keyboard and check that by pressing J you can hear the notes playing)

Video instructions:

[![Operation Video](https://img.youtube.com/vi/HKUYVPlAp8E/0.jpg)](https://www.youtube.com/watch?v=HKUYVPlAp8E)








**Instruction for installation on Reaper for Windows:**

1) Download the latest release (on the right side of this page)
2) Extract the VST3 plugin contained in the zip file and move it to the following plugin folder:  C:\Program Files\Common Files\VST3
3) Download the MIDI file: https://github.com/maxdiluca/haydn_midi/archive/refs/tags/download.zip
4) Extract the MIDI contained in the zip file and move it to your documents folder
5) Open Reaper
6) On a new empty project, create a new track and add the plugin by navigating to View > Browse FX and search for VST3: AdaptiveMetronome (ARME)
7) In the new window add an instrument to the track by clicking on "Add" at the bottom left of the window, or by navigating to View > Browse FX
8) In the FX window related to this track (if you have closed it you can re-open by clicking on the green FX button on the track), load a MIDI file by pressing the "Load MIDI" button.
