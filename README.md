## DataAggr v0.1

DataAggr is a zero-friction note-taking app for Windows. With a hotkey you can bring up the note-taking window, type something in quickly, then close the window with the same hotkey. Your notes will be appended to a text file along with a timestamp.

### How to use

Start the application. Use Ctrl-Space to bring up the DataAggr window. Type something. Use Ctrl-Space again to save your text and hide the window again.

The default file to save notes in is notes.txt. If you want to save your notes in a different file, use the following syntax to enter the note:
:file(filename);Notes go here

Here's what it looks like:
![App window screenshot](/img/app.png?raw=true "The DataAggr window")

And here's what the notes.txt file looks like:
![Notes text file screenshot](/img/notes.png?raw=true "Notes saved to notes.txt")

To have automatic backups of the notes, I would recommend placing the main.exe and notes.txt files in DropBox or something similar.

A recent binary is included in the repo if you don't want to build from source.

### How to build from source

I've only tested building the project using the Microsoft Visual C++ Toolset (MSVC).
Here is the simplest way to build from source:

1. Install VSCode
2. Install MSVC and set it up with VS Code using these instructions: https://code.visualstudio.com/docs/cpp/config-msvc
3. Launch VSCode from the Developer Command prompt as described in the link above
4. Run the build task (Terminal => Run Build Task)
5. main.exe should be built and put into the bin directory.

### About me

I'm a Software Engineer based out of London. I built DataAggr back in 2011, when I was in college and needed a fast and easy way to take notes.

The code lay untouched for over ten years until 2022, when I revived this project. Shockingly, the binary that I'd built back in 2011, probably on Windows Vista, still worked perfectly on Windows 10 :) Who says Microsoft doesn't care about backwards compatibility?