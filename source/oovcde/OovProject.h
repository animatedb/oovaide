/*
 * OovProject.h
 *
 *  Created on: Feb 26, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVPROJECT_H_
#define OOVPROJECT_H_

#include "OovString.h"
#include "NameValueFile.h"
#include "ModelObjects.h"
#include "IncludeMap.h"
#include "Project.h"
#include "Options.h"
#include "OovProcess.h"
#include "OovThreadedBackgroundQueue.h"

class ProjectStatus
    {
    public:
        ProjectStatus()
            { clear(); }
        void clear()
            {
            mProjectOpen = false;
            mBackgroundProcIdle = true;
            mBackgroundThreadIdle = true;
            mAnalysisStatus = AS_NoChange;
            }
        bool operator!=(ProjectStatus const &stat)
            {
            return(mProjectOpen != stat.mProjectOpen ||
                    mBackgroundProcIdle != stat.mBackgroundProcIdle ||
                    mBackgroundThreadIdle != stat.mBackgroundThreadIdle ||
                    mAnalysisStatus != stat.mAnalysisStatus);
            }
        bool isAnalysisReady() const
            {
            return mProjectOpen && mBackgroundProcIdle &&
                    (mAnalysisStatus & ProjectStatus::AS_Loaded);
            }
        bool isIdle() const
            {
            return mBackgroundProcIdle && mBackgroundThreadIdle;
            }

        bool mProjectOpen;      // The component list can be updated.
        /// Making this status signals instead of states allows the GUI to
        /// see that some transitions happened so that it doesn't miss
        /// something going from loaded to unloaded, loading and back to loaded.
        enum eAnalysisStatus { AS_NoChange=0, AS_UnLoaded=0x01,
            AS_Loading=0x02, AS_Loaded=0x04 };
        eAnalysisStatus mAnalysisStatus;
        bool mBackgroundProcIdle;
        bool mBackgroundThreadIdle;
    };

inline ProjectStatus::eAnalysisStatus operator|=(ProjectStatus::eAnalysisStatus &a,
        ProjectStatus::eAnalysisStatus b)
    {
    a = static_cast<ProjectStatus::eAnalysisStatus>(a | b);
    return a;
    }

class ProjectBackgroundItem
    {

    };


/// This is a non-GUI class and is the main background top level class for the
/// Oovcde project.
class OovProject:public ThreadedWorkBackgroundQueue<OovProject, ProjectBackgroundItem>
    {
    public:
        OovProject():
            mStatusListener(nullptr)
            {}
        virtual ~OovProject();

        void setBackgroundProcessListener(OovProcessListener *listener)
            { mBackgroundProc.setListener(listener); }
        void setStatusListener(OovTaskStatusListener *listener)
            { mStatusListener = listener; }

        /// This is provided because this may need to be done before the
        /// destruction.  This stops processes and loading and resolving.
        void stopAndWaitForBackgroundComplete();

        enum eNewProjectStatus { NP_CreatedProject, NP_CantCreateDir,
            NP_CantCreateFile };
        /// Creates a new project directory and initial default configuration
        /// files, and then opens the project.
        /// @return false if analysis is being loaded.
        bool newProject(OovString projectDir, CompoundValue const &excludeDirs,
                eNewProjectStatus &projStatus);

        /// @param openedProj false if the project directory does not contain a project file.
        /// @return false if analysis is being loaded.
        bool openProject(OovStringRef projectDir, bool &openedProj);

        /// @return false if analysis is being loaded.
        bool clearAnalysis();

        /// This loads the files on a backgroundThread. Use getStatus to see
        /// when the loading is complete.
        /// @return false if analysis is being loaded.
        bool loadAnalysisFiles();

        enum eSrcManagerOptions { SM_Analyze, SM_Build, SM_CovInstr, SM_CovBuild, SM_CovStats };
        /// Returns true if process started.
        bool runSrcManager(OovStringRef const buildConfigName,
                OovStringRef const runStr, eSrcManagerOptions smo);
        void stopSrcManager();

        ModelData &getModelData()
            { return mModelData; }
        IncDirDependencyMapReader &getIncMap()
            { return mIncludeMap; }
        ProjectStatus getProjectStatus();
        ProjectReader &getProjectOptions()
            { return mProjectOptions; }
        GuiOptions &getGuiOptions()
            { return mGuiOptions; }

        bool isAnalysisReady()
            {
            return getProjectStatus().isAnalysisReady();
            }
        bool isProjectIdle()
            {
            return(getProjectStatus().isIdle());
            }

        // Called from ThreadedWorkBackgroundQueue
        void processItem(ProjectBackgroundItem const &item)
            { processAnalysisFiles(); }

    private:
        OovTaskStatusListener *mStatusListener;
        ProjectStatus mProjectStatus;
        ProjectReader mProjectOptions;
        GuiOptions mGuiOptions;
        ModelData mModelData;
        IncDirDependencyMapReader mIncludeMap;
        OovBackgroundPipeProcess mBackgroundProc;

        // Called from ThreadedWorkBackgroundQueue through processItem.
        void processAnalysisFiles();
        void loadIncludeMap();
    };


#endif /* OOVPROJECT_H_ */
