/*
 * DuplicatesView.cpp
 *
 *  Created on: Jun 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */
#include "DuplicatesView.h"
#include "Project.h"


eDupReturn createDuplicatesFile(/*OovStringRef const projDir,*/ std::string &outFn)
    {
    eDupReturn ret = DR_NoDupFilesFound;
    DuplicateOptions options;
    std::vector<cDuplicateLineInfo> dupLineInfo;
    if(getDuplicateLineInfo(options, dupLineInfo))
        {
        FilePath outPath(Project::getProjectDirectory(), FP_Dir);
        outPath.appendDir(DupsDir);
        outPath.appendFile("Dups.txt");
        File outFile(outPath, "w");
        if(outFile.isOpen())
            {
            outFn = outPath;
            for(auto const &lineInfo : dupLineInfo)
                {
                fprintf(outFile.getFp(), "lines %u  :  %s %u  :  %s %u\n",
                    lineInfo.mTotalDupLines,
                    lineInfo.mFile1.getStr(), lineInfo.mFile1StartLine,
                    lineInfo.mFile2.getStr(), lineInfo.mFile2StartLine);
                }
            ret = DR_Success;
            }
        else
            {
            ret = DR_UnableToCreateDirectory;
            }
        }
    return ret;
    }

