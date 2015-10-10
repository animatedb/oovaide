/*
 * NewModule.h
 *
 *  Created on: Oct 9, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */
#include "FilePath.h"

class NewModule
    {
    public:
        bool runDialog();
        FilePath getFileName() const
            { return mFileName; }

    private:
        FilePath mFileName;
        bool createModuleFiles();
    };
