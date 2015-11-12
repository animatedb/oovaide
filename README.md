
# Oovaide

An object oriented analysis and integrated development platform that automatically
generates build, class, sequence, zone, portion, and component diagrams for C++,
Objective C, and Java languages. Also includes test coverage, complexity and
analysis statistics.

[Oovaide Home Page](http://oovaide.sourceforge.net/)

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
		- Edit within in Oovaide and save and restore complex diagrams.
		- Save as SVG and edit using tools such as Inkscape.
		- Component dependency diagrams
		- Zone diagrams (view thousands of classes)
		- Class diagrams
		- Portion diagrams (view class attribute/member relations)
		- Sequence diagrams 
		- Include dependency diagrams
	- Analysis
		- Dependencies are generated automatically between files and components.
		- Code coverage (Not for Java)
			- Instruments code and produces coverage information.
			- Can be used to find dead code (dynamic).
		- Code complexity
			- Provides complexity for methods and classes.
		- Duplicate code
		- Dead code detection (static)
			- Class data member usage
			- Class method usage
		- Line counts (Not for Java)
		- Project statistics
		- Export to sqlite database
	- Build
		- Analysis information is used for easy setup.
		- Multithreaded based on number of cores
	- Code editor
		- Syntax highlighting and debugging (GDB).

## Download

Download the Oovaide binaries from the [releases]
(https://github.com/animatedb/oovaide/releases) or from [Sourceforge]
(http://sourceforge.net/projects/oovaide/files/). There is a version for Linux,
and one for Windows. The Linux version requires GTK and CLang to be
installed on the computer. The Windows version does not require any extra downloads
for analysis, but CLang or some minGW package is required for building.

If you would like to build from source:
- mkdir oovaide-git
- cd oovaide-git
- git clone https://github.com/animatedb/oovaide.git
- cd oovaide
- cmake source
- make
- sudo make install

The following dependencies are required:
- libclang-dev   (to fix compile error for clang-c/Index.h, change file
  oovaide/source/CMakelists.txt so llvm-x.x is the matching version on your system.)
- lib-gtk-3-dev

Depending on the version of LLVM on your system, it may be required that
the CMakeLists.txt file is modified to use a different version. GTK3 is also
required.


## Quick Start

The quickest way to get started is to download the program, then run the examples.
The explanation of the examples is here.
[oovaide.sourceforge.net/userguide/examples.html]
(http://oovaide.sourceforge.net/userguide/examples.html)


## Project Goals

- Maximize programmer productivity
	- visibility of code and design information
	- simple setup
	- background operations
	- fast (written in C++)
	- simple, lightweight (minimal dependencies)


## Other Documentation

 - The user guide is [http://oovaide.sourceforge.net/userguide/oovaideuserguide.shtml]
	(http://oovaide.sourceforge.net/userguide/oovaideuserguide.shtml)
 - The design documentation is [http://oovaide.sourceforge.net/design/OovaideDesign.html]
	(http://oovaide.sourceforge.net/design/OovaideDesign.html)
 - The existing Oovaide website is at http://oovaide.sourceforge.net/


## License
The software is licensed using the GPLv2.
