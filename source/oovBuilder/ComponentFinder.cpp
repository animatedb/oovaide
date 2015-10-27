/*
 * projFinder.cpp
 *
 *  Created on: Aug 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */
#include "ComponentFinder.h"
#include "Project.h"
#include "Debug.h"
#include "OovError.h"
#include <set>

#define DEBUG_FINDER 0
#if DEBUG_FINDER
static DebugFile sLog("oov-finder-debug.txt");
#endif

void ToolPathFile::getPaths()
    {
    if(mPathCompiler.length() == 0)
        {
        std::string projFileName = Project::getProjectFilePath();
        setFilename(projFileName);
        OovStatus status = readFile();
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to get project paths");
            }

        std::string optStr = makeBuildConfigArgName(OptToolLibPath, mBuildConfig);
        mPathLibber = getValue(optStr);
        optStr = makeBuildConfigArgName(OptToolCompilePath, mBuildConfig);
        mPathCompiler = getValue(optStr);
        optStr = makeBuildConfigArgName(OptToolObjSymbolPath, mBuildConfig);
        mPathObjSymbol = getValue(optStr);
        optStr = makeBuildConfigArgName(OptToolJavaCompilePath, mBuildConfig);
        mPathJavaCompiler = getValue(optStr);
        optStr = makeBuildConfigArgName(OptToolJavaJarToolPath, mBuildConfig);
        mPathJavaJarTool = getValue(optStr);
        }
    }

std::string ToolPathFile::getAnalysisToolCommand(FilePath const &filePath)
    {
    OovString command;
    if(isJavaSource(filePath))
        {
        command += "java -cp";
        OovString jarsArg;
        jarsArg += "oovJavaParser.jar";
#ifdef __linux__
        jarsArg += ":";
#else
        jarsArg += ";";
#endif
        FilePath toolsJar(getenv("JAVA_HOME"), FP_Dir);
        toolsJar.appendDir("lib");
        toolsJar.appendFile("tools.jar");
        jarsArg += toolsJar;
        FilePathQuoteCommandLinePath(jarsArg);
        command += ' ';
        command += jarsArg;
        command += " oovJavaParser";
        }
    else
        {
        FilePath path(Project::getBinDirectory(), FP_Dir);
        path.appendFile("oovCppParser");
        command = FilePathMakeExeFilename(path);
        }
    return(command);
    }

std::string ToolPathFile::getCompilerPath()
    {
    getPaths();
    return(mPathCompiler);
    }

std::string ToolPathFile::getJavaCompilerPath()
    {
    getPaths();
    return(mPathJavaCompiler);
    }

std::string ToolPathFile::getJavaJarToolPath()
    {
    getPaths();
    return(mPathJavaJarTool);
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

std::string ToolPathFile::getCovInstrToolPath()
    {
    OovString path = Project::getBinDirectory();
    path += "oovCovInstr";
    return(path);
    }

void ScannedComponent::saveComponentFileInfo(
    ComponentTypesFile::CompFileTypes cft, ProjectReader const &proj,
    OovStringRef const compName, ComponentTypesFile &compFile,
    OovStringRef analysisPath, OovStringSet const &newFiles) const
    {
    OovStringSet deleteFiles;
    for(auto const &newFile : newFiles)
        {
        OovString newCompName = getComponentName(proj, newFile);
        OovStringVec origFiles = compFile.getComponentFiles(cft, newCompName, false);
        for(auto const &origFileName : origFiles)
            {
            if(newFiles.find(origFileName) == newFiles.end())
                {
                // Found a file that is not in the new set. It must have been
                // deleted. So delete the analysis file if it exists.
                OovString analysisFile = Project::makeAnalysisFileName(origFileName,
                        Project::getSrcRootDirectory(), analysisPath);
                deleteFiles.insert(analysisFile);
                }
            }
        }
    // It should be ok to delete analysis files since they should not conflict
    // with scanning the source directories.
    for(auto const &file : deleteFiles)
        {
        OovStatus status = FileDelete(file.getStr());
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to delete analysis files");
            }
        }
    compFile.setComponentFiles(cft, compName, newFiles);
    }

void ScannedComponent::saveComponentSourcesToFile(ProjectReader const &proj,
    OovStringRef const compName, ComponentTypesFile &compFile,
    OovStringRef analysisPath) const
    {
    saveComponentFileInfo(ComponentTypesFile::CFT_JavaSource, proj, compName,
        compFile, analysisPath, mJavaSourceFiles);
    saveComponentFileInfo(ComponentTypesFile::CFT_CppSource, proj, compName,
        compFile, analysisPath, mCppSourceFiles);
    saveComponentFileInfo(ComponentTypesFile::CFT_CppInclude, proj, compName,
        compFile, analysisPath, mCppIncludeFiles);
    }

OovString ScannedComponent::getComponentName(ProjectReader const &proj,
    OovStringRef const filePath, OovString *rootPathName)
    {
    FilePath path(filePath, FP_File);
    path.discardFilename();
    OovString compName = Project::getSrcRootDirRelativeSrcFileName(path,
            proj.getSrcRootDirectory());
    if(compName.length() == 0)
        {
        if(rootPathName)
            {
            *rootPathName = Project::getRootComponentFileName();
            }
        compName = Project::getRootComponentName();
        }
    else
        {
        FilePathRemovePathSep(compName, compName.length()-1);
        }
    return compName;
    }

OovString ComponentFinder::getComponentName(OovStringRef const filePath)
    {
    return ScannedComponent::getComponentName(mProject, filePath, &mRootPathName);
    }


//////////////

ScannedComponentsInfo::MapIter ScannedComponentsInfo::addComponents(
    OovStringRef const compName)
    {
    FilePath parent = FilePath(compName, FP_Dir).getParent();
    if(parent.length() > 0)
        {
        FilePathRemovePathSep(parent, parent.length()-1);
        addComponents(parent);
        }
    MapIter it = mComponents.find(compName);
    if(it == mComponents.end())
        {
        it = mComponents.insert(std::make_pair(compName, ScannedComponent())).first;
        }
    return it;
    }

void ScannedComponentsInfo::addJavaSourceFile(OovStringRef const compName,
    OovStringRef const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addJavaSourceFileName(srcFileName);
    }

void ScannedComponentsInfo::addCppSourceFile(OovStringRef const compName,
    OovStringRef const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addCppSourceFileName(srcFileName);
    }

void ScannedComponentsInfo::addCppIncludeFile(OovStringRef const compName,
    OovStringRef const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addCppIncludeFileName(srcFileName);
    }

static void setFileValues(OovStringRef const tagName,
    OovStringVec const &vals, ComponentsFile &compFile)
    {
    CompoundValue incArgs;
    for(const auto &inc : vals)
        {
        incArgs.addArg(inc);
        }
    compFile.setNameValue(tagName, incArgs.getAsString());
    }

void ScannedComponentsInfo::setProjectComponentsFileValues(ComponentsFile &compFile)
    {
    setFileValues("Components-init-proj-incs", mProjectIncludeDirs, compFile);
    }

void ScannedComponentsInfo::initializeComponentTypesFileValues(
    ProjectReader const &proj, ComponentTypesFile &compFile,
    OovStringRef analysisPath)
    {
    CompoundValue comps;
    for(const auto &comp : mComponents)
        {
        comps.addArg(comp.first);
        comp.second.saveComponentSourcesToFile(proj, comp.first, compFile,
            analysisPath);
        }
    compFile.setComponentNames(comps.getAsString());
    }

//////////////

bool ComponentFinder::readProject(OovStringRef const oovProjectDir,
        OovStringRef const buildConfigName)
    {
    OovStatus status = mProject.readProject(oovProjectDir);
    if(status.ok())
        {
        mProjectBuildArgs.loadBuildArgs(buildConfigName);
        status = mComponentTypesFile.read();
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to read project for finding components");
        }
    return status.ok();
    }

OovStatus ComponentFinder::scanProject()
    {
    mScanningPackage = nullptr;
    mExcludeDirs = mProjectBuildArgs.getProjectExcludeDirs();
    OovStatus status = recurseDirs(mProject.getSrcRootDirectory().getStr());
    if(status.needReport())
        {
        status.report(ET_Error, "Error scanning project");
        }
    return status;
    }

void ComponentFinder::scanExternalProject(OovStringRef const externalRootSrch,
        Package const *pkg)
    {
    OovString externalRootDir;
    ComponentsFile::parseProjRefs(externalRootSrch, externalRootDir, mExcludeDirs);

    Package rootPkg;
    if(pkg)
        rootPkg = *pkg;
    rootPkg.setRootDirPackage(externalRootSrch);
    mScanningPackage = &rootPkg;
    mAddIncs = rootPkg.needIncs();
    mAddLibs = rootPkg.needLibs();
    rootPkg.clearDirScan();

    OovStatus status = recurseDirs(externalRootDir.getStr());
    if(status.needReport())
        {
        OovString err = "Error scanning external project ";
        err += externalRootDir;
        status.report(ET_Error, err);
        }

    addBuildPackage(rootPkg);
    }

void ComponentFinder::addBuildPackage(Package const &pkg)
    {
    getProjectBuildArgs().getBuildPackages().insertPackage(pkg);
    OovStatus status = getProjectBuildArgs().getBuildPackages().savePackages();
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to save build packages");
        }
    }

void ComponentFinder::saveProject(OovStringRef const compFn, OovStringRef analysisPath)
    {
    mComponentsFile.read(compFn);
    mScannedInfo.setProjectComponentsFileValues(mComponentsFile);
    mScannedInfo.initializeComponentTypesFileValues(mProject, mComponentTypesFile,
        analysisPath);
    OovStatus status = mComponentTypesFile.writeFile();
    if(status.needReport())
        {
        OovString errStr = "Unable to save component types file";
        status.report(ET_Error, errStr);
        }
    status = mComponentsFile.writeFile();
    if(status.needReport())
        {
        OovString errStr = "Unable to save components file: ";
        errStr += mComponentsFile.getFilename();
        status.report(ET_Error, errStr);
        }
    }

bool ComponentFinder::processFile(OovStringRef const filePath)
    {
    /// @todo - find files with no extension? to match things like std::vector include
    if(!ComponentsFile::excludesMatch(filePath, mExcludeDirs))
        {
        bool cppInc = isCppHeader(filePath);
        bool cppSrc = isCppSource(filePath);
        bool javaSrc = isJavaSource(filePath);
        bool lib = isLibrary(filePath);
        if(mScanningPackage)
            {
            if(cppInc)
                {
                if(mAddIncs)
                    {
                    FilePath path(filePath, FP_File);
                    path.discardFilename();

                    mScanningPackage->appendAbsoluteIncDir(path.getDrivePath());

                    // Insert parent for things like #include "gtk/gtk.h" or "sys/stat.h"
                    FilePath parent(path.getParent());
                    if(parent.length() > 0)
                        mScanningPackage->appendAbsoluteIncDir(parent.getDrivePath());

                    // Insert grandparent for things like "llvm/ADT/APInt.h"
                    FilePath grandparent(parent.getParent());
                    if(grandparent.length() > 0)
                        mScanningPackage->appendAbsoluteIncDir(grandparent.getDrivePath());
                    }
                }
            else if(lib)
                {
                if(mAddLibs)
                    {
                    FilePath path(filePath, FP_File);
                    mScanningPackage->appendAbsoluteLibName(filePath);
                    }
                }
            }
        else
            {
            if(cppSrc)
                {
                std::string name = getComponentName(filePath);
                mScannedInfo.addCppSourceFile(name, filePath);
                }
            else if(cppInc)
                {
                FilePath path(filePath, FP_File);
                path.discardFilename();
                mScannedInfo.addProjectIncludePath(path);
                std::string name = getComponentName(filePath);
                mScannedInfo.addCppIncludeFile(name, filePath);
                }
            else if(javaSrc)
                {
                std::string name = getComponentName(filePath);
                mScannedInfo.addJavaSourceFile(name, filePath);
                }
            }
        }
    return true;
    }

OovStringVec ComponentFinder::getAllIncludeDirs() const
    {
    InsertOrderedSet projIncs = getScannedInfo().getProjectIncludeDirs();
    OovStringVec incs;
    std::copy(projIncs.begin(), projIncs.end(), std::back_inserter(incs));
    for(auto const &pkg : getProjectBuildArgs().getBuildPackages().getPackages())
        {
        OovStringVec pkgIncs = pkg.getIncludeDirs();
        std::copy(pkgIncs.begin(), pkgIncs.end(), std::back_inserter(incs));
        }
    return incs;
    }


void CppChildArgs::addCompileArgList(const ComponentFinder &finder,
        const OovStringVec &incDirs)
    {
    // add cpp args
    for(const auto &arg : finder.getProjectBuildArgs().getCompileArgs())
        {
        std::string tempArg = arg;
        CompoundValue::quoteCommandLineArg(tempArg);
        addArg(tempArg);
        }

    for(size_t i=0; static_cast<size_t>(i)<incDirs.size(); i++)
        {
        std::string incStr = std::string("-I") + incDirs[i];
        CompoundValue::quoteCommandLineArg(incStr);
        addArg(incStr);
        }
    }

