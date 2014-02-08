//============================================================================
// Name        : oovCppParser.cpp
//  \copyright 2013 DCBlaha.  Distributed under the GPL.
//============================================================================

// This parses C++ source files and saves some parsed information in .XMI files.
// It also saves include file information in a different text file.
//
// This uses the libtooling interface of CLang, which is in the Index.h file.
// http://clang.llvm.org/doxygen/group__CINDEX.html

#include "CppParser.h"
#include "Version.h"
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <stdio.h>


#define DEBUG_CPP_PARSE 0

static CppParser sCppParser;

int main(int argc, char const *const argv[])
    {
    CppParser::eErrorTypes et = CppParser::ET_None;
#if(DEBUG_CPP_PARSE)
    if(argc == 1)
	{
	int numArgs = 0;
	char const *cppArgv[40];
	cppArgv[numArgs++] = "";
	cppArgv[numArgs++] = "../oovcde/Builder.h";
	cppArgv[numArgs++] = "../";
	cppArgv[numArgs++] = "../../../../trunk-oovcde/";
	cppArgv[numArgs++] = "-x";
	cppArgv[numArgs++] = "c++";
	cppArgv[numArgs++] = "-std=c++11";
	cppArgv[numArgs++] = "-I/mingw/include";
	cppArgv[numArgs++] = "-I/mingw/lib/gcc/mingw32/4.6.2/include";
	cppArgv[numArgs++] = "-I/mingw/lib/gcc/mingw32/4.6.2/include/c++";
	cppArgv[numArgs++] = "-I/mingw/lib/gcc/mingw32/4.6.2/include/c++/mingw32";
	cppArgv[numArgs++] = "-IC:/Program Files/GTK+-Bundle-3.6.1/include/gtk-3.0";
	cppArgv[numArgs++] = "-IC:/Program Files/GTK+-Bundle-3.6.1/include/glib-2.0";
	cppArgv[numArgs++] = "-IC:/Program Files/GTK+-Bundle-3.6.1/lib/glib-2.0/include";
	cppArgv[numArgs++] = "-IC:/Program Files/GTK+-Bundle-3.6.1/include/pango-1.0";
	cppArgv[numArgs++] = "-IC:/Program Files/GTK+-Bundle-3.6.1/include/cairo";
	cppArgv[numArgs++] = "-IC:/Program Files/GTK+-Bundle-3.6.1/include/gdk-pixbuf-2.0";
	cppArgv[numArgs++] = "-IC:/Program Files/GTK+-Bundle-3.6.1/include/atk-1.0";
	cppArgv[numArgs++] = "-I../oovCommon";
//	success = sCppParser.parse("../../../testinput/testaggr.cpp", "../../../testinput/", "../../../testoutput/", cppArgv, numArgs);
//	success = sCppParser.parse("../testinput/cParam.h", "../testinput/", "../testoutput/", cppArgv, numArgs);
//	success = sCppParser.parse("../oovCommon/ModelObjects.h", "../", "../../../../trunk-oovcde/", cppArgv, numArgs);
	et = sCppParser.parse(cppArgv[1], cppArgv[2], cppArgv[3], &cppArgv[4], numArgs-4);
    }
    else
#endif
    if(argc >= 4)
	{
	// This saves the CPP info in an XMI file.
	et = sCppParser.parse(argv[1], argv[2], argv[3], &argv[4], argc-4);
	if(et != CppParser::ET_None && et != CppParser::ET_CompileWarnings)
	    {
	    fprintf(stderr, "oovCppParser: Error analyzing file %s\n", argv[1]);
	    }
	}
    else
	{
	fprintf(stderr, "OovCppParser version %s\n", OOV_VERSION);
	fprintf(stderr, "oovCppParser: Args are: sourceFilePath sourceRootDir outputProjectFilesDir [cppArgs]...\n");
	}
    int exitCode = 0;
    if(et != CppParser::ET_None && et != CppParser::ET_CompileWarnings)
	exitCode = EXIT_FAILURE;
    return exitCode;
    }
