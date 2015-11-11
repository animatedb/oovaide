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
#include "OovThreadedWaitQueue.h"
#include "Project.h"


class ComponentPkgDeps
    {
    public:
        void addPkgDep(OovString const compName, OovStringRef const pkgName)
            { mCompPkgMap[compName].insert(pkgName); }
        bool isDependent(OovStringRef const compName, OovStringRef const pkgName) const;

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
        OovString mWorkingDir;      // zero length means no working directory.
        OovString mOutputFile;
        OovProcessChildArgs mChildArgs;
        OovString mStdOutFn;  // zero length will not use the name
        OovString mLibFilePath; // Only used for lib symbol processing.
    };

class TaskQueueListener
    {
    public:
        virtual ~TaskQueueListener();
        virtual void extraProcessing(bool success, OovStringRef const outFile,
                OovStringRef const stdOutFn, ProcessArgs const &item) = 0;
    };

class ComponentTaskQueue:public ThreadedWorkWaitQueue<ProcessArgs, ComponentTaskQueue>
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
            const OovProcessChildArgs &args, InProcMutex &mutex,
            OovStringRef const stdOutFn=nullptr, OovStringRef const workingDir=nullptr);

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
        /// A map of all packages required to build each component.
        ComponentPkgDeps mComponentPkgDeps;

        const ComponentTypesFile &getComponentTypesFile() const
            { return mComponentFinder.getComponentTypesFile(); }
        void buildComponents();
        void processSourceForComponents(eProcessModes pm);
        /// Saves a map of all packages required to build each component.
        /// Goes through all non-unknown components in the project and searches
        /// the include paths to see if any came from any of the packages. The
        /// map that is saved is mComponentPkgDeps.
        void generateDependencies();
        void processCppSourceFile(eProcessModes pm, OovStringRef const srcFile,
            const OovStringVec &incDirs, const OovStringVec &incFiles,
            const OovStringSet &externPkgCompileArgs);

        /// This uses the javac program to create class files from java files.
        ///
        /// @param pm At the moment, this does not perform coverage.
        /// @param compName This is used to find directories.
        /// @param javaSources This is typically all source files for the component
        ///     from multiple directories that are defined in the component.
        /// @param externPkgCompileArgs
        ///
        /// javac creates the output files relative to the -d output directory,
        /// but appends the package name from inside the java file. This function
        /// uses the intermediate directory plus the component name to create
        /// the output directory.
        ///
        /// The -cp flag can be used for javac to include a jar, but this function
        /// requires that the user must specify the classpath as an environment
        /// variable.  It would be nice to fix this in the future so the
        /// jar dependencies are resolved by adding -cp for supplier jars.
        void processJavaSourceFiles(eProcessModes pm, OovStringRef compName,
            OovStringVec javaSources /*, const OovStringSet &externPkgCompileArgs*/);

        void makeLib(OovStringRef const libName, const OovStringVec &objectFileNames);
        void makeLibSymbols(OovStringRef const clumpName, OovStringVec const &files);

        void makeExe(OovStringRef const compName, const OovStringVec &sources,
            const OovStringVec &projectLibsFilePaths,
            const OovStringVec &externLibDirs,
            const IndexedStringVec &externOrderedLibNames,
            const IndexedStringSet &externPkgLinkArgs,
            bool shared);

        /// This creates a jar if the output file is older than the input files.
        /// All library jars in the project are passed to the jar command.
        /// @param compName The name of the component is used to name the output
        ///     jar and the output directory.
        /// @param sources The source java files are used to determine where the
        ///     class files are that are used to create the jar.
        /// @param prog If true, then the jar libs in the project are used
        ///     while building the program jar. A Manifest.txt file is required
        ///     to be in the source directory if prog is true.
        void makeJar(OovStringRef const compName, OovStringVec const &sources,
            bool prog);


        /// Returns the absolute path
        OovString makeOutputObjectFileName(OovStringRef const str);

        /// Returns the absolute path
        /// @param compName The component name.
        OovString makeOutputJarName(OovStringRef const compName)
            { return ComponentTypesFile::getComponentFileName(mOutputPath,
                compName, "jar"); }

        OovString makeLibFn(OovStringRef const compName)
            { return ComponentTypesFile::getComponentFileName(mOutputPath,
                compName, "lib", "a"); }


        OovString getSymbolBasePath();
        OovString getDiagFileName() const
            { return(mIntermediatePath + "oovaide-BuildOut.txt"); }
        bool anyIncDirsMatch(OovStringRef const compName,
                RootDirPackage const &pkg);
        void makeOrderedPackageLibs(OovStringRef const compName);
        /// For the specified component, get the library directories and library names
        /// from the external build packages.
        void appendOrderedPackageLibs(OovStringRef const compName,
                OovStringVec &libDirs, IndexedStringVec &sortedLibNames);
        /// file arg is not used.
        OovStringSet getComponentCompileArgs(OovStringRef const compName,
                ComponentTypesFile const & /*file*/);
        /// For the specified component, get the link arguments for the external build
        /// packages.
        IndexedStringSet getComponentPackageLinkArgs(OovStringRef const compName,
                ComponentTypesFile const &file);
    };


#endif /* COMPONENTBUILDER_H_ */
