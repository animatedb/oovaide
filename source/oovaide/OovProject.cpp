/*
 * OovProject.cpp
 *
 *  Created on: Feb 26, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "OovProject.h"
#include "Project.h"
#include "ProjectSettingsDialog.h"
#include "FilePath.h"
#include "Options.h"
#include "BuildConfigReader.h"
#include "DirList.h"
#include "Xmi2Object.h"
#include "Debug.h"
#include "OovError.h"

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
            OovStatus status = FileEnsurePathExists(projectDir);
            if(status.ok())
                {
                Project::setProjectDirectory(projectDir);
                // The new project dialog already clears the options.
                // The options cannot be cleared here or the src root dir is also cleared.
/*
                OptionsDefaults optionDefaults(mProjectOptions);
                optionDefaults.setDefaultOptions();
                getProjectOptions().setNameValue(OptSourceRootDir, rootSrcText);
*/
                mGuiOptions.setDefaultOptions();

                mProjectOptions.setFilename(Project::getProjectFilePath());
                mGuiOptions.setFilename(Project::getGuiOptionsFilePath());

                mProjectOptions.setNameValue(OptProjectExcludeDirs, excludeDirs.getAsString(';'));
                status = mProjectOptions.writeFile();
                if(status.ok())
                    {
                    status = mGuiOptions.writeFile();
                    if(!status.ok())
                        {
                        OovString errStr = "Unable to write GUI options file: ";
                        errStr += mGuiOptions.getFilename();
                        status.report(ET_Error, errStr);
                        }
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

static OovStringVec getExternalProjectPackages(ProjectReader const &projOptions)
    {
    OovStringVec cppArgVars = projOptions.getMatchingNames("CppArgs");
    OovStringVec extPackageNames;
    for(auto const &cppArg : cppArgVars)
        {
        BuildVariable buildVar;
        buildVar.initVarFromString(cppArg, "");
        OovString platform = buildVar.getFilterValue(OptFilterNamePlatform);
        if(platform.length() == 0 || OptionsDefaults::getPlatform() == platform)
            {
            OovString cppArgStr = projOptions.getValue(cppArg);
            CompoundValue compArgs;
            compArgs.parseString(cppArgStr);
            for(auto const &arg : compArgs)
                {
                size_t pos = arg.find("-EP");
                if(pos != std::string::npos)
                    {
                    extPackageNames.push_back(arg.substr(pos+3));
                    }
                }
            }
        }
    return extPackageNames;
    }

bool OovProject::openProject(OovStringRef projectDir, bool &openedProject)
    {
    bool started = isProjectIdle();
    openedProject = false;
    OovStatus status(true, SC_File);
    if(started)
        {
        mProjectStatus.clear();
        Project::setProjectDirectory(projectDir);
        status = mProjectOptions.readProject(projectDir);
        if(status.ok())
            {
            if(FileIsDirOnDisk(Project::getSourceRootDirectory(), status))
                {
                openedProject = true;
                }
            else
                {
                Gui::messageBox("Unable to find source directory");
                ProjectSettingsDialog dlg(Gui::getMainWindow(),
                    getProjectOptions(), getGuiOptions(),
                    ProjectSettingsDialog::PS_OpenProjectEditSource);
                if(dlg.runDialog())
                    {
                    status = mProjectOptions.writeFile();
                    openedProject = true;
                    }
                }
            if(openedProject)
                {
                OovStringVec extPackageNames = getExternalProjectPackages(
                    mProjectOptions);
                updateProjectPackages(extPackageNames);
                status = mGuiOptions.read();
                if(status.needReport())
                    {
                    mGuiOptions.setDefaultOptions();
                    status.report(ET_Error,
                        "Unable to read GUI options for project, using defaults");
                    }
                }
            }
        mProjectStatus.mProjectOpen = openedProject;
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to open project");
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
        loadIncludeMap();
        addTask(ProjectBackgroundItem());
        logProj("-loadAnalysisFiles");
        }
    return started;
    }

void OovProject::loadIncludeMap()
    {
    BuildConfigReader buildConfig;
    std::string incDepsFilePath = buildConfig.getIncDepsFilePath();
    mIncludeMap.read(incDepsFilePath);
    }

void OovProject::stopAndWaitForBackgroundComplete()
    {
    logProj("+stopAndWait");
    // This sets a flag in ThreadedWorkBackgroundQueue that will stop the
    // background thread and abort loops in processAnalysisFiles().
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
    OovStatus status = getDirListMatchExt(buildConfig.getAnalysisPath(),
        FilePath(".xmi", FP_File), fileNames);
    if(status.ok())
        {
        int typeIndex = 0;
        OovTaskStatusListenerId taskId = 0;
        if(mStatusListener)
            {
            taskId = mStatusListener->startTask("Loading files.", fileNames.size());
            }
        // The continueProcessingItem is from the ThreadedWorkBackgroundQueue,
        // and is set false when stopAndWaitForCompletion() is called.
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
            File file;
            OovStatus status = file.open(fileNames[i], "r");
            if(status.ok())
                {
                loadXmiFile(file, mModelData, fileNames[i], typeIndex);
                }
            if(status.needReport())
                {
                OovString err = "Unable to read XMI file ";
                err += fileNames[i];
                status.report(ET_Error, err);
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
    if(status.needReport())
        {
        status.reported();
// The analysis directory is optional.
//        status.report(ET_Error, "Unable to get directory for analysis");
        }
    mProjectStatus.mAnalysisStatus |= ProjectStatus::AS_Loaded;
    logProj("-processAnalysisFiles");
    }

static OovString makeBuildConfigArgName(OovStringRef const baseName,
        OovStringRef const buildConfig)
    {
    OovString name = baseName;
    name += '-';
    name += buildConfig;
    return name;
    }

bool OovProject::runSrcManager(OovStringRef const buildConfigName,
        OovStringRef const runStr, eProcessModes pm)
    {
    bool success = true;
    OovString procPath = Project::getBinDirectory();
    procPath += FilePathMakeExeFilename("oovBuilder");
    OovProcessChildArgs args;
    args.addArg(procPath);
    args.addArg(Project::getProjectDirectory());

    switch(pm)
        {
        case PM_Analyze:
            args.addArg("-mode-analyze");
            break;

        case PM_Build:
            args.addArg("-mode-build");
            args.addArg(makeBuildConfigArgName("-cfg", buildConfigName));
            break;

        case PM_CovInstr:
            args.addArg("-mode-cov-instr");
            args.addArg(makeBuildConfigArgName("-cfg", BuildConfigAnalysis));
            break;

        case PM_CovBuild:
            args.addArg("-mode-cov-build");
            args.addArg(makeBuildConfigArgName("-cfg", BuildConfigDebug));
            break;

        case PM_CovStats:
            args.addArg("-mode-cov-stats");
            args.addArg(makeBuildConfigArgName("-cfg", BuildConfigDebug));
            break;

        default:
            if(pm & PM_CleanMask)
                {
                OovString cleanArg = "-mode-clean-";
                if(pm & PM_CleanAnalyze)
                    {
                    cleanArg += "a";
                    }
                if(pm & PM_CleanBuild)
                    {
                    cleanArg += "b";
                    }
                if(pm & PM_CleanCoverage)
                    {
                    cleanArg += "c";
                    }
                args.addArg(cleanArg);
                args.addArg(makeBuildConfigArgName("-cfg", BuildConfigDebug));
                }
            break;
        }
    if(success)
        {
        success = mBackgroundProc.startProcess(procPath, args.getArgv(), false);
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

