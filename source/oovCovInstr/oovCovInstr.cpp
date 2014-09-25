//============================================================================
// Name        : oovCppParser.cpp
//  \copyright 2013 DCBlaha.  Distributed under the GPL.
//============================================================================

// This parses C++ source files and saves some parsed information in .XMI files.
// It also saves include file information in a different text file.
//
// This uses the libtooling interface of CLang, which is in the Index.h file.
// http://clang.llvm.org/doxygen/group__CINDEX.html

#include "CppInstr.h"
#include "Version.h"
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <stdio.h>



static CppInstr sCppInstr;

int main(int argc, char const *const argv[])
    {
    CppInstr::eErrorTypes et = CppInstr::ET_None;
    if(argc >= 4)
	{
	// This saves the CPP info in an XMI file.
	et = sCppInstr.parse(argv[1], argv[2], argv[3], &argv[4], argc-4);
	if(et != CppInstr::ET_None && et != CppInstr::ET_CompileWarnings)
	    {
	    fprintf(stderr, "oovCovInstr: Error analyzing file %s\n", argv[1]);
	    }
	}
    else
	{
	fprintf(stderr, "OovCovInstr version %s\n", OOV_VERSION);
	fprintf(stderr, "oovCovInstr: Args are: sourceFilePath sourceRootDir outputProjectFilesDir [cppArgs]...\n");
	fprintf(stderr, "     cppArgs    Standard compile options. Use -o<filename> to specify the output file\n");
	}
    int exitCode = 0;
    if(et != CppInstr::ET_None && et != CppInstr::ET_CompileWarnings)
	exitCode = EXIT_FAILURE;
    return exitCode;
    }
