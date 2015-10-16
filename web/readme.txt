The Oovaide web site is at http://oovaide.sourceforge.net or
	https://github.com/animatedb/oovaide.
	The User Guide contains Examples that show the use of Oovaide.
	The Releases list the features added in each version.
	The Features shows a current list of functionality.


Analyzing Programs - Windows Versions:
	The oovaide-*-win downloads include parts of GTK-3.0 and CLang
	so running analysis does not require any additional
	downloads. Using Oovaide for more complete analysis usually
	requires some MinGW or other package so that CLang can find
	standard include files. Using database export requires sqlite3.

Analyzing Programs - Linux Versions:
	The oovaide-*-linux downloads require GTK3 and a 64 bit OS.
	Part of CLang is included so running analysis does not
	require any additional downloads. Using Oovaide for more
	complete analysis usually requires some other package so
	that CLang can find standard include files. Using database
 	export requires sqlite3.


Building Programs - Windows Versions:
	Using Oovaide to build requires MinGW, MinGW-builds or
	MinGW-W64 because they supply the nm and ld tools.

	MinGW-W64 or MinGW-builds 32 bit:
		This is required to build Oovaide since it contains
		threading.
		Set the environment path to the bin directory.

		On windows, CLang has hard coded paths, and is
		typically for \MinGW.
		To use the MinGW-W64 paths, use the External Project
		Packages button	in the Analysis/Settings dialog.

		From: http://mingw-w64.sourceforge.net/download.php
		File: i686-5.1.0-release-posix-dwarf-rt_v4-rev0
		Or:
		From: http://sourceforge.net/projects/mingwbuilds/
		File: mingw-builds-install.exe
			Install to C:\Program Files\mingw-builds

	MinGW:
		From: http://sourceforge.net/projects/mingw/files/
		File: "mingw-get-setup.exe"
			Add package mingw32-gcc-g++	4.8.1-4
			Add package mingw32-base	2013072200
			Install to \MinGW

	Also required for building Oovaide:
	GTK-3.0:
		From: http://www.tarnyko.net/dl/
		File: "GTK+ 3.6.4 Bundle for Windows"
			Extract to "C:\Program Files\GTK+-Bundle-3.6.4"

	Clang 3.4-3.6:
		From: http://llvm.org/releases/download.html
		File: "Clang for Windows (.sig)"
			Install to C:\Program Files\LLVM


Building Programs - Linux Versions:
	Either the Gnu or CLang compiler is required for building.

	The easiest way to build Oovaide is using git and is described
	at https://github.com/animatedb/oovaide.
	Eclipse or CMake is supported for building Oovaide.

	On ubuntu, external packages gtk+-3.0 and libclang-dev
	are required in order to build Oovaide.

	To build oovaide on Linux using CMake (Debian/Ubuntu):
		- Run "sudo apt-get install libgtk-3-dev clang libclang-dev"
		  from a terminal
		- LLVM include and lib paths are in the top level CMakeLists.txt
		- Run "cmake ./" from the oovaide top level directory
		- Run "make" from the same directory


Requesting Features and Bug Fixes:
	Some bugs are listed under Tickets, in the sourceforge web
	site. In addition, release notes and issues can be seen under
	"Code" in trunk/docs/releaseNotes.txt.  Add new requests
	to either the SourceForge or Github issue pages.

Contribute:
	If you would like to contribute, contact me at either web site.
