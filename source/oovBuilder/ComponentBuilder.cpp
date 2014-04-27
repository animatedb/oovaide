/*
 * ComponentBuilder.cpp
 *
 *  Created on: Sep 4, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ComponentBuilder.h"
#include "srcFileParser.h"
#include "Project.h"
#include "File.h"
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>	// for getenv
#include <algorithm>
#include "ObjSymbols.h"

bool ComponentBuilder::build(char const * const srcRootDir,
	char const * const incDepsFilePath,
	char const * const buildDirClass)
    {
    mSrcRootDir = srcRootDir;

    mOutputPath.setPath(Project::getOutputDir(buildDirClass).c_str(), FP_Dir);
    mIntermediatePath.setPath(Project::getIntermediateDir(buildDirClass).c_str(), FP_Dir);

    mToolPathFile.setConfig(buildDirClass);

    deleteFile(getDiagFileName().c_str());
    mIncDirMap.read(incDepsFilePath);
    return buildComponents();
    }

bool const ComponentPkgDeps::isDependent(char const * const compName,
	char const * const pkgName) const
    {
    bool dependent = false;
    auto const &mapIt = mCompPkgMap.find(compName);
    if(mapIt != mCompPkgMap.end())
	{
	auto const &pkgSet = mapIt->second;
	dependent = (pkgSet.find(pkgName) != pkgSet.end());
	}
    return dependent;
    }


// libNames = all libs from -EP, -ER and -l
bool ComponentBuilder::anyIncDirsMatch(char const * const compName,
	RootDirPackage const &pkg)
    {
    std::string compDir = mComponentFinder.getComponentTypesFile().
	    getComponentAbsolutePath(compName);

    std::vector<std::string> incRoots = pkg.getIncludeDirs();
    return mIncDirMap.anyRootDirsMatch(incRoots, compDir.c_str());
    }

void ComponentBuilder::appendOrderedPackageLibs(char const * const compName,
	std::vector<std::string> &libDirs, std::vector<std::string> &sortedLibNames)
    {
    BuildPackages &buildPackages = mComponentFinder.getProject().getBuildPackages();
    std::vector<Package> packages = buildPackages.getPackages();
// This probably shouldn't be done since it does not include external packages.
// Just leave them in the original order specified.
//	    std::reverse(packages.begin(), packages.end());
    /// @todo - it might be nice to only make symbols
    /// at the time that a component needs the symbols.
    bool didAnything = false;
    for(auto &pkg : packages)
	{
	if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName().c_str()))
	    {
	    if(!pkg.areLibraryNamesOrdered())
		{
		didAnything = true;
		std::vector<std::string> libNames = pkg.getScannedLibraryFilePaths();
		makeLibSymbols(pkg.getPkgName().c_str(), libNames);
		mObjSymbols.appendOrderedLibs(pkg.getPkgName().c_str(),
			getSymbolBasePath().c_str(), libDirs, sortedLibNames);

		pkg.setOrderedLibs(libDirs, sortedLibNames);
		buildPackages.insertPackage(pkg);
		}
	    else
		{
		std::vector<std::string> pkgLibs = pkg.getLibraryNames();
		std::copy(pkgLibs.begin(), pkgLibs.end(), std::back_inserter(sortedLibNames));

		std::vector<std::string> pkgLibDirs = pkg.getLibraryDirs();
		std::copy(pkgLibDirs.begin(), pkgLibDirs.end(), std::back_inserter(libDirs));
		}
	    }
	}
    if(didAnything)
	buildPackages.savePackages();
    }

std::set<std::string> ComponentBuilder::getComponentCompileArgs(char const * const compName)
    {
    std::set<std::string> compileArgs;
    BuildPackages &buildPackages = mComponentFinder.getProject().getBuildPackages();
    for(auto const &pkg : buildPackages.getPackages())
	{
	if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName().c_str()))
	    {
	    for(auto const &arg : pkg.getCompileArgs())
		{
		compileArgs.insert(arg);
		}
	    }
	}
    return compileArgs;
    }

std::set<std::string> ComponentBuilder::getComponentLinkArgs(char const * const compName)
    {
    std::set<std::string> linkArgs;
    BuildPackages &buildPackages = mComponentFinder.getProject().getBuildPackages();
    for(auto const &pkg : buildPackages.getPackages())
	{
	if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName().c_str()))
	    {
	    for(auto const &arg : pkg.getLinkArgs())
		{
		linkArgs.insert(arg);
		}
	    }
	}
    return linkArgs;
    }


// GCC-4.3 has c++0x
// LLVM 3.2 has c++0x and experimental binaries for mingw32/x86
// LLVM 3.3 has c++11
// G++ 4.7.2 and clang3.2 may work with -stdlib=libstdc++

//
// Some points about compilation:
//   - Both clang and gnu/mingw compilers have built-in include path parsing.
//   The interesting point is that if include paths to mingw are specified on
//   the command line, then the compile will fail when the wrong "string" file
//   is found. It is best to discard any mingw directory include paths, and let
//   the compiler do its own thing.
//
//   - Some #includes such as "gtk/gtk.h" must save the include path as the
//   parent directory location, so these cannot be merely found by looking
//   at the include file name.
//
//   - For optimized dependency checking, there is usually no need to check
//   file dependencies of files outside of the project,
//
// Compilers find mingw directories without passing as -I args
//	http://www.mingw.org/wiki/IncludePathHOWTO - may use mingw path as base?
//
// clang user manual says it expects the following directories for mingw32. 64 uses path?
//	    C:/mingw/include
//	    C:/mingw/lib
//	    C:/mingw/lib/gcc/mingw32/4.[3-5].0/include/c++
//
// http://stackoverflow.com/questions/4779759/how-to-determine-inter-library-dependencies
// ldd utility lists the dynamic dependencies of executable files or shared objects
// C:\Program Files\GTK+-Bundle-3.6.1\lib	(contains glib, gmodule stuff on Windows)
bool ComponentBuilder::buildComponents()
    {
    bool success = false;
    ComponentTypesFile const &compTypesFile =
	    mComponentFinder.getComponentTypesFile();
    std::vector<std::string> compNames = compTypesFile.getComponentNames();
    BuildPackages const &buildPackages =
	    mComponentFinder.getProject().getBuildPackages();

    // Find component dependencies
    for(const auto &compName : compNames)
	{
	/// @todo - this should be optimized so that only components with source files
	/// that have changed include dependencies are processed.
	// This is done for libraries too because they may need compile switches.
	if(compTypesFile.getComponentType(compName.c_str()) != ComponentTypesFile::CT_Unknown)
	    {
	    for(auto const &pkg : buildPackages.getPackages())
		{
		if(anyIncDirsMatch(compName.c_str(), pkg))
		    {
		    mComponentPkgDeps.addPkgDep(compName.c_str(), pkg.getPkgName().c_str());
		    }
		}
	    }
	}

    // Compile all objects.
    for(const auto &name : compNames)
	{
	std::set<std::string> compileArgs = getComponentCompileArgs(name.c_str());
	if(compTypesFile.getComponentType(name.c_str()) != ComponentTypesFile::CT_Unknown)
	    {
	    std::vector<std::string> sources =
		    compTypesFile.getComponentSources(name.c_str());

	    std::vector<std::string> orderedIncRoots = mComponentFinder.getAllIncludeDirs();
	    for(const auto &src : sources)
		{
		FilePath absSrc;
		absSrc.getAbsolutePath(src.c_str(), FP_File);
		std::vector<std::string> incDirs =
			mIncDirMap.getOrderedIncludeDirsForSourceFile(absSrc.c_str(),
			orderedIncRoots);
		/// @todo - this could be optimized to not check file times of external files.
		/// @todo - more optimization could use the times in the incdeps file.
		std::set<IncludedPath> incFilesSet;
		mIncDirMap.getNestedIncludeFilesUsedBySourceFile(absSrc.c_str(), incFilesSet);
		std::vector<std::string> incFiles;
		for(auto const &file : incFilesSet)
		    {
		    incFiles.push_back(file.getFullPath());
		    }
		success = makeObj(src, incDirs, incFiles, compileArgs);
		if(!success)
		    break;
		}
	    }
	}
    // Build all project libraries
    std::vector<std::string> libFileNames;
    bool builtAnyLib = false;
    for(const auto &name : compNames)
	{
	if(compTypesFile.getComponentType(name.c_str()) == ComponentTypesFile::CT_StaticLib)
	    {
	    std::vector<std::string> sources =
		    compTypesFile.getComponentSources(name.c_str());
	    for(size_t i=0; i<sources.size(); i++)
		{
		sources[i] = makeOutputObjectFileName(sources[i].c_str());
		}
	    std::string outFileName;
	    bool builtLib = false;
	    success = makeLib(name, sources, outFileName, builtLib);
	    if(success)
		{
		if(builtLib)
		    builtAnyLib = true;
		libFileNames.push_back(outFileName);
		}
	    }
	}
    if(success && builtAnyLib)
	{
	mObjSymbols.forceUpdate(true);
	makeLibSymbols("ProjLibs", libFileNames);
	mObjSymbols.forceUpdate(false);
	}
    // Build programs
    std::vector<std::string> projectLibFileNames;
    std::vector<std::string> externalLibDirs; 	// not in library search order, eliminate dups.
    std::vector<std::string> externalOrderedLibNames;
    mObjSymbols.appendOrderedLibFileNames("ProjLibs", getSymbolBasePath().c_str(),
	    projectLibFileNames);
    int projectLibNameSize = externalOrderedLibNames.size();
    int projectLibDirSize = externalLibDirs.size();
    for(const auto &name : compNames)
	{
	externalOrderedLibNames.resize(projectLibNameSize);
	externalLibDirs.resize(projectLibDirSize);
	bool shared = (compTypesFile.getComponentType(name.c_str()) ==
		ComponentTypesFile::CT_SharedLib);
	if(compTypesFile.getComponentType(name.c_str()) ==
		ComponentTypesFile::CT_Program || shared)
	    {
	    appendOrderedPackageLibs(name.c_str(), externalLibDirs, externalOrderedLibNames);
	    std::set<std::string> linkArgs = getComponentLinkArgs(name.c_str());

	    std::vector<std::string> sources =
		compTypesFile.getComponentSources(name.c_str());
	    success = makeExe(name.c_str(), sources, projectLibFileNames,
		    externalLibDirs, externalOrderedLibNames,
		    linkArgs, shared);
	    }
	}
    return success;
    }


void ToolPathFile::getPaths()
    {
    if(mPathCompiler.length() == 0)
	{
	std::string projFileName = Project::getProjectFilePath();
	setFilename(projFileName.c_str());
	readFile();

	std::string optStr = makeBuildConfigArgName(OptToolLibPath, mBuildConfig.c_str());
	mPathLibber = getValue(optStr.c_str());
	optStr = makeBuildConfigArgName(OptToolCompilePath, mBuildConfig.c_str());
	mPathCompiler = getValue(optStr.c_str());
	optStr = makeBuildConfigArgName(OptToolObjSymbolPath, mBuildConfig.c_str());
	mPathObjSymbol = getValue(optStr.c_str());
	}
    }

std::string ToolPathFile::getCompilerPath()
    {
    getPaths();
    return(mPathCompiler);
    }

std::string ToolPathFile::getObjSymbolPath()
    {
    getPaths();
    return(mPathObjSymbol);
    }

std::string ToolPathFile::getLibberPath()
    {
    getPaths();
    return(mPathLibber);
    }

bool ComponentBuilder::runProcess(char const * const procPath,
	char const * const outFile, const OovProcessChildArgs &args,
	char const * const stdOutFn)
    {
    FilePath outDir(outFile, FP_File);
    outDir.discardFilename();
    bool success = ensurePathExists(outDir.c_str());
    if(success)
	{
	printf("oovBuilder Building: %s\n", outFile);
	File stdoutFile;
	OovProcessStdListener listener;
	if(stdOutFn)
	    {
	    // The ar tool must send its output to a file.
	    stdoutFile.open(stdOutFn, "a");
	    listener.setStdOut(stdoutFile.getFp(), OovProcessStdListener::OP_OutputFile);
	    }
	else if(sLog.isOpen())
	    {
	    listener.setErrOut(sLog.getFp(), OovProcessStdListener::OP_OutputStdAndFile);
	    listener.setStdOut(sLog.getFp(), OovProcessStdListener::OP_OutputStdAndFile);
	    }
	int exitCode;
	OovPipeProcess pipeProc;
	success = pipeProc.spawn(procPath, args.getArgv(), listener, exitCode);
	if(!success || exitCode != 0)
	    {
	    fprintf(stderr, "oovBuilder: Unable to build %s\nArguments were: ", outFile);
	    args.printArgs(stderr);
	    }
	}
    else
	{
	fprintf(stderr, "oovBuilder: Unable to create directory %s\n",
	    outDir.c_str());
	}
    fflush(stdout);
    fflush(stderr);
    return success;
    }

std::string ComponentBuilder::makeOutputObjectFileName(char const * const srcFile)
    {
    std::string outFileName;
    Project::makeTreeOutBaseFileName(srcFile, mSrcRootDir, mIntermediatePath,
	    outFileName);
    outFileName += ".o";
    return outFileName;
    }

bool ComponentBuilder::makeObj(const std::string &srcFile,
	const std::vector<std::string> &incDirs,
	const std::vector<std::string> &incFiles,
	const std::set<std::string> &externPkgCompileArgs)
    {
    bool success = true;
    if(isSource(srcFile.c_str()))
	{
	int incFileOlderIndex = -1;
	std::string outFileName = makeOutputObjectFileName(srcFile.c_str());
	if(FileStat::isOutputOld(outFileName.c_str(), srcFile.c_str()) ||
		FileStat::isOutputOld(outFileName.c_str(), incFiles, &incFileOlderIndex))
	    {
	    CppChildArgs ca;
	    std::string procPath = mToolPathFile.getCompilerPath();
	    ca.addArg(procPath.c_str());
	    ca.addCompileArgList(mComponentFinder, incDirs);
	    for(auto const &arg : externPkgCompileArgs)
		{
		ca.addArg(arg.c_str());
		}
	    ca.addArg("-o");
	    ca.addArg(outFileName.c_str());
	    ca.addArg(srcFile.c_str());
	    sLog.logProcess(srcFile.c_str(), ca.getArgv(), ca.getArgc());
	    success = runProcess(procPath.c_str(), outFileName.c_str(), ca);
	    if(incFileOlderIndex != -1)
		sLog.logOutputOld(incFiles[incFileOlderIndex].c_str());
	    sLog.logProcessStatus(success);
            }
	}
    return success;
    }

bool ComponentsFile::excludesMatch(const std::string &filePath,
	const std::vector<std::string> &excludes)
    {
    bool exclude = false;
    for(const auto &str : excludes)
	{
	if(filePath.find(str) != std::string::npos)
	    {
	    exclude = true;
	    break;
	    }
	}
    return exclude;
    }

/*
static bool getDirListMatchExtWithExcludes(char const * const path,
	const FilePath &fp, const std::vector<std::string> &excludes,
	std::vector<std::string> &files)
    {
    std::vector<std::string> tempFiles;
    bool success = getDirListMatchExt(path, fp, tempFiles);
    for(const auto &file : tempFiles)
	{
	if(!ComponentsFile::excludesMatch(file.c_str(), excludes))
	    files.push_back(file);
	}
    return success;
    }
*/

std::string ComponentBuilder::getSymbolBasePath()
    {
    FilePath outPath = mIntermediatePath;
    outPath.appendDir("sym");
    return outPath;
    }

bool ComponentBuilder::makeLibSymbols(char const * const clumpName,
	const std::vector<std::string> &files)
    {
    std::string objSymbolTool = mToolPathFile.getObjSymbolPath();
    return(mObjSymbols.makeObjectSymbols(clumpName, files,
	    getSymbolBasePath().c_str(), objSymbolTool.c_str()));
    }

bool ComponentBuilder::makeLib(const std::string &libName,
	const std::vector<std::string> &objectFileNames, std::string &outFileName,
	bool &builtLib)
    {
    bool success = true;
    builtLib = false;
    outFileName = mOutputPath + "lib" + libName + ".a";
    if(FileStat::isOutputOld(outFileName.c_str(), objectFileNames))
	{
	std::string procPath = mToolPathFile.getLibberPath();
	OovProcessChildArgs ca;
	ca.addArg(procPath.c_str());
	ca.addArg("r");
	ca.addArg(outFileName.c_str());
	for(const auto &objName : objectFileNames)
	    {
	    ca.addArg(objName.c_str());
	    }
	    sLog.logProcess(outFileName.c_str(), ca.getArgv(), ca.getArgc());
	success = runProcess(procPath.c_str(), outFileName.c_str(), ca);
	if(success)
	    builtLib = true;
	sLog.logProcessStatus(success);
	}
    return success;
    }


/// @param libs Libary names including full paths. Only contains project and
///		clump (-ER) libraries. getLinkArgs() contains -l from command
///		line and -EP
bool ComponentBuilder::makeExe(char const * const compName,
	const std::vector<std::string> &sources,
	const std::vector<std::string> &projectLibFilePaths,
	const std::vector<std::string> &externLibsDirs,
	const std::vector<std::string> &externOrderedLibNames,
	const std::set<std::string> &externPkgLinkArgs,
	bool shared)
    {
    bool success = true;
    std::string exeName = mComponentFinder.makeFileName(compName);
    std::string outFileName = mOutputPath + exeName;
    if(shared)
	outFileName += ".so";
    else
	outFileName = makeExeFilename(outFileName.c_str());

    std::vector<std::string> objects;
    for(const auto &src : sources)
	{
	std::string objName = makeOutputObjectFileName(src.c_str());
	objects.push_back(objName.c_str());
	}

    if(FileStat::isOutputOld(outFileName.c_str(), projectLibFilePaths) ||
	    FileStat::isOutputOld(outFileName.c_str(), objects))
	{
	std::string procPath = mToolPathFile.getCompilerPath();
	OovProcessChildArgs ca;
	ca.addArg(procPath.c_str());
	ca.addArg("-o");
	ca.addArg(outFileName.c_str());
	if(shared)
	    ca.addArg("-shared");

	for(const auto &obj : objects)
	    ca.addArg(obj.c_str());

	std::vector<std::string> libNames;	// must be vector for no sorting
	std::set<std::string> libDirs;

	for(const auto &lib : projectLibFilePaths)
	    {
	    FilePathImmutableRef libPath(lib.c_str());
	    libDirs.insert(libPath.getDrivePath());
	    libNames.push_back(libPath.getNameExt());
	    }

	// These libs must be after other libs above for the oovCppParser project.
	// May have to look at a better way of keeping original order.
	for(const auto &arg : mComponentFinder.getProject().getLinkArgs())
	    {
	    std::string temp = arg;
	    CompoundValue::quoteCommandLineArg(temp);
	    if(temp.compare(0, 2, "-l") == 0)
		libNames.push_back(temp.substr(2));
	    else
		ca.addArg(temp.c_str());
	    }
	for(const auto &arg : externPkgLinkArgs)
	    {
	    std::string temp = arg;
	    CompoundValue::quoteCommandLineArg(temp);
	    if(temp.compare(0, 2, "-l") == 0)
		libNames.push_back(temp.substr(2));
	    else
		ca.addArg(temp.c_str());
	    }

	for(auto const &dir : externLibsDirs)
	    {
	    libDirs.insert(dir);
	    }
	std::copy(externOrderedLibNames.begin(), externOrderedLibNames.end(),
		std::back_inserter(libNames));

	for(const auto &path : libDirs)
	    {
	    std::string quotedPath = path;
	    quoteCommandLinePath(quotedPath);
	    std::string arg = std::string("-L") + quotedPath;
	    ca.addArg(arg.c_str());
	    }

	for(const auto &lib : libNames)
	    {
	    std::string arg = "-l";
	    // Removing .a or .lib works in Windows.
	    // On Windows, clang is libclang.lib, and the link arg must be -llibclang
	    //		pango-1.0.lib must be -lpango-1.0
	    FilePath libFn(lib.c_str(), FP_File);
	    if(libFn.getExtension().compare(".a") == 0)
		{
		libFn.discardMatchingHead("lib");
		arg += libFn.getName();		// with no extension
		}
	    else if(libFn.getExtension().compare(".lib") == 0)
		{
		arg += libFn.getName();		// with no extension
		}
	    else
		{
		// cannot discard extension or it will change "atk-1.0" to "atk-1"
		arg += libFn;
		}
	    ca.addArg(arg.c_str());
	    }

	sLog.logProcess(outFileName.c_str(), ca.getArgv(), ca.getArgc());
	success = runProcess(procPath.c_str(), outFileName.c_str(), ca);
	sLog.logProcessStatus(success);
	}
    return success;
    }
