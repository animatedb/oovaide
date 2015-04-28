
# Oovcde

An object oriented analysis and code development platform that automatically
generates build, class, sequence, and component diagrams for C++
and Objective C languages.


## Contents

 - [Features](#features)
 - [Download](#download)
 - [Quick start](#quick-start)
 - [Project Goals](#project-goals)
 - [Other Documentation](#other-documentation)
 - [License](#license)


## Features

- An analysis system that searches directories for source code and uses CLang
  to generate dependencies between files and components.
- The analysis system uses CLang to parse the object relationships and
  sequence information.
- Oovcde can dynamically generate diagrams allowing quick navigation between objects.
- Diagrams can be edited in Oovcde, then saved as SVG and edited using tools
  such as Inkscape.
- A multithreaded build system that uses the analysis information for easy setup.
- A simple code editor with a debugger.


## Download

Download the Oovcde software from [Sourceforge]
(http://sourceforge.net/projects/oovcde/files/). There is a version for Linux,
and one for Windows.

If you would like to build from source:
- mkdir oovcde-git
- cd oovcde-git
- git clone https://github.com/animatedb/oovcde.git
- cd oovcde
- cmake source
- make
Depending on the version of LLVM on your system, it may be required that
the CMakeLists.txt file is modified to use a different version. GTK3 is also
required.


## Quick Start

The quickest way to get started is to download the program, then run the examples.
The explanation of the examples is here.
[oovcde.sourceforge.net/userguide/examples.shtml]
(http://oovcde.sourceforge.net/userguide/examples.shtml)


## Project Goals

- Maximize programmer productivity
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
