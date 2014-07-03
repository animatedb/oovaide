/*
 * BuildConfigReader.cpp
 *
 *  Created on: Jan 13, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "BuildConfigReader.h"
#include "Project.h"
#include "OovString.h"
#include <stdio.h>	// For snprintf


BuildConfig::BuildConfig()
    {
    mConfigFile.setFilename(getBuildConfigFilename().c_str());
    mConfigFile.readFile();
    }

std::string BuildConfig::getAnalysisPath(char const * const buildType,
	char const * const crcStr) const
    {
    FilePath analysisCrcStr(Project::getProjectDirectory(), FP_Dir);
    std::string dirName = "analysis-";
    dirName += crcStr;
    analysisCrcStr.appendDir(dirName.c_str());
    return analysisCrcStr;
    }

std::string BuildConfig::getAnalysisPath(char const * const buildType) const
    {
    return getAnalysisPath(buildType, getCrcAsStr(buildType, CT_AnalysisArgsCrc).c_str());
    }

std::string BuildConfig::getIncDepsFilePath(char const * const buildType) const
    {
    FilePath fp(getAnalysisPath(buildType).c_str(), FP_Dir);
    fp.appendFile(Project::getAnalysisIncDepsFilename());
    return fp;
    }

std::string BuildConfig::getComponentsFilePath(char const * const buildType) const
    {
    FilePath fp(getAnalysisPath(buildType), FP_Dir);
    fp.appendFile(Project::getAnalysisComponentsFilename());
    return fp;
    }

std::string BuildConfig::getCrcAsStr(char const * const buildType, CrcTypes crcType) const
    {
    OovStringRef const buildStr = mConfigFile.getValue(buildType).c_str();
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
    fp.appendFile("oovcde-buildconfig.txt");
    return fp;
    }


