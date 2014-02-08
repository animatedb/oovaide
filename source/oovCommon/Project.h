/*
* Project.h
*
*  Created on: Jun 19, 2013
*  \copyright 2013 DCBlaha.  Distributed under the GPL.
*/

#ifndef PROJECT_H_
#define PROJECT_H_

#include <string>
#include "FilePath.h"
#include "NameValueFile.h"
#include "Packages.h"
#include "OovString.h"

#define OptSourceRootDir "SourceRootDir"
#define OptBaseArgs "BuildArgsBase"

// These are stored per configuration. They will have the build configuration
// name appended.
#define OptExtraBuildArgs "BuildArgsExtra"

// These are stored per configuration. They will have the build configuration
// name appended.
#define OptToolLibPath "Tool-LibPath"
#define OptToolCompilePath "Tool-CompilerPath"
#define OptToolObjSymbolPath "Tool-ObjSymbolPath"

#define BuildConfigAnalysis "Analyze"
#define BuildConfigDebug "Debug"
#define BuildConfigRelease "Release"

std::string makeBuildConfigArgName(char const * const baseName,
	char const * const buildConfig);

/// A project file contains compile flags and drawing parameters.
/// There is also a matching .XMI file for every source file, that only
/// contains content that is needed to display diagrams.
class Project
    {
    public:
	static void setProjectDirectory(char const * const projDir)
	    { sProjectDirectory = projDir; }
	static void setSourceRootDirectory(char const * const projDir)
	    { sSourceRootDirectory = projDir; }
	static const std::string &getProjectDirectory()
	    { return sProjectDirectory; }
	static std::string &getSrcRootDirectory();
	static std::string getComponentTypesFilePath();
	static char const * const getAnalysisIncDepsFilename()
	    { return "oovcde-incdeps.txt"; }
	static char const * const getAnalysisComponentsFilename()
	    { return "oovcde-comps.txt"; }
	static std::string getProjectFilePath();
	static std::string getGuiOptionsFilePath();

	static FilePath getOutputDir(char const * const buildDirClass);
	static FilePath getIntermediateDir(char const * const buildDirClass);

	/// Returns a filename relative to the root source directory.
	static void getSrcRootDirRelativeSrcFileName(const std::string &srcFileName,
		const std::string &srcRootDir, std::string &relSrcFileName);
	/// Make a filename for the compressed content file for each source file.
	static void makeAnalysisFileName(const std::string &srcFileName,
		const std::string &srcRootDir,
		const std::string &analysisDir, std::string &outFileName)
	    {
	    makeOutBaseFileName(srcFileName, srcRootDir, analysisDir, outFileName);
	    outFileName += ".xmi";
	    }
	// This does not have an extension.
	static void makeOutBaseFileName(const std::string &srcFileName,
		const std::string &srcRootDir,
		const std::string &outFilePath, std::string &outFileName);
	// This does not have an extension.
	static void makeTreeOutBaseFileName(const std::string &srcFileName,
		const std::string &srcRootDir,
		const std::string &outFilePath, std::string &outFileName);
	static void replaceChars(std::string &str, char oldC, char newC);
    private:
	static std::string sProjectDirectory;
	static std::string sSourceRootDirectory;
    };

class ProjectReader:public NameValueFile
    {
    public:
	ProjectReader():
	    mProjectPackages(false), mBuildPackages(false)
	    {}
	bool readOovProject(char const * const oovProjectDir,
		char const * const buildConfigName);

	const ProjectPackages &getProjectPackages() const
	    { return mProjectPackages; }
	BuildPackages const &getBuildPackages() const
	    { return mBuildPackages; }
	BuildPackages &getBuildPackages()
	    { return mBuildPackages; }
	const StdStringVec &getCompileArgs() const
	    { return mCompileArgs; }
	const StdStringVec &getLinkArgs() const
	    { return mLinkArgs; }
	const StdStringVec &getExternalArgs() const
	    { return mExternalRootArgs; }
	// These are only used for computing CRC's. The builder will later get
	// the arguments from the packages because not all packages are used by
	// all components.
	const StdStringVec getAllCrcCompileArgs() const;
	const StdStringVec getAllCrcLinkArgs() const;
	static std::string &getSrcRootDirectory()
	    { return Project::getSrcRootDirectory(); }

    private:
	StdStringVec mCompileArgs;
	StdStringVec mLinkArgs;
	StdStringVec mExternalRootArgs;
	StdStringVec mPackageCrcCompileArgs;
	StdStringVec mPackageCrcLinkArgs;
	StdStringVec mPackageCrcNames;
	/// These are chosen by the user and saved for the project.
	ProjectPackages mProjectPackages;
	/// These may be modified by the build and contains project packages and
	/// external root directories.
	BuildPackages mBuildPackages;

	void addCompileArg(char const * const str)
	    { mCompileArgs.push_back(str); }
	void addLinkArg(char const * const str)
	    { mLinkArgs.push_back(str); }
	void addExternalArg(char const * const str)
	    { mExternalRootArgs.push_back(str); }

	void addPackageCrcName(char const * const pkgName)
	    { mPackageCrcNames.push_back(std::string("-EP") + pkgName); }
	void addPackageCrcCompileArg(char const * const str)
	    { mPackageCrcCompileArgs.push_back(str); }
	void addPackageCrcLinkArg(char const * const str)
	    { mPackageCrcLinkArgs.push_back(str); }

	void parseArgs(StdStringVec const &strs);
	void handleExternalPackage(char const * const pkgName);
    };

#endif /* PROJECT_H_ */
