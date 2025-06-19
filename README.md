<h1 align="center"> Adaptive Metronome </h1>

<p align="center">

The EPSRC-funded [ARME](https://arme-project.co.uk/) project has released a plug-in that simulates how musicians in a violin quartet stay in sync while playing a piece by Haydn. This tool lets users observe and analyse the timing of each musician’s performance during the simulation. Virtual players use a linear phase correction model to adjust their timing and stay in harmony with the group. The plug-in allows users to set up different combinations of virtual and human players, and includes control settings to customize how the virtual musicians respond and play together.

A set-up documentation website has been created to help understand how you are able to use the Adaptive Metronome with Max 9, as well as getting started a development environment if you are interested in contributing to the project. You can find the link to the site [here](https://arme-project.github.io/ARME-System-Setup/juce-plugin/about/)

---
  
`evaluation_deocupling` branch:

This branch contains descriptions for functions found in the `EnsembleModel.h` which corresponds to functions and replaces `new_comments` branch. The aim of this branch is decouple the logics from the `EnsembleModel.cpp` into smaller maintainable classes (OSC, Logs, Polling, and Configs). 
