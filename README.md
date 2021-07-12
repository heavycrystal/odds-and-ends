# odds-and-ends
Contains small [mostly one file] projects that are too small to warrant their own repository.

Currently contains:

1. chip8.cpp is a one-file emulator for the CHIP-8 platform. Written only using SDL2. Passes most compatibility tests I've found, but lacks sound.

2. gradiente.cpp is a [frankly overengineered] program that procedurally generates random gradient wallpapers of an arbitrary size and writes them to a image file of the .PPM file format. No external libraries were used.

3. sha256.c is a strict ANSI-C/C89 implementation of the popular SHA-256 hashing algorithm. Even very old C compilers should be able to compile this. Due to lack of fixed-width integer types, users will need to override the appropriate typedefs manually.  

4. micro_backend.py is a Python script that calls the Spotify API regularly to save the details of the current playback and also make snapshots of playlists that refresh frequently. Saved to a database using SQLAlchemy [I recommend SQLite as the backing data store], needs to run constantly and app credentials will need to be manually provided. Requires spotipy and SQLAlchemy.


