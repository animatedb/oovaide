
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

- The analysis system searches directories for source code and uses CLang.
	- Dependencies are generated automatically between files and components.
	- Object relationships and sequence information is saved for diagrams.
	- Diagrams (zone, class, sequence, and dependency) can be dynamically
	  generated allowing quick navigation
	  between objects. Diagrams can be edited in Oovcde, then saved as
	  SVG and edited using tools such as Inkscape.
- The multithreaded build system uses analysis information for easy setup.
- The code editor has syntax highlighting, and a debugger.
- Code test coverage instruments the code and produces coverage information.
- Complexity measurements are provided for each method of a class.
- Duplicate code is found and listed.


## Download

Download the Oovcde software from [Sourceforge]
(http://sourceforge.net/projects/oovcde/files/). There is a version for Linux,
and one for Windows.


## Quick Start

The quickest way to get started is to download the program, then run the examples.
The explanation of the examples is here.
[oovcde.sourceforge.net/userguide/examples.shtml]
(http://oovcde.sourceforge.net/userguide/examples.shtml)


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
