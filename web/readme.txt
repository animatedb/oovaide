The Oovcde web site is at http://oovcde.sourceforge.net.
	The User Guide contains Examples that show the use of Oovcde.
	The Releases list the features added in each version.
	The Features shows a current list of functionality.


Analysis - Windows Versions:
	The oovcde-*-win downloads include parts of GTK-3.0 and CLang
	so running analysis does not require any additional
	downloads. Using Oovcde for more complete analysis usually
	requires some MinGW or other package so that CLang can find
	standard include files.

Analysis - Linux Versions:
	The oovcde-*-linux downloads require GTK3 and a 64 bit OS.
	Part of CLang is included so running analysis does not
	require any additional downloads. Using Oovcde for more
	complete analysis usually requires some other package so
	that CLang can find standard include files.


Building - Windows Versions:
	Using Oovcde to build requires MinGW, MinGW-builds or
	MinGW-W64 because they supply the nm and ld tools.

	MinGW-W64 or MinGW-builds 32 bit:
		This is required to build Oovcde since it contains
		threading.
		Set the environment path to the bin directory.

		On windows, CLang has hard coded paths, and is
		typically for \MinGW.
		To use the MinGW-W64 paths, use the External Project
		Packages button	in the Analysis/Settings dialog.

		From: http://mingw-w64.sourceforge.net/download.php
		File: i686-4.92-release-posix-dwarf-rt_v3-rev1
		Or:
		From: http://sourceforge.net/projects/mingwbuilds/
		File: mingw-builds-install.exe
			Install to C:\Program Files\mingw-builds
			Using x32-4.8.1-posix-dwarf-rev5

	MinGW:
		From: http://sourceforge.net/projects/mingw/files/
		File: "mingw-get-setup.exe"
			Add package mingw32-gcc-g++	4.8.1-4
			Add package mingw32-base	2013072200
			Install to \MinGW

	Also required for building Oovcde:
	GTK-3.0:
		From: http://www.tarnyko.net/dl/
		File: "GTK+ 3.6.4 Bundle for Windows"
			Extract to "C:\Program Files\GTK+-Bundle-3.6.4"

	Clang 3.4-3.6:
		From: http://llvm.org/releases/download.html
		File: "Clang for Windows (.sig)"
			Install to C:\Program Files\LLVM


Building - Linux Versions:
	Either the Gnu or CLang compiler is required for building.

Building Oovcde:
	Eclipse or CMake is supported for building Oovcde.

	On ubuntu, external packages gtk+-3.0 and
	gmodule-2.0 should be linked in order to build Oovcde.

	To build oovcde on Linux using CMake (Debian/Ubuntu):
		- Run "sudo apt-get install libgtk-3-dev clang libclang-dev"
		  from a terminal
		- LLVM include and lib paths are in the top level CMakeLists.txt
		- Run "cmake ./" from the oovcde top level directory
		- Run "make" from the same directory


Reporting bugs:
	Some bugs are listed under Tickets, in the sourceforge web
	site. In addition, release notes and issues can be seen under
	"Code" in trunk/docs/releaseNotes.txt.
