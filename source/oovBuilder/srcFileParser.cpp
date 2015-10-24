//
// C++ Implementation: srcFileParser
//
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#include "srcFileParser.h"
#include <stdio.h>
#include "Project.h"
#include "FilePath.h"
#include "OovProcess.h"
#include "ComponentFinder.h"


bool srcFileParser::analyzeSrcFiles(OovStringRef const srcRootDir,
        OovStringRef const analysisDir)
    {
    mSrcRootDir = srcRootDir;
    mAnalysisDir = analysisDir;

#define MULTIPLE_THREADS 1
#if(MULTIPLE_THREADS)
    // This requires that the oovaide-incdeps file can be updated by multiple processes.
    setupQueue(getNumHardwareThreads());
#else
    setupQueue(1);
#endif
    mIncDirArgs = mComponentFinder.getAllIncludeDirs();
    mExcludeDirs = mComponentFinder.getProjectBuildArgs().getProjectExcludeDirs();
    OovStatus status = recurseDirs(srcRootDir);
    mIncDirArgs.clear();
    waitForCompletion();
    return status.ok();
    }

VerboseDumper sVerboseDump;

void VerboseDumper::open(OovStringRef const outPath)
    {
    mFp = stdout;
    /*
    std::string path = outPath;
    ensureLastPathSep(path);
    path += "OovBuilder.txt";
    DebugFile::open(path);
    */
    }

void VerboseDumper::logProcess(OovStringRef const srcFile,  char const * const *argv, int argc)
    {
    if(mFp)
        {
        fprintf(mFp, "\nOovBuilder: %s", srcFile.getStr());
        if(argc > 0)
            {
            for(int i=0; i<argc; i++)
                fprintf(mFp, " %s", argv[i]);
            fprintf(mFp, "\n");
            }
        }
    }

void VerboseDumper::logProgress(OovStringRef const progress)
    {
    if(mFp)
        {
        fprintf(mFp, "\nOovBuilder: %s", progress.getStr());
        }
    }

void VerboseDumper::logOutputOld(OovStringRef const fn)
    {
    if(mFp)
        {
        fprintf(mFp, "OovBuilder: Output older than %s\n", fn.getStr());
        }
    }


bool srcFileParser::processFile(OovStringRef const srcFile)
    {
    bool success = true;
    if(!ComponentsFile::excludesMatch(srcFile, mExcludeDirs))
        {
        FilePath ext(srcFile, FP_File);
        if(isCppHeader(ext) || isCppSource(ext) || isJavaSource(ext))
            {
            struct OovStat32 srcFileStat;
            success = (OovStatFunc(srcFile, &srcFileStat) == 0);
            if(success)
                {
                OovString srcRoot = mSrcRootDir;
                FilePathEnsureLastPathSep(srcRoot);
                OovString outFileName = Project::makeAnalysisFileName(srcFile,
                        srcRoot, mAnalysisDir);
                OovStatus status(true, SC_File);
                if(FileStat::isOutputOld(outFileName, srcFile, status))
                    {
                    std::string procPath = ToolPathFile::getAnalysisToolCommand(ext);
                    CppChildArgs ca;
                    ca.addArg(procPath);
                    ca.addArg(srcFile);
                    ca.addArg(mSrcRootDir);
                    ca.addArg(mAnalysisDir);

                    ca.addCompileArgList(mComponentFinder, mIncDirArgs);
                    addTask(ca);
    /*
                    sLog.logProcess(srcFile, ca.getArgv(), ca.getArgc());
                    printf("\noovBuilder Analyzing: %s\n", srcFile);
                    fflush(stdout);
                    OovProcessStdListener listener;
                    int exitCode;
                    OovPipeProcess pipeProc;
                    success = pipeProc.spawn(procPath, ca.getArgv(), listener, exitCode);
                    if(!success)
                        fprintf(stderr, "OovBuilder: Unable to execute process %s\n", procPath.getStr());
                    if(!success || exitCode != 0)
                        {
                        fprintf(stderr, "oovBuilder: Errors from %s\nArguments were: ", procPath.getStr());
                        ca.printArgs(stderr);
                        }
                    fflush(stdout);
                    fflush(stderr);
    */
                    }
                /// @todo - notify oovaide when files are ready to parse?
                }
            }
        }
    return success;
    }

bool srcFileParser::processItem(CppChildArgs const &item)
    {
    OovProcessBufferedStdListener listener(mListenerStdMutex);
    int exitCode;
    OovPipeProcess pipeProc;
    OovString processStr = "\noovBuilder Analyzing: ";
    processStr += item.getArgv()[1];
    processStr += "\n";
    printf("%s", processStr.getStr());
    fflush(stdout);
    listener.setProcessIdStr(processStr);
    bool success = pipeProc.spawn(item.getArgv()[0], item.getArgv(),
            listener, exitCode);
    if(!success || exitCode != 0)
        {
        OovString tempStr;
        if(!success)
            {
            tempStr = "OovBuilder: Unable to execute process ";
            }
        else
            {
            tempStr = "OovBuilder: Process returned error ";
            }
        tempStr += item.getArgv()[0];
        if(success)
            {
            tempStr += " ";
            tempStr.appendInt(exitCode);
            }
        if(!pipeProc.isArgLengthOk(static_cast<int>(tempStr.length() + item.getArgsAsStr().length())))
            {
            tempStr += "\nToo long of command arguments.";
            }
        tempStr += "\nArguments were: ";
        tempStr += item.getArgsAsStr();
        tempStr += "\n";
        listener.onStdErr(tempStr, tempStr.length());
        }
    return true;
    }
