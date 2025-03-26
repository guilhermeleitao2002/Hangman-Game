# RC Word Game - Socket Programming Project

## Project Overview

This project involves developing a distributed word guessing game using socket programming, inspired by the classic Hangman game. The game is implemented with two main components:

### Game Server (GS)
- Manages game logic and state
- Runs on a known IP address and port
- Selects words randomly from a predefined word file
- Supports multiple simultaneous player connections
- Handles game mechanics like word checking, error tracking, and scoreboard management

### Player Application
- Allows players to interact with the game through a command-line interface
- Supports various commands such as:
  - Starting a new game
  - Playing letters
  - Guessing the full word
  - Requesting hints
  - Viewing scoreboard
  - Checking game state
  - Quitting or exiting the game

## Communication Protocols

### UDP Protocol (Game Play)
- Used for core game interactions like:
  - Starting a new game
  - Playing letters
  - Guessing words
  - Quitting games

### TCP Protocol (Additional Services)
- Used for:
  - Retrieving scoreboard
  - Fetching hint images
  - Checking game state

## Key Features

- Player identification via 6-digit student number
- Dynamic error limit based on word length
- Case-insensitive letter guessing
- Comprehensive game state tracking
- Top 10 scoreboard
- Hint system with category-related images

## Technical Requirements

- Implemented in C or C++
- Uses socket programming interfaces
- Supports concurrent player connections
- Robust error handling
- Supports multiple system calls for network and file operations
