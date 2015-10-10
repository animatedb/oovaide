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
#include "OovProcessArgs.h"
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <stdio.h>
#include <string.h>


static CppParser sCppParser;

int main(int argc, char const *const argv[])
    {
    CppParser::eErrorTypes et = CppParser::ET_None;
    OovError::setComponent(EC_OovCppParser);
    if(argc >= 4)
        {
        bool dupHashes = false;
        OovProcessChildArgs childArgs;
        for(int i=4; i<argc; i++)
            {
            if(strcmp(argv[i], "-dups") == 0)
                {
                dupHashes = true;
                }
            else
                {
                childArgs.addArg(argv[i]);
                }
            }
        // This saves the CPP info in an XMI file.
//      et = sCppParser.parse(dupHashes, argv[1], argv[2], argv[3], &argv[4], argc-4);
        et = sCppParser.parse(dupHashes, argv[1], argv[2], argv[3],
            childArgs.getArgv(), static_cast<int>(childArgs.getArgc()));
        if(et == CppParser::ET_CLangError)
            {
            fprintf(stderr, "oovCppParser: CLang error analyzing file %s.\n"
                    "It could be an argument error (Windows spaces in path), or a bug in CLang\n", argv[1]);
            }
        else if(et != CppParser::ET_None && et != CppParser::ET_CompileWarnings)
            {
            fprintf(stderr, "oovCppParser: Error analyzing file %s\n", argv[1]);
            }
        }
    else
        {
        fprintf(stderr, "OovCppParser version %s\n", OOV_VERSION);
        fprintf(stderr, "oovCppParser args are: sourceFilePath sourceRootDir outputProjectFilesDir [cppArgs]...\n");
        }
    int exitCode = 0;
    if(et != CppParser::ET_None && et != CppParser::ET_CompileWarnings)
        exitCode = EXIT_FAILURE;
    return exitCode;
    }
