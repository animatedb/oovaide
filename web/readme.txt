The Oovcde web site is at http://oovcde.sourceforge.net.
	The User Guide contains Examples that show the use of Oovcde.
	The Releases list the features added in each version.
	The Features shows a current list of functionality.


CMake is supported for custom builds.


Windows versions:
	Parts of GTK-3.0 and CLang are included in the oovcde-*-win
	downloads so drawing analysis does not require any additional
	downloads. MinGW is required for building because it supplies
	the nm and ld tools.
	GTK-3.0
		From: http://www.tarnyko.net/dl/
		File: "GTK+ 3.6.4 Bundle for Windows"
			Extract to "C:\Program Files\GTK+-Bundle-3.6.4"
	Clang 3.4
		From: http://llvm.org/releases/download.html
		File: "Clang for Windows (.sig)"
			Install to C:\Program Files\LLVM
	MinGW:
		From: http://sourceforge.net/projects/mingw/files/
		File: "mingw-get-setup.exe"	0.6.2-beta-20131004-1
			Add package mingw32-gcc-g++		4.8.1-4
			Add package mingw32-base		2013072200
			Add package mingw32-pthreads-w32 dev	2.9.1-1
			Install to \MinGW


Linux versions:
	For the prebuilt binaries, GTK3 and a 64 bit OS is required.
	A compiler is not required for drawing analysis since
	libclang.so is included, but either the Gnu or CLang compiler
	is required for building.



Windows versions on or before 2014-01:
	GTK-3.0
		From:	http://www.tarnyko.net/dl/
		File:	"version 3.6.1 TARNYKO 11-13-2012 for Windows"
	CLang 3.2
		From:	http://llvm.org/releases/download.html
		File:	"Experimental Clang Binaries for Mingw32/x86"
	MinGW
		From:	https://sourceforge.net/projects/oovcde/files/
		File:	mingw-get-inst-20120426.exe


Release notes and issues can be seen under "Code" in
trunk/docs/releaseNotes.txt.
