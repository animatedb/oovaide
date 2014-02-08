/*
 * BuildConfigWriter.h
 *
 *  Created on: Jan 13, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef BUILDCONFIGWRITER_H_
#define BUILDCONFIGWRITER_H_

#include "BuildConfigReader.h"


class BuildConfigStrings
    {
    public:
	std::string mExternalConfig;
	std::string mProjectConfig;
	std::string mLinkArgsConfig;
	std::string mOtherArgsConfig;

	std::string getCrcAsStr(BuildConfig::CrcTypes crcType) const;
    };

class BuildConfigWriter:public BuildConfig
    {
    public:
	void setInitialConfig(char const * const buildConfigType,
		std::vector<std::string> const &extArgs,
		std::vector<std::string> const &cppArgs,
		std::vector<std::string> const &linkArgs);
	void setProjectConfig(std::vector<std::string> const &projIncs);

	bool isAnyConfigDifferent(char const * const buildType) const;
	bool isConfigDifferent(char const * const buildType,
		BuildConfig::CrcTypes crcType) const;
	void getUnusedCrcs(std::vector<std::string> &unusedCrcs) const;
	void saveConfig(char const * const buildType);

    private:
	BuildConfigStrings mNewBuildConfigStrings;
	std::string mBuildConfigType;
    };

#endif /* BUILDCONFIGWRITER_H_ */
