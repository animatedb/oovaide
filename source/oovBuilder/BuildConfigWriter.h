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
        OovString mExternalConfig;
        OovString mProjectConfig;
        OovString mLinkArgsConfig;
        OovString mOtherArgsConfig;

        OovString getCrcAsStr(BuildConfig::CrcTypes crcType) const;
    };

class BuildConfigWriter:public BuildConfig
    {
    public:
        void setInitialConfig(OovStringRef const buildConfigType,
                OovStringVec const &extArgs,
                OovStringVec const &cppArgs,
                OovStringVec const &linkArgs);
        void setProjectConfig(OovStringVec const &projIncs);

        bool isAnyConfigDifferent(OovStringRef const buildType) const;
        bool isConfigDifferent(OovStringRef const buildType,
                BuildConfig::CrcTypes crcType) const;
        void getUnusedCrcs(OovStringVec &unusedCrcs) const;
        void saveConfig(OovStringRef const buildType);

    private:
        BuildConfigStrings mNewBuildConfigStrings;
        OovString mBuildConfigType;
    };

#endif /* BUILDCONFIGWRITER_H_ */
