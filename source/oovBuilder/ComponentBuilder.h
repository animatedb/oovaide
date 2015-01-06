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
	void addPkgDep(OovString const compName, OovStringRef const pkgName)
	    { mCompPkgMap[compName].insert(pkgName); }
	bool const isDependent(OovStringRef const compName, OovStringRef const pkgName) const;

    private:
	std::map<OovString, OovStringSet > mCompPkgMap;
    };

class ProcessArgs
    {
    public:
        ProcessArgs(OovStringRef const proc, OovStringRef const out,
            const OovProcessChildArgs &args, char const *stdOutFn=""):
            mProcess(proc), mOutputFile(out), mChildArgs(args),
            mStdOutFn(stdOutFn)
            {}
        ProcessArgs()
            {}
        OovString mProcess;
        OovString mOutputFile;
        OovProcessChildArgs mChildArgs;
        OovString mStdOutFn;  // zero length will not use the name
        OovString mLibFilePath;	// Only used for lib symbol processing.
    };

class TaskQueueListener
    {
    public:
	virtual ~TaskQueueListener()
	    {}
	virtual void extraProcessing(bool success, OovStringRef const outFile,
		OovStringRef const stdOutFn, ProcessArgs const &item) = 0;
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
	static bool runProcess(OovStringRef const procPath, OovStringRef const outFile,
		const OovProcessChildArgs &args,
                InProcMutex &mutex, OovStringRef const stdOutFn=nullptr);

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
	void build(eProcessModes mode, OovStringRef const incDepsFilePath,
		OovStringRef const buildDirClass);

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
	void processSourceFile(eProcessModes pm, OovStringRef const srcFile,
		const OovStringVec &incDirs, const OovStringVec &incFiles,
		const OovStringSet &externPkgCompileArgs);
	void makeLib(OovStringRef const libName, const OovStringVec &objectFileNames);
	void makeLibSymbols(OovStringRef const clumpName, OovStringVec const &files);
	void makeExe(OovStringRef const compName, const OovStringVec &sources,
		const OovStringVec &projectLibsFilePaths,
		const OovStringVec &externLibDirs,
		const IndexedStringVec &externOrderedLibNames,
		const IndexedStringSet &externPkgLinkArgs,
		bool shared);
	OovString makeOutputObjectFileName(OovStringRef const str);
	OovString getSymbolBasePath();
	OovString getDiagFileName() const
	    { return(mIntermediatePath + "oovcde-BuildOut.txt"); }
	bool anyIncDirsMatch(OovStringRef const compName,
		RootDirPackage const &pkg);
	void makePackageLibSymbols(OovStringRef const compName);
	void makeOrderedPackageLibs(OovStringRef const compName,
		OovStringVec &libDirs, OovStringVec &sortedLibNames);
	/// For the specified component, get the library directories and library names
	/// from the external build packages.
	void appendOrderedPackageLibs(OovStringRef const compName,
		OovStringVec &libDirs, IndexedStringVec &sortedLibNames);
	OovStringSet getComponentCompileArgs(OovStringRef const compName,
		ComponentTypesFile const &file);
	/// For the specified component, get the link arguments for the external build
	/// packages.
	IndexedStringSet getComponentPackageLinkArgs(OovStringRef const compName,
		ComponentTypesFile const &file);
    };


#endif /* COMPONENTBUILDER_H_ */
