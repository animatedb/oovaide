/*
 * StaticAnalysis.h
 *
 *  Created on: Nov 24, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef STATICANALYSIS_H_
#define STATICANALYSIS_H_

#include "ModelObjects.h"
#include "IncludeMap.h"
#include "Gui.h"

class StaticAnalysis
    {
    public:
        /// This outputs usage counts of all member variables by their containing class.
        static bool createMemberVarUsageFile(GtkWindow *parentWindow,
            ModelData const &modelData, std::string &fn);

        /// This outputs usage counts of all methods called by all other methods.
        static bool createMethodUsageFile(GtkWindow *parentWindow,
                ModelData const &modelData, std::string &fn);

        static bool createIncludeTypeUsageFile(GtkWindow *parentWindow,
            ModelData const &modelData, IncDirDependencyMapReader const &incMap,
            std::string &fn);

        static bool createProjectStats(ModelData const &modelData, std::string &displayStr);

        /// This outputs code and comment lines for all functions and modules.
        static bool createLineStatsFile(ModelData const &modelData, std::string &fn);
    };

#endif /* STATICANALYSIS_H_ */
