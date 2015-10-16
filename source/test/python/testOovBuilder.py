
import testSupport
import errno
import stat
import os
import shutil
import re
import time

def runOovBuilder(projDir, configName, args):
    os.system("oovBuilder.exe " + projDir + " -bld-" + configName + \
        " " + args + " " + " > " + projDir + "/testOovBuilderOut.txt")


def testExampleStaticLibAnalysis():
    os.chdir("../../bin")
    # relative to bin dir
    projDir = os.path.normpath("../examples/staticlib-oovaide")
    while os.path.exists(projDir):
        shutil.rmtree(projDir)
        time.sleep(.5)
    os.mkdir(projDir)
    oovaideStrs = "BuildArgsBase|-c\n"   \
        "BuildArgsExtra-Analyze|-x;c++;-std=c++11\n"   \
        "BuildArgsExtra-Debug|-std=c++0x;-O0;-g3\n"   \
        "BuildArgsExtra-Release|-std=c++0x;-O3\n"   \
        "SourceRootDir|C:/Dave/Mine/software/oovaide/svn/trunk/examples/staticlib/\n"   \
        "Tool-CompilerPath-Debug|g++.exe\n"   \
        "Tool-CompilerPath-Release|g++.exe\n"   \
        "Tool-LibPath-Debug|ar.exe\n"   \
        "Tool-LibPath-Release|ar.exe\n"   \
        "Tool-ObjSymbolPath-Debug|nm.exe\n"   \
        "Tool-ObjSymbolPath-Release|nm.exe\n"
    of = open(projDir + "/oovaide.txt", 'w')
    of.write(oovaideStrs)
    of.close()

    compTypes = "Comp-type-BlackSheep|StaticLib\n"    \
        "Comp-type-MainClients|Program\n"     \
        "Components|BlackSheep;MainClients\n"
    ctf = open(projDir + "/oovaide-comptypes.txt", 'w')
    ctf.write(compTypes)
    ctf.close()

    runOovBuilder(projDir, "Debug", "-v")
    if not os.path.exists(projDir + "/" + "out-Debug/MainClients.exe"):
        testSupport.dumpError("Error file doesn't exist")
