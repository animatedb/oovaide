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
#include "Project.h"


class ComponentPkgDeps
    {
    public:
	void addPkgDep(char const * const compName, char const * const pkgName)
	    { mCompPkgMap[compName].insert(pkgName); }
	bool const isDependent(char const * const compName, char const * const pkgName) const;

    private:
	std::map<std::string, std::set<std::string> > mCompPkgMap;
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
	    mComponentFinder(compFinder)
	    {}
	void build(eProcessModes mode, char const * const incDepsFilePath,
		char const * const buildDirClass);

    private:
	FilePath mSrcRootDir;
	FilePath mOutputPath;
	FilePath mIntermediatePath;
	ComponentFinder &mComponentFinder;
	ToolPathFile mToolPathFile;
	ObjSymbols mObjSymbols;
	IncDirDependencyMapReader mIncDirMap;
	ComponentPkgDeps mComponentPkgDeps;

	void buildComponents();
	void processSourceForComponents(eProcessModes pm);
	void generateDependencies();
	void processSourceFile(eProcessModes pm, const std::string &srcFile,
		const StdStringVec &incDirs, const StdStringVec &incFiles,
		const std::set<std::string> &externPkgCompileArgs);
	void makeLib(const std::string &libName, const StdStringVec &objectFileNames);
	void makeLibSymbols(char const * const clumpName, const StdStringVec &files);
	void makeExe(char const * const compName, const StdStringVec &sources,
		const StdStringVec &projectLibsFilePaths,
		const StdStringVec &externLibDirs,
		const IndexedStringVec &externOrderedLibNames,
		const IndexedStringSet &externPkgLinkArgs,
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
	/// For the specified component, get the library directories and library names
	/// from the external build packages.
	void appendOrderedPackageLibs(char const * const compName,
		StdStringVec &libDirs, IndexedStringVec &sortedLibNames);
	std::set<std::string> getComponentCompileArgs(char const * const compName,
		ComponentTypesFile const &file);
	/// For the specified component, get the link arguments for the external build
	/// packages.
	IndexedStringSet getComponentPackageLinkArgs(char const * const compName,
		ComponentTypesFile const &file);
    };


#endif /* COMPONENTBUILDER_H_ */
