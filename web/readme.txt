The Oovaide web site is at http://oovaide.sourceforge.net or
	https://github.com/animatedb/oovaide.
	The User Guide contains Examples that show the use of Oovaide.
	The Releases list the features added in each version.
	The Features shows a current list of functionality.


Using OovAide to analyze programs:
    Windows Versions:
	The oovaide-*-win downloads include parts of GTK-3.0 and CLang
	so running analysis does not require any additional
	downloads. Using Oovaide for more complete analysis usually
	requires some MinGW or other package so that CLang can find
	additional include files. Using database export requires
	sqlite3. Analyzing java source code requires the Java JDK.

     Linux Versions:
	The oovaide-*-linux downloads require GTK3, CLang and a 64
        bit OS. Using database export requires sqlite3. Analyzing
	java source code requires the Java JDK.

Using OovAide to build programs:
    Either G++ or CLang is required to build C++ programs using
    OovAide.  The Java compiler is required to build java programs.


Building OovAide - Windows Versions:
	Using Oovaide to build requires MinGW, MinGW-builds or
	MinGW-W64 because they supply the nm and ld tools.

	MinGW-W64 32 bit:
		This is required to build Oovaide since it contains
		threading.
		Set the environment path to the bin directory.

		On windows, CLang has hard coded paths, and is
		typically for \MinGW.
		To use the MinGW-W64 paths, use the External Project
		Packages button	in the Analysis/Settings dialog.

		From: http://sourceforge.net/projects/mingw-w64/
		File: x86_64-mingw32-gcc-4.7.0-release-c,c++,fortran-sjlj.7z

	GTK-3.0:
		From: http://www.tarnyko.net/dl/gtk.htm
		File: gtk+-bundle_3.6.4-20130513_win32.zip
			Extract to "C:\Program Files\GTK+-Bundle-3.6.4"

	Clang 3.4-3.6:
		From: http://llvm.org/releases/download.html
		File: LLVM-3.6.2-win32.exe
			Install to C:\Program Files\LLVM


Building OovAide - Linux Versions:
	Either the Gnu or CLang compiler is required for building.

	The easiest way to build Oovaide is using git and is described
	at https://github.com/animatedb/oovaide.
	Eclipse or CMake is supported for building Oovaide.

	On ubuntu, external packages gtk+-3.0 and libclang-dev
	are required in order to build Oovaide.

	To build oovaide on Linux using CMake (Debian/Ubuntu):
		- Run "sudo apt-get install libgtk-3-dev clang
		  libclang-dev"
		  from a terminal
		- LLVM include and lib paths are in the top level
		  CMakeLists.txt
		- Run "cmake ./" from the oovaide top level directory
		- Run "make" from the same directory


Requesting Features and Bug Fixes:
	Some bugs are listed under Tickets, in the sourceforge web
	site. In addition, release notes and issues can be seen under
	"Code" in trunk/docs/releaseNotes.txt.  Add new requests
	to either the SourceForge or Github issue pages.

Contribute:
	If you would like to contribute, contact me at either web site.
