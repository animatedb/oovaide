/*
 * BuildConfigReader.cpp
 *
 *  Created on: Jan 13, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "BuildConfigReader.h"
#include "Project.h"
#include "OovString.h"
#include <stdio.h>      // For snprintf


BuildConfig::BuildConfig()
    {
    mConfigFile.setFilename(getBuildConfigFilename());
    mConfigFile.readFile();
    }

std::string BuildConfig::getAnalysisPathUsingCRC(OovStringRef const crcStr) const
    {
    FilePath analysisCrcStr(Project::getProjectDirectory(), FP_Dir);
    OovString dirName = "analysis-";
    dirName += crcStr;
    analysisCrcStr.appendDir(dirName);
    return analysisCrcStr;
    }

std::string BuildConfig::getAnalysisPath() const
    {
    // If a build is being performed, it is possible that an analysis was never
    // done, so there will be no analysis string. So get the one from the
    // debug or release build.  They should normally be all the same since
    // it is mainly a CRC of the project related paths.
    OovString crcStr = getCrcAsStr(BuildConfigAnalysis, CT_AnalysisArgsCrc);
    if(crcStr.length() == 0)
        {
        crcStr = getCrcAsStr(BuildConfigDebug, CT_AnalysisArgsCrc);
        if(crcStr.length() == 0)
            {
            crcStr = getCrcAsStr(BuildConfigRelease, CT_AnalysisArgsCrc);
            }
        }
    return getAnalysisPathUsingCRC(crcStr);
    }

std::string BuildConfig::getIncDepsFilePath() const
    {
    FilePath fp(getAnalysisPath(), FP_Dir);
    fp.appendFile(Project::getAnalysisIncDepsFilename());
    return fp;
    }

/*
DEAD CODE
std::string BuildConfig::getComponentsFilePath() const
    {
    FilePath fp(getAnalysisPath(), FP_Dir);
    fp.appendFile(Project::getAnalysisComponentsFilename());
    return fp;
    }
*/

std::string BuildConfig::getCrcAsStr(OovStringRef const buildType, CrcTypes crcType) const
    {
    OovString buildStr = mConfigFile.getValue(buildType);
    std::vector<OovString> crcStrs = buildStr.split(';');
    std::string crcStr;
    if(crcStrs.size() == CT_LastCrc+1)
        {
        crcStr = crcStrs[crcType];
        }
    return crcStr;
    }

std::string BuildConfig::getBuildConfigFilename() const
    {
    FilePath fp(Project::getProjectDirectory(), FP_Dir);
    fp.appendFile("oovcde-tmp-buildconfig.txt");
    return fp;
    }


