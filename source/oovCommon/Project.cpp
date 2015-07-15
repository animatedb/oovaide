/*
 * Project.cpp
 *
 *  Created on: Jul 16, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Project.h"
#include "Packages.h"

OovString Project::sProjectDirectory;
OovString Project::sSourceRootDirectory;

static char sCoverageSourceDir[] = "oov-cov";
static char sCoverageProjectDir[] = "oov-cov-oovcde";


OovString makeBuildConfigArgName(OovStringRef const baseName,
        OovStringRef const buildConfig)
    {
    OovString name = baseName;
    name += '-';
    name += buildConfig;
    return name;
    }

OovString Project::getComponentTypesFilePath()
    {
    FilePath fn(sProjectDirectory, FP_Dir);
    fn.appendFile("oovcde-comptypes.txt");
    return fn;
    }

OovString Project::getComponentSourceListFilePath()
    {
    FilePath fn(sProjectDirectory, FP_Dir);
    fn.appendFile("oovcde-tmp-compsources.txt");
    return fn;
    }

OovString Project::getPackagesFilePath()
    {
    FilePath fn(Project::getProjectDirectory(), FP_Dir);
    fn.appendFile("oovcde-pkg.txt");
    return fn;
    }

OovString Project::getBuildPackagesFilePath()
    {
    FilePath fn(Project::getProjectDirectory(), FP_Dir);
    fn.appendFile("oovcde-tmp-buildpkg.txt");
    return fn;
    }

OovStringRef const Project::getSrcRootDirectory()
    {
    if(sSourceRootDirectory.length() == 0)
        {
        NameValueFile file(getProjectFilePath());
        file.readFile();
        sSourceRootDirectory = file.getValue(OptSourceRootDir);
        }
    return sSourceRootDirectory;
    }


OovString const &Project::getBinDirectory()
    {
    static OovString path;
    if(path.length() == 0)
        {
#ifdef __linux__
        // On linux, we could use the "which" command using "which oovcde", but
        // this will probably be ok?
        if(FileIsFileOnDisk("./oovcde"))
            {
            path = "./";
            }
        else if(FileIsFileOnDisk("/usr/local/bin/oovcde"))
            {
            path = "/usr/local/bin/";
            }
        else
            {
            path = "/usr/bin";
            }
#else
        path = "./";
#endif
        }
    return path;
    }

OovString Project::getProjectFilePath()
    {
    FilePath fn(sProjectDirectory, FP_Dir);
    fn.appendFile("oovcde.txt");
    return fn;
    }

OovString Project::getGuiOptionsFilePath()
    {
    FilePath fn(sProjectDirectory, FP_Dir);
    fn.appendFile("oovcde-gui.txt");
    return fn;
    }

FilePath Project::getOutputDir(OovStringRef const buildDirClass)
    {
    std::string outClass("out-");
    outClass += buildDirClass;
    FilePath dir(getProjectDirectory(), FP_Dir);
    dir.appendDir(outClass);
    return dir;
    }

FilePath Project::getIntermediateDir(OovStringRef const buildDirClass)
    {
    std::string outClass("bld-");
    outClass += buildDirClass;
    FilePath dir(getProjectDirectory(), FP_Dir);
    dir.appendDir(outClass);
    return dir;
    }

OovString Project::getSrcRootDirRelativeSrcFileName(OovStringRef const srcFileName,
        OovStringRef const srcRootDir)
    {
    OovString relSrcFileName = srcFileName;
    size_t pos = relSrcFileName.find(srcRootDir.getStr());
    if(pos != std::string::npos)
        {
        relSrcFileName.erase(pos, srcRootDir.numBytes());
        FilePathRemovePathSep(relSrcFileName, 0);
        }
    return relSrcFileName;
    }

OovString Project::getSrcRootDirRelativeSrcFileDir(OovStringRef const srcFileName)
    {
    FilePath relSrcFileDir(srcFileName, FP_Dir);
    OovStringRef srcRootDir = Project::getSrcRootDirectory();
    size_t pos = relSrcFileDir.find(srcRootDir);
    if(pos != std::string::npos)
        {
        relSrcFileDir.erase(pos, srcRootDir.numBytes());
        FilePathRemovePathSep(relSrcFileDir, 0);
        }
    return relSrcFileDir;
    }

static OovString getAbsoluteDirFromSrcRootDirRelativeDir(
    OovStringRef const relSrcFileDir)
    {
    FilePath absSrcFileDir(Project::getSrcRootDirectory(), FP_Dir);
    absSrcFileDir.appendDir(relSrcFileDir);
    return absSrcFileDir;
    }

OovString Project::makeOutBaseFileName(OovStringRef const srcFileName,
        OovStringRef const srcRootDir, OovStringRef const outFilePath)
    {
    OovString file = getSrcRootDirRelativeSrcFileName(srcFileName, srcRootDir);
    if(file[0] == '/')
        file.erase(0, 1);
    file.replaceStrs("_", "_u");
    file.replaceStrs("/", "_s");
    file.replaceStrs(".", "_d");
    FilePath outFileName(outFilePath, FP_Dir);
    outFileName.appendFile(file);
    return outFileName;
    }

OovString Project::recoverFileName(OovStringRef const srcFileName)
    {
    OovString file = srcFileName;
    file.replaceStrs("_u", "_");
    file.replaceStrs("_s", "/");
    file.replaceStrs("_d", ".");
    return file;
    }

OovString Project::makeTreeOutBaseFileName(OovStringRef const srcFileName,
        OovStringRef const srcRootDir, OovStringRef const outFilePath)
    {
    OovString file = getSrcRootDirRelativeSrcFileName(srcFileName, srcRootDir);
    if(file[0] == '/')
        file.erase(0, 1);

    FilePath tmpOutFileName(outFilePath, FP_Dir);
    tmpOutFileName.appendFile(file);
    tmpOutFileName.discardExtension();
    return tmpOutFileName;
    }

OovString Project::getCoverageSourceDirectory()
    {
    FilePath coverageDir(getProjectDirectory(), FP_Dir);
    coverageDir.appendDir(sCoverageSourceDir);
    return coverageDir;
    }

OovString Project::makeCoverageSourceFileName(OovStringRef const srcFileName,
        OovStringRef const srcRootDir)
    {
    OovString coverageDir = getCoverageSourceDirectory();
    OovString outFileName = makeTreeOutBaseFileName(srcFileName, srcRootDir,
            coverageDir);
    FilePath srcFn(srcFileName, FP_File);
    outFileName += srcFn.getExtension();
    return outFileName;
    }

OovString Project::getCoverageProjectDirectory()
    {
    FilePath coverageDir(getProjectDirectory(), FP_Dir);
    coverageDir.appendDir(sCoverageProjectDir);
    return coverageDir;
    }


bool ProjectReader::readProject(OovStringRef const oovProjectDir)
    {
    Project::setProjectDirectory(oovProjectDir);
    setFilename(Project::getProjectFilePath());
    bool success = readFile();
    if(success)
        {
        Project::setSourceRootDirectory(getValue(OptSourceRootDir));
        }
    return success;
    }

void ProjectBuildArgs::loadBuildArgs(OovStringRef const buildConfigName)
    {
    mProjectPackages.read();
    mBuildPackages.read();

    OovStringVec args;
    CompoundValue baseArgs;
    baseArgs.parseString(mProjectOptions.getValue(OptBaseArgs));
    for(auto const &arg : baseArgs)
        {
        args.push_back(arg);
        }

    std::string optionExtraArgs = makeBuildConfigArgName(OptExtraBuildArgs,
            buildConfigName);
    CompoundValue extraArgs;
    extraArgs.parseString(mProjectOptions.getValue(optionExtraArgs));
    extraArgs.quoteAllArgs();
    for(auto const &arg : extraArgs)
        {
        args.push_back(arg);
        }
    parseArgs(args);
    }

void ProjectBuildArgs::parseArgs(OovStringVec const &args)
    {
    unsigned int linkOrderIndex = LOI_AfterInternalProject;
    for(auto const &arg : args)
        {
        if(arg.find("-ER", 0, 3) == 0)
            {
            addExternalArg(arg);
            }
        else if(arg.find("-EP", 0, 3) == 0)
            {
            addExternalPackageName(linkOrderIndex, arg.substr(3));
            linkOrderIndex += LOI_PackageIncrement;
            }
        else if(arg.find("-bv", 0, 3) == 0)
            {
            mVerbose = true;
            }
        else if(arg.find("-lnk", 0, 4) == 0)
            {
            addLinkArg(linkOrderIndex++, arg.substr(4));
            }
        else
            {
            addCompileArg(arg);
            }
        }
    for(auto const &arg : args)
        {
        if(arg.find("-EP", 0, 3) == 0)
            handleExternalPackage(arg.substr(3));
        }
    }

CompoundValue ProjectBuildArgs::getProjectExcludeDirs() const
    {
    CompoundValue val;
    val.parseString(mProjectOptions.getValue(OptProjectExcludeDirs));
// This doesn't work.
//    val.push_back(sCoverageSourceDir);
//    val.push_back(sCoverageProjectDir);
    for(size_t i=0; i<val.size(); i++)
        {
        val[i] = getAbsoluteDirFromSrcRootDirRelativeDir(val[i]);
        }
    return val;
    }

void ProjectBuildArgs::handleExternalPackage(OovStringRef const pkgName)
    {
    addPackageCrcName(pkgName);
    Package pkg = mProjectPackages.getPackage(pkgName);
    OovStringVec cppArgs = pkg.getCompileArgs();
    for(auto const &arg : cppArgs)
        {
        addPackageCrcCompileArg(arg);
        }
    OovStringVec incDirs = pkg.getIncludeDirs();
    for(auto const &dir : incDirs)
        {
        std::string arg = "-I";
        arg += dir;
        addPackageCrcCompileArg(arg);
        }
    OovStringVec linkArgs = pkg.getLinkArgs();
    for(auto const &arg : linkArgs)
        {
        addPackageCrcLinkArg(arg);
        }
    OovStringVec libDirs = pkg.getLibraryDirs();
    for(auto const &dir : libDirs)
        {
        std::string arg = "-L";
        arg += dir;
        addPackageCrcLinkArg(arg);
        }
    OovStringVec libNames = pkg.getLibraryNames();
    for(auto const &dir : libNames)
        {
        std::string arg = "-l";
        arg += dir;
        addPackageCrcLinkArg(arg);
        }
    }

const OovStringVec ProjectBuildArgs::getAllCrcCompileArgs() const
    {
    OovStringVec vec;
    vec = mCompileArgs;
    std::copy(mPackageCrcCompileArgs.begin(), mPackageCrcCompileArgs.end(),
            std::back_inserter(vec));
    std::copy(mPackageCrcNames.begin(), mPackageCrcNames.end(),
            std::back_inserter(vec));
    return vec;
    }

const OovStringVec ProjectBuildArgs::getAllCrcLinkArgs() const
    {
    OovStringVec vec;
    for(auto item : mLinkArgs)
        vec.push_back(item.mString);
    std::copy(mPackageCrcLinkArgs.begin(), mPackageCrcLinkArgs.end(),
            std::back_inserter(vec));
    return vec;
    }

unsigned int ProjectBuildArgs::getExternalPackageLinkOrder(OovStringRef const pkgName) const
    {
    unsigned int index = LOI_AfterInternalProject;
    for(auto &item : mExternalPackageNames)
        {
        if(item.mString.compare(pkgName) == 0)
            index = item.mLinkOrderIndex;
        }
    return index;
    }
