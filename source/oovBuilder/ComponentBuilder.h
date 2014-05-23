/*
 * ComponentBuilder.h
 *
 *  Created on: Sep 4, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTBUILDER_H_
#define COMPONENTBUILDER_H_

#include "ComponentFinder.h"
#include "ObjSymbols.h"
#include "OovThreadedQueue.h"


class ComponentPkgDeps
    {
    public:
	void addPkgDep(char const * const compName, char const * const pkgName)
	    { mCompPkgMap[compName].insert(pkgName); }
	bool const isDependent(char const * const compName, char const * const pkgName) const;

    private:
	std::map<std::string, std::set<std::string> > mCompPkgMap;
    };

class ToolPathFile:public NameValueFile
    {
    public:
	void setConfig(char const * const buildConfig)
	    {
	    mBuildConfig = buildConfig;
	    }
	std::string getCompilerPath();
	std::string getLibberPath();
	std::string getObjSymbolPath();
    private:
	std::string mBuildConfig;
	std::string mPathLibber;
	std::string mPathCompiler;
	std::string mPathObjSymbol;
	void getPaths();
    };


class ProcessArgs
    {
    public:
        ProcessArgs(char const *proc, char const *out,
            const OovProcessChildArgs &args, char const *stdOutFn=""):
            mProcess(proc), mOutputFile(out), mChildArgs(args),
            mStdOutFn(stdOutFn)
            {}
        ProcessArgs()
            {}
        std::string mProcess;
        std::string mOutputFile;
        OovProcessChildArgs mChildArgs;
        std::string mStdOutFn;  // zero length will not use the name
        std::string mLibFilePath;	// Only used for lib symbol processing.
    };

class TaskQueueListener
    {
    public:
	virtual ~TaskQueueListener()
	    {}
	virtual void extraProcessing(bool success, char const * const outFile,
		char const * const stdOutFn, ProcessArgs const &item) = 0;
    };

class ComponentTaskQueue:public ThreadedWorkQueue<ProcessArgs, ComponentTaskQueue>
    {
    public:
	ComponentTaskQueue():
	    mListener(nullptr)
	    {}
	// Set to nullptr to remove listener
	void setTaskListener(TaskQueueListener *listener)
	    { mListener = listener; }

	// Called by ThreadedWorkQueue
        bool processItem(ProcessArgs const &item);

	/// @param outFile - used only to make an output directory, and display error.
	static bool runProcess(char const * const procPath, char const * const outFile,
		const OovProcessChildArgs &args,
                InProcMutex &mutex, char const * const stdOutFn=nullptr);

        InProcMutex mListenerStdMutex;
    private:
        TaskQueueListener *mListener;
    };

// Builds components. This recursively compiles source files
// into object files.
class ComponentBuilder:public ComponentTaskQueue
    {
    public:
	ComponentBuilder(ComponentFinder &compFinder):
	    mSrcRootDir(nullptr), mComponentFinder(compFinder)
	    {}
	void build(char const * const srcRootDir,
		char const * const incDepsFilePath, char const * const buildDirClass);

    private:
	char const * mSrcRootDir;
	FilePath mOutputPath;
	FilePath mIntermediatePath;
	ComponentFinder &mComponentFinder;
	ToolPathFile mToolPathFile;
	ObjSymbols mObjSymbols;
	IncDirDependencyMapReader mIncDirMap;
	ComponentPkgDeps mComponentPkgDeps;

	void buildComponents();
	void makeObj(const std::string &srcFile,
		const std::vector<std::string> &incDirs,
		const std::vector<std::string> &incFiles,
		const std::set<std::string> &externPkgCompileArgs);
	void makeLib(const std::string &libName,
		const std::vector<std::string> &objectFileNames);
	void makeLibSymbols(char const * const clumpName,
		const std::vector<std::string> &files);
	void makeExe(char const * const compName,
		const std::vector<std::string> &sources,
		const std::vector<std::string> &projectLibsFilePaths,
		const std::vector<std::string> &externLibDirs,
		const std::vector<std::string> &externOrderedLibNames,
		const std::set<std::string> &externPkgLinkArgs,
		bool shared);
	std::string makeOutputObjectFileName(char const * const str);
	std::string getSymbolBasePath();
	std::string getDiagFileName() const
	    { return(mIntermediatePath + "oovcde-BuildOut.txt"); }
	bool anyIncDirsMatch(char const * const compName,
		RootDirPackage const &pkg);
	void makePackageLibSymbols(char const * const compName);
	void makeOrderedPackageLibs(char const * const compName,
		std::vector<std::string> &libDirs, std::vector<std::string> &sortedLibNames);
	void appendOrderedPackageLibs(char const * const compName,
		std::vector<std::string> &libDirs, std::vector<std::string> &sortedLibNames);
	std::set<std::string> getComponentCompileArgs(char const * const compName,
		ComponentTypesFile const &file);
	std::set<std::string> getComponentLinkArgs(char const * const compName,
		ComponentTypesFile const &file);
    };


#endif /* COMPONENTBUILDER_H_ */
