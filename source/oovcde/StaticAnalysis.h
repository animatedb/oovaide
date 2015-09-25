/*
 * StaticAnalysis.h
 *
 *  Created on: Nov 24, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef STATICANALYSIS_H_
#define STATICANALYSIS_H_

#include "ModelObjects.h"
#include "Gui.h"

/// This outputs usage counts of all member variables by their containing class.
bool createMemberVarUsageStaticAnalysisFile(GtkWindow *parentWindow,
        ModelData const &modelData, std::string &fn);

/// This outputs usage counts of all methods called by all other methods.
bool createMethodUsageStaticAnalysisFile(GtkWindow *parentWindow,
        ModelData const &modelData, std::string &fn);

bool createProjectStats(ModelData const &modelData, std::string &displayStr);
/// This outputs code and comment lines for all functions and modules.
bool createLineStatsFile(ModelData const &modelData, std::string &fn);

#endif /* STATICANALYSIS_H_ */
