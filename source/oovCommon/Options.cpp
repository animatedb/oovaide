/*
 * Options.cpp
 *
 *  Created on: Jun 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Options.h"
#include "Project.h"
#include "OovString.h"
#include <stdlib.h>     // for getenv
#include <string.h>


#ifndef __linux__
/*
// Return is empty if search string not found
static std::string getPathSegment(const std::string &path, OovStringRef const srch)
    {
    OovString lowerPath;
    lowerPath.setLowerCase(path);
    std::string seg;
    size_t pos = lowerPath.find(srch);
    if(pos != std::string::npos)
        {
        std::string arg = "-ER";
        size_t startPos = path.rfind(';', pos);
        if(startPos != std::string::npos)
            startPos++;
        else
            startPos = 0;
        size_t endPos = path.find(';', pos);
        if(endPos == std::string::npos)
            endPos = path.length();
        seg = path.substr(startPos, endPos-startPos);
        }
    return seg;
    }

static bool addExternalReference(const std::string &path, OovStringRef const srch,
        CompoundValue &args)
    {
    std::string seg = getPathSegment(path, srch);
    size_t binPos = seg.find("bin", seg.length()-3);
    if(binPos != std::string::npos)
        {
        seg.resize(binPos);
        removePathSep(seg, binPos-1);
        }
    if(seg.size() > 0)
        {
        std::string arg = "-ER";
        arg += seg;
        args.addArg(arg);
        }
    return seg.size() > 0;
    }
*/
#endif

static void addCLangArgs(CompoundValue &cv)
    {
    cv.addArg("-x");
    cv.addArg("c++");
    cv.addArg("-std=c++11");
    }

static void setBuildConfigurationPaths(NameValueFile &file,
        OovStringRef const buildConfig, OovStringRef const extraArgs, bool useclang)
    {
    std::string optStr = makeBuildConfigArgName(OptExtraBuildArgs, buildConfig);
    file.setNameValue(optStr, extraArgs);

    if(std::string(buildConfig).compare(BuildConfigAnalysis) == 0)
        {
        useclang = true;
        }
    // Assume the archiver is installed and on path.
    // llvm-ar gives link error.
    // setNameValue(makeExeFilename("llvm-ar"));
    optStr = makeBuildConfigArgName(OptToolLibPath, buildConfig);
    file.setNameValue(optStr, FilePathMakeExeFilename("ar"));

    // llvm-nm gives bad file error on Windows, and has no output on Linux.
    // mPathObjSymbol = "makeExeFilename(llvm-nm)";
    optStr = makeBuildConfigArgName(OptToolObjSymbolPath, buildConfig);
    file.setNameValue(optStr, FilePathMakeExeFilename("nm"));

    std::string compiler;
    if(useclang)
        compiler = FilePathMakeExeFilename("clang++");
    else
        compiler = FilePathMakeExeFilename("g++");
    optStr = makeBuildConfigArgName(OptToolCompilePath, buildConfig);
    file.setNameValue(optStr, compiler);

    optStr = makeBuildConfigArgName(OptToolJavaCompilePath, buildConfig);
    if(strcmp(buildConfig, BuildConfigAnalysis) == 0)
        {
        file.setNameValue(optStr, "java");

        optStr = makeBuildConfigArgName(OptToolJavaAnalyzerTool, buildConfig);
        file.setNameValue(optStr, "oovJavaParser");
        }
    else
        {
        file.setNameValue(optStr, "javac");
        }

    optStr = makeBuildConfigArgName(OptToolJavaJarToolPath, buildConfig);
    file.setNameValue(optStr, "jar");
    }

void OptionsDefaults::setDefaultOptions()
    {
    CompoundValue baseArgs;
    CompoundValue extraCppDocArgs;
    CompoundValue extraCppRlsArgs;
    CompoundValue extraCppDbgArgs;

    baseArgs.addArg("-c");
    bool useCLangBuild = false;
#ifdef __linux__
//    baseArgs.addArg("-ER/usr/include/gtk-3.0");
//    baseArgs.addArg("-ER/usr/lib/x86_64-linux-gnu/glib-2.0/include");
    OovStatus status(true, SC_File);
    if(FileIsFileOnDisk("/usr/bin/clang++", status))
        useCLangBuild = true;
    if(status.needReport())
        {
        status.reported();
        }
#else
    OovString path = getenv("PATH");
    if(path.find("LLVM") != std::string::npos)
        {
        useCLangBuild = true;
        }
/*
    // On Windows, GTK includes gmodule, glib, etc., so no reason to get mingw.
    bool success = addExternalReference(path, "gtk", baseArgs);
    if(!success)
        {
        addExternalReference(path, "mingw", baseArgs);
        }
    addExternalReference(path, "qt", baseArgs);
*/
#endif
    // The GNU compiler cannot have these, but the clang compiler requires them.
    // CLang 3.2 does not work on Windows well, so GNU is used on Windows for
    // building, but CLang is used for XMI parsing.
    // These are important for xmi parsing to show template relations
    if(useCLangBuild)
        {
        addCLangArgs(baseArgs);
        }
    else
        {
        addCLangArgs(extraCppDocArgs);
        extraCppDbgArgs.addArg("-std=c++0x");
        extraCppRlsArgs.addArg("-std=c++0x");
        }
    extraCppDbgArgs.addArg("-O0");
    extraCppDbgArgs.addArg("-g3");

    extraCppRlsArgs.addArg("-O3");

#ifdef __linux__
    mProject.setNameValue(OptToolDebuggerPath, "/usr/bin/gdb");
#else
    // This works without setting the full path.
    // The path could be /MinGW/bin, or could be /Program Files/mingw-builds/...
    mProject.setNameValue(OptToolDebuggerPath, "gdb.exe");
#endif

    setBuildConfigurationPaths(mProject, BuildConfigAnalysis,
            extraCppDocArgs.getAsString(), useCLangBuild);
    setBuildConfigurationPaths(mProject, BuildConfigDebug,
            extraCppDbgArgs.getAsString(), useCLangBuild);
    setBuildConfigurationPaths(mProject, BuildConfigRelease,
            extraCppRlsArgs.getAsString(), useCLangBuild);

    mProject.setNameValue(OptBaseArgs, baseArgs.getAsString());

    OovString str = getenv("CLASSPATH");;
    if(str.length())
        {
        mProject.setNameValue(OptJavaClassPath, str);
        }
    str = getenv("JAVA_HOME");
    if(str.length() > 0)
        {
#ifdef __linux__
        str = "/usr/lib/jvm/default-java";
#else
#endif
        mProject.setNameValue(OptJavaJdkPath, str);
        }
    }

void GuiOptions::setDefaultOptions()
    {
    setNameValueBool(OptGuiShowAttributes, true);
    setNameValueBool(OptGuiShowOperations, true);
    setNameValueBool(OptGuiShowOperParams, true);
    setNameValueBool(OptGuiShowAttrTypes, true);
    setNameValueBool(OptGuiShowOperTypes, true);
    setNameValueBool(OptGuiShowPackageName, true);
    setNameValueBool(OptGuiShowOovSymbols, true);
    setNameValueBool(OptGuiShowTemplateRelations, true);
    setNameValueBool(OptGuiShowOperParamRelations, true);
    setNameValueBool(OptGuiShowOperBodyVarRelations, true);
    setNameValueBool(OptGuiShowRelationKey, true);

    setNameValueBool(OptGuiShowCompImplicitRelations, false);
    /*
    #ifdef __linux__
        setNameValue(OptEditorPath, "/usr/bin/gedit");
    #else
        setNameValue(OptEditorPath, "\\Windows\\notepad.exe");
    #endif
    */
#ifdef __linux__
    setNameValue(OptGuiEditorPath, "./oovEdit");
#else
    setNameValue(OptGuiEditorPath, "oovEdit.exe");
#endif
    setNameValue(OptGuiEditorLineArg, "+");
    }

OovStatusReturn GuiOptions::read()
    {
    setFilename(Project::getGuiOptionsFilePath());
    OovStatus status = NameValueFile::readFile();
    if(status.ok())
        {
        setDefaultOptions();
        }
    return status;
    }
