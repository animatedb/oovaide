//
// C++ Implementation: srcFileParser
//
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#include "srcFileParser.h"
#include <stdio.h>
#include "Project.h"
#include "OovProcess.h"


bool srcFileParser::analyzeSrcFiles(char const * const srcRootDir,
	char const * const analysisDir)
    {
    mSrcRootDir = srcRootDir;
    mAnalysisDir = analysisDir;

#define MULTIPLE_THREADS 1
#if(MULTIPLE_THREADS)
    // This requires that the oovcde-incdeps file can be updated by multiple processes.
    setupQueue(getNumHardwareThreads());
#else
    setupQueue(1);
#endif
    mIncDirArgs = mComponentFinder.getAllIncludeDirs();
    bool success = recurseDirs(srcRootDir);
    mIncDirArgs.clear();
    waitForCompletion();
    return success;
    }

Logger sLog;

void Logger::open(char const * const outPath)
    {
    std::string path = outPath;
    ensureLastPathSep(path);
    path += "oovcde-OovBuilder.txt";
    DebugFile::open(path.c_str());
    }

void Logger::logProcess(char const * const srcFile, char const * const *argv, int argc)
    {
    if(mFp)
	{
	fprintf(mFp, "\n%s\n", srcFile);
	if(argc > 0)
	    {
	    for(int i=0; i<argc; i++)
		fprintf(mFp, " %s", argv[i]);
	    fprintf(mFp, "\n");
	    }
	}
    }

void Logger::logOutputOld(char const * const fn)
    {
    if(mFp)
	{
	fprintf(mFp, "Output older than %s\n", fn);
	}
    }

void Logger::logProcessStatus(bool success)
    {
    if(!success)
	fprintf(mFp, "Unable to execute process\n");
    }


bool srcFileParser::processFile(const std::string &srcFile)
    {
    bool success = true;
    FilePath ext(srcFile, FP_File);
    if(isHeader(ext.c_str()) || isSource(ext.c_str()))
        {
        struct OovStat32 srcFileStat;
        bool success = (OovStatFunc(srcFile.c_str(), &srcFileStat) == 0);
        if(success)
            {
            std::string outFileName;
	    std::string srcRoot = mSrcRootDir;
	    ensureLastPathSep(srcRoot);
	    Project::makeAnalysisFileName(srcFile, srcRoot, mAnalysisDir, outFileName);
	    if(FileStat::isOutputOld(outFileName.c_str(), srcFile.c_str()))
		{
#ifdef __linux__
		char const * const procPath = "./oovCppParser";
#else
		char const * const procPath = "./oovCppParser.exe";
#endif
		CppChildArgs ca;
		ca.addArg(procPath);
		ca.addArg(srcFile.c_str());
		ca.addArg(mSrcRootDir);
		ca.addArg(mAnalysisDir);

		ca.addCompileArgList(mComponentFinder, mIncDirArgs);
		addTask(ca);
/*
		sLog.logProcess(srcFile.c_str(), ca.getArgv(), ca.getArgc());
		printf("\noovBuilder Analyzing: %s\n", srcFile.c_str());
		fflush(stdout);
		OovProcessStdListener listener;
		int exitCode;
		OovPipeProcess pipeProc;
		success = pipeProc.spawn(procPath, ca.getArgv(), listener, exitCode);
		sLog.logProcessStatus(success);
		if(!success || exitCode != 0)
		    {
		    fprintf(stderr, "oovBuilder: Errors from %s\nArguments were: ", procPath);
		    ca.printArgs(stderr);
		    }
		fflush(stdout);
		fflush(stderr);
*/
		}
	    /// @todo - notify oovcde when files are ready to parse?
            }
        }
    return success;
    }

bool srcFileParser::processItem(CppChildArgs const &item)
    {
    OovProcessBufferedStdListener listener(mListenerStdMutex);
    listener.setErrOut(stdout);
    int exitCode;
    OovPipeProcess pipeProc;
    std::string processStr = "\noovBuilder Analyzing: ";
    processStr += item.getArgv()[1];
    processStr += "\n";
    listener.setProcessIdStr(processStr.c_str());
    bool success = pipeProc.spawn(item.getArgv()[0], item.getArgv(),
	    listener, exitCode);
    sLog.logProcessStatus(success);
    if(!success || exitCode != 0)
	{
	std::string tempStr = "oovBuilder: Errors from ";
	tempStr += item.getArgv()[0];
	tempStr += "\nArguments were: ";
	tempStr += item.getArgsAsStr();
	listener.onStdErr(tempStr.c_str(), tempStr.length());
	}
    return true;
    }
