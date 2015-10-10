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
    std::vector<DuplicateLineInfo> dupLineInfo;
    if(getDuplicateLineInfo(options, dupLineInfo))
        {
        FilePath outPath(Project::getProjectDirectory(), FP_Dir);
        outPath.appendDir(DupsDir);
        outPath.appendFile("Dups.txt");
        File outFile;
        OovStatus status = outFile.open(outPath, "w");
        if(status.ok())
            {
            outFn = outPath;
            for(auto const &lineInfo : dupLineInfo)
                {
                OovString str = "lines ";
                str.appendInt(lineInfo.mTotalDupLines);
                str += "  :  ";
                str += lineInfo.mFile1;
                str += " ";
                str.appendInt(lineInfo.mFile1StartLine);
                str += "  :  ";
                str += lineInfo.mFile2;
                str += " ";
                str.appendInt(lineInfo.mFile2StartLine);
                str += "\n";
                status = outFile.putString(str);
                if(!status.ok())
                    {
                    break;
                    }
                }
            ret = DR_Success;
            }
        else
            {
            ret = DR_UnableToCreateDirectory;
            }
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to write to duplicates file");
            }
        }
    return ret;
    }

