
import testSupport
import os
import shutil
import re
import time


inDir = "oovCppParserIn"
outDir = "oovCppParserOut"

def verifySubString(mainstr, substr):
    if(mainstr.find(substr) == -1):
        testSupport.dumpError("Missing: " + substr)

def verifyRegEx(mainstr, findstr):
    if(not re.findall(findstr, mainstr)):
        testSupport.dumpError("Missing: " + findstr)

def runCppParser(srcFn):
    if not os.path.exists(outDir):
        os.mkdir(outDir)
    inFn = os.path.normpath(inDir + "/" + srcFn)
    cmd = os.path.normpath("../../bin/oovCppParser.exe ")
    outRedir = os.path.normpath(outDir + "/testCppParserOut.txt")
    os.system(cmd + inFn + " " + inDir + \
              " " + outDir + " -c -x c++ -std=c++11 > " + outRedir)
    if os.path.exists(outDir + "/" + srcFn.replace(".", "_") + "-err.txt"):
        testSupport.dumpError("Error file should not exist")

def setup():
    while os.path.exists(outDir):
        shutil.rmtree(outDir)
        time.sleep(.5)


def testAggregation():
    setup()
    runCppParser("testAggregation.cpp")
    with file(outDir + "/testAggregation_cpp.xmi") as outFile:
        outContents = outFile.read()

        verifyRegEx(outContents, "Class[^<>]+name=\"classBase")
        verifyRegEx(outContents, "Class[^<>]+name=\"classLeaf2a")
        verifyRegEx(outContents, "Class[^<>]+name=\"classLeaf3a")
        verifyRegEx(outContents, "Class[^<>]+name=\"classMultiLeaf")
        verifyRegEx(outContents, "Operation name=\"func\" access=\"public\" const=\"false")
        verifyRegEx(outContents, "Attribute[^<>]+name=\"classLeaf2a_intMember\""     \
            ".+const=\"false\".+ref=\"false\".+access=\"private\"")
        verifyRegEx(outContents, "Attribute[^<>]+name=\"classLeaf2a_leaf3aMember\""  \
            ".+const=\"false\".+ref=\"false\".+access=\"public\"")
        verifyRegEx(outContents, "Attribute[^<>]+name=\"classLeaf2a_multiLeafMember\""   \
            ".+const=\"true\".+ref=\"true\".+access=\"private\"")


def testInherit():
    runCppParser("testInherit.h")
    with file(outDir + "/testInherit_h.xmi") as outFile:
        outContents = outFile.read()

        verifyRegEx(outContents, "Class[^<>]+name=\"baseClass")
        verifyRegEx(outContents, "Operation[^<>]+name=\"baseInlineFunc.+"   \
            "access=\"public\".+const=\"false")
        verifyRegEx(outContents, "Operation[^<>]+name=\"constBaseInlineFunc.+"  \
            "access=\"public\".+const=\"true")
        verifyRegEx(outContents, "Generalization xmi.id=\"100000\" "   \
            "child=\"4\" parent=\"2\" access=\"protected\"")
        verifyRegEx(outContents, "Generalization xmi.id=\"100001\" "   \
            "child=\"5\" parent=\"2\" access=\"private\"")
        verifyRegEx(outContents, "Generalization xmi.id=\"100002\" "   \
            "child=\"6\" parent=\"2\" access=\"public\"")

    
def testStatements():
    runCppParser("testStatements.cpp")
    with file(outDir + "/testStatements_cpp.xmi") as outFile:
        outContents = outFile.read()

        verifySubString(outContents, "*[x &lt; sizeof ( data ) ]")
        verifySubString(outContents, "[x &amp; val ]")
        verifySubString(outContents, "Condition name=\"*[: data2]\"")


def testTemplates():
    runCppParser("testTemplates.h")
    with file(outDir + "/testTemplates_h.xmi") as outFile:
        outContents = outFile.read()

        verifyRegEx(outContents, "DataType[^<>]+name=\"std::vector&lt;templItem&gt;")
        verifyRegEx(outContents, "Class[^<>]+name=\"templClassInherited")
        verifyRegEx(outContents, "Class[^<>]+name=\"templClassHasMembers")
        verifyRegEx(outContents, "Attribute[^<>]+name=\"itemVector")
        verifyRegEx(outContents, "Attribute[^<>]+name=\"itemPtrVector")
        verifyRegEx(outContents, "Generalization ")
