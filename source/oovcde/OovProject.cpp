/*
 * OovProject.cpp
 *
 *  Created on: Feb 26, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "OovProject.h"
#include "Project.h"
#include "FilePath.h"
#include "Options.h"
#include "BuildConfigReader.h"
#include "DirList.h"
#include "Xmi2Object.h"
#include "Debug.h"

#define DEBUG_PROJ 0
#if(DEBUG_PROJ)
static DebugFile sProjFile("DebugProj.txt", "w");
static void logProj(char const *str)
    {
    sProjFile.printflush("%s\n", str);
    }
#else
#define logProj(str);
#endif


OovProject::~OovProject()
    {
    }

bool OovProject::newProject(OovString projectDir, CompoundValue const &excludeDirs,
	OovProject::eNewProjectStatus &projStat)
    {
    bool started = isProjectIdle();
    projStat = NP_CantCreateDir;
    if(started)
	{
	if(projectDir.length())
	    {
	    FilePathEnsureLastPathSep(projectDir);
	    if(FileEnsurePathExists(projectDir))
		{
		Project::setProjectDirectory(projectDir);
		gBuildOptions.setFilename(Project::getProjectFilePath());
		gGuiOptions.setFilename(Project::getGuiOptionsFilePath());

		gBuildOptions.setNameValue(OptProjectExcludeDirs, excludeDirs.getAsString(';'));
		if(gBuildOptions.writeFile())
		    {
		    gGuiOptions.writeFile();
		    bool openedProject = false;
		    // Return is discarded because isProjectIdle was checked previously.
		    openProject(projectDir, openedProject);
		    if(openedProject)
			{
			projStat = NP_CreatedProject;
			}
		    }
		else
		    {
		    projStat = NP_CantCreateFile;
		    }
		}
	    else
		{
		projStat = NP_CantCreateDir;
		}
	    }
	}
    return started;
    }

bool OovProject::openProject(OovStringRef projectDir, bool &openedProject)
    {
    bool started = isProjectIdle();
    openedProject = false;
    if(started)
	{
	mProjectStatus.clear();
	Project::setProjectDirectory(projectDir);
	ProjectReader reader;
	if(reader.miniReadOovProject(projectDir))
	    {
	    openedProject = true;
	    }
	mProjectStatus.mProjectOpen = openedProject;
	}
    return started;
    }

bool OovProject::clearAnalysis()
    {
    bool started = isProjectIdle();
    if(started)
	{
	logProj(" clearAnalysis");
	mProjectStatus.mAnalysisStatus = ProjectStatus::AS_UnLoaded;
	mModelData.clear();
	}
    return started;
    }

bool OovProject::loadAnalysisFiles()
    {
    bool started = isProjectIdle();
    if(started)
	{
	logProj("+loadAnalysisFiles");
	stopAndWaitForBackgroundComplete();
	addTask(ProjectBackgroundItem());
	logProj("-loadAnalysisFiles");
	}
    return started;
    }

void OovProject::stopAndWaitForBackgroundComplete()
    {
    logProj("+stopAndWait");
    stopAndWaitForCompletion();
    mBackgroundProc.stopProcess();
    logProj("-stopAndWait");
    }

void OovProject::processAnalysisFiles()
    {
    logProj("+processAnalysisFiles");
    mProjectStatus.mAnalysisStatus |= ProjectStatus::AS_Loading;
    std::vector<std::string> fileNames;
    BuildConfigReader buildConfig;
    bool open = getDirListMatchExt(buildConfig.getAnalysisPath(),
	    FilePath(".xmi", FP_File), fileNames);
    if(open)
	{
	int typeIndex = 0;
	OovTaskStatusListenerId taskId = 0;
	if(mStatusListener)
	    {
	    taskId = mStatusListener->startTask("Loading files.", fileNames.size());
	    }
	for(size_t i=0; i<fileNames.size() && continueProcessingItem(); i++)
	    {
	    OovString fileText = "File ";
	    fileText.appendInt(i);
	    fileText += ": ";
	    fileText += fileNames[i];
	    if(mStatusListener && !mStatusListener->updateProgressIteration(
		    taskId, i, fileText))
		{
		break;
		}
	    File file(fileNames[i], "r");
	    if(file.isOpen())
		{
		loadXmiFile(file.getFp(), mModelData, fileNames[i], typeIndex);
		}
	    }
    logProj(" processAnalysisFiles - loaded");
	if(continueProcessingItem())
	    {
	    if(mStatusListener)
		{
		taskId = mStatusListener->startTask("Resolving Model.", 100);
		}
	    mModelData.resolveModelIds();
    logProj(" processAnalysisFiles - resolved");
	    if(mStatusListener)
		{
		mStatusListener->updateProgressIteration(taskId, 50, nullptr);
		mStatusListener->endTask(taskId);
		}
	    }
	}
    mProjectStatus.mAnalysisStatus |= ProjectStatus::AS_Loaded;
    logProj("-processAnalysisFiles");
    }

bool OovProject::runSrcManager(OovStringRef const buildConfigName,
	OovStringRef const runStr, eSrcManagerOptions smo)
    {
    bool success = true;
    OovString procPath = Project::getBinDirectory();
    procPath += FilePathMakeExeFilename("oovBuilder");
    OovProcessChildArgs args;
    args.addArg(procPath);
    args.addArg(Project::getProjectDirectory());

    switch(smo)
	{
	case SM_Analyze:
	    args.addArg("-mode-analyze");
	    break;

	case SM_Build:
	    args.addArg("-mode-build");
	    args.addArg(makeBuildConfigArgName("-cfg", buildConfigName));
	    break;

	case SM_CovInstr:
	    args.addArg("-mode-cov-instr");
	    args.addArg(makeBuildConfigArgName("-cfg", BuildConfigAnalysis));
	    break;

	case  SM_CovBuild:
	    args.addArg("-mode-cov-build");
	    args.addArg(makeBuildConfigArgName("-cfg", BuildConfigDebug));
	    break;

	case SM_CovStats:
	    args.addArg("-mode-cov-stats");
	    args.addArg(makeBuildConfigArgName("-cfg", BuildConfigDebug));
	    break;
	}
    if(success)
	{
	success = mBackgroundProc.startProcess(procPath, args.getArgv());
	}
    return success;
    }

void OovProject::stopSrcManager()
    {
    mBackgroundProc.childProcessKill();
    }

ProjectStatus OovProject::getProjectStatus()
    {
    mProjectStatus.mBackgroundProcIdle = mBackgroundProc.isIdle();
    mProjectStatus.mBackgroundThreadIdle = !isQueueBusy();
    return mProjectStatus;
    }

