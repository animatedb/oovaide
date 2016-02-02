/*
 * NewModule.h
 *
 *  Created on: Oct 9, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */
#include "FilePath.h"
#include "Project.h"

class NewModule
    {
    public:
        NewModule(ProjectReader &project):
            mProject(project)
            {}
        bool runDialog();
        FilePath getFileName() const
            { return mFileName; }

    private:
        ProjectReader mProject;
        FilePath mFileName;
        bool createModuleFiles();
    };
