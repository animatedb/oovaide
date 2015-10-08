
# Oovcde

An object oriented analysis and code development platform that automatically
generates build, class, sequence, zone, portion, and component diagrams for C++
and Objective C languages.

[Oovcde Home Page](http://oovcde.sourceforge.net/)

## Contents

 - [Features](#features)
 - [Download](#download)
 - [Quick start](#quick-start)
 - [Project Goals](#project-goals)
 - [Other Documentation](#other-documentation)
 - [License](#license)


## Features

- The analysis system searches directories for source code and uses CLang.
	- Diagrams
		- Dynamically generated allowing quick navigation between objects.
		- Edit within in Oovcde and save and restore complex diagrams.
		- Save as SVG and edit using tools such as Inkscape.
		- Component dependency diagrams
		- Zone diagrams (view thousands of classes)
		- Class diagrams
		- Portion diagrams (view class attribute/member relations)
		- Sequence diagrams 
		- Include dependency diagrams
	- Analysis
		- Dependencies are generated automatically between files and components.
		- Code coverage
			- Instruments code and produces coverage information.
			- Can be used to find dead code (dynamic).
		- Code complexity
			- Provides complexity for methods and classes.
		- Duplicate code
		- Dead code detection (static)
			- Class data member usage
			- Class method usage
		- Line counts
		- Project statistics
		- Export to sqlite database
	- Build
		- Analysis information is used for easy setup.
		- Multithreaded based on number of cores
	- Code editor
		- Syntax highlighting and debugging (GDB).

## Download

Download the Oovcde binaries from the releases or from [Sourceforge]
(http://sourceforge.net/projects/oovcde/files/). There is a version for Linux,
and one for Windows.

If you would like to build from source:
- mkdir oovcde-git
- cd oovcde-git
- git clone https://github.com/animatedb/oovcde.git
- cd oovcde
- cmake source
- make
- sudo make install

The following dependencies are required:
- libclang-dev   (If there are compile error for clang-c/Index.h, change
  oovcde/source/CMakelists.txt so llvm-x.x matches the version on your system.)
- lib-gtk-3-dev

Depending on the version of LLVM on your system, it may be required that
the CMakeLists.txt file is modified to use a different version. GTK3 is also
required.


## Quick Start

The quickest way to get started is to download the program, then run the examples.
The explanation of the examples is here.
[oovcde.sourceforge.net/userguide/examples.shtml]
(http://oovcde.sourceforge.net/userguide/examples.html)


## Project Goals

- Maximize programmer productivity
	- visibility of code and design information
	- simple setup
	- background operations
	- fast (written in C++)
	- simple (minimal dependencies)


## Other Documentation

 - The user guide is [http://oovcde.sourceforge.net/userguide/oovcdeuserguide.shtml]
	(http://oovcde.sourceforge.net/userguide/oovcdeuserguide.shtml)
 - The design documentation is [http://oovcde.sourceforge.net/design/OovcdeDesign.html]
	(http://oovcde.sourceforge.net/design/OovcdeDesign.html)
 - The existing Oovcde website is at http://oovcde.sourceforge.net/


## License
The software is licensed using the GPLv2.
