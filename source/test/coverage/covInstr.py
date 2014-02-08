# Copyright 2013 DCBlaha.  Distributed under the GPL.
# This file instruments c++ code to generate code coverage information.
# When the instrumented c++ code runs, it will write the output to
# a file. Then covStats.py is run, which takes the output data and produces
# organized info inluding coverage percentages and source code with comments
# that indicate coverage.
import os
import shutil
import re
import covCommon

HighestInstrLineIndex = 0
InstrLineIndex = 0
DisplayWarnings = False

# Copy from a project source directory and instrument the source files. 
def covInstr():
    covInstrFiles(covCommon.srcPath, covCommon.outDir, covCommon.coverageSrcPath)


# Instrument every file ending with .cpp in the srcPath.
# Simply copy all other files such as project files.
def covInstrFiles(srcPath, outDir, coverageSrcPath):
    fileNum = 0
    for (dirPath, dirNames, fileNames) in os.walk(srcPath):
        for srcFn in fileNames:
            fullSrcPath = dirPath + '/' + srcFn
            if covCommon.filterFiles(srcFn):
                print fullSrcPath
                covInstrSourceFile(outDir, fullSrcPath, fileNum)
                fileNum += 1
            else:
                outFn = prepDir(outDir, fullSrcPath)
                shutil.copyfile(fullSrcPath, outFn)
    outputCoverageHeader(coverageSrcPath, fileNum, HighestInstrLineIndex)
    outputCoverageArray(coverageSrcPath, fileNum, HighestInstrLineIndex)


# Instrument a source file by copying to an output directory.
# This does not do any comment or preproc parsing.
# This uses a very simple rule to determine if the braces are in a
# statement initialization. It only checks the previous line for ending with '='
def covInstrSourceFile(outDir, srcFn, fileNum):
    global InstrLineIndex
    InstrLineIndex = 0
    braceLevel = 0
    lineNum = 0
    startInStatementLevel = 0
    inStatement = False
    skipBraceLevel = False
    inStructOrSwitch = False
    inf = open(srcFn, 'r')

    outFn = prepDir(outDir, srcFn)

    outf = open(outFn, 'w')
    outf.write("#include \"coverage.h\"\n")
    prevLine = ""
    for origLine in inf:
        codeLine = origLine  # codeLine does not contain comment lines. "//"
        outLine = origLine
        if "//" in origLine:
            codeLine = origLine[0:origLine.find("//")] + "\n"
        lineNum += 1
        if braceLevel == startInStatementLevel:
            inStatement = False
        inc = codeLine.count('{')
        dec = codeLine.count('}')
        if(checkSingleLineConditionalAndStatement(codeLine)):
            outLine = instrExistingLinePlusBraces(codeLine, \
                findSingleLineConditionalEnd(codeLine), fileNum)
            instrSingleLine = False
        else:
            instrSingleLine = checkInstrConditionalAndSingleLineStatement(prevLine, codeLine)
        if(inc + dec > 1):
            if(re.match('^\s*\{.*\}\s*$', codeLine) and not prevLine.rstrip().endswith(',')):
                outLine = instrExistingLine(codeLine, codeLine.index('{')+1, fileNum)
            elif DisplayWarnings:
                print("Too many braces on a line: " + srcFn, lineNum, codeLine)
            inStatement = True       # Prevent brace instrumentation below
        if dec:
            braceLevel -= dec
        if instrSingleLine:
            outf.write("{\n")
            instrNewLine(outf, fileNum)
        if checkInstrCaseDefault(codeLine):
            outLine = instrExistingLine(codeLine, codeLine.index(':')+1, fileNum)
        outf.write(outLine)
        if instrSingleLine:
            outf.write("}\n")
        if inc:
            braceLevel += inc
            regDataPat = '(^|[^A-Za-z_])enum([^A-Za-z_]|$)'
            # If this line has open brace and prev line ended with '=', or
            # This or prev line contains a data start keyword, then
            # don't instrument any nested braces.
            if prevLine.rstrip().endswith("=") or re.search(regDataPat, codeLine) or \
                re.search(regDataPat, prevLine):
                inStatement = True
                startInStatementLevel = braceLevel-1
            # If this line has open brace and prev line has switch, then
            # don't instrument current brace level.
            # This pattern attempts to avoid some casts.
            if re.search('(^|[^A-Za-z_<\(])(class|struct|switch)([^A-Za-z_]|$)', prevLine):
                skipBraceLevel = True
            else:
                skipBraceLevel = False
            if not inStatement and not skipBraceLevel:
                instrNewLine(outf, fileNum)
        prevLine = codeLine
    inf.close()
    outf.close()
    if braceLevel != 0:
        print("Brace count error: " + fn)

def checkSingleLineConditionalAndStatement(line):
    if(re.search('(^|\s)(if|for|while)\s*\(', line)):
        index = findSingleLineConditionalEnd(line)
        if(index > 0 and re.search(';', line[index:])):
            return True
    return False

def findSingleLineConditionalEnd(line):
    nest = 0
    opened = False
    for i in range(0, len(line)):
        if(line[i] == '('):
            opened = True
            nest+=1
        elif(line[i] == ')' and opened):
            nest-=1
            if(nest == 0):
                return i+1
    return 0

def checkInstrConditionalAndSingleLineStatement(prevLine, line):
    instrSingleLine = False
    if ((re.search('(^|\s)(if|for|while)\s*\(', prevLine) or \
        re.search('\selse\s*', prevLine)) and   \
        prevLine.count('{') == 0):
        if(line.count(';') and line.count('{') == 0):
            instrSingleLine = True
        if(re.search('\sfor\s*\(', prevLine) and prevLine.count(';') != 2):
            instrSingleLine = False
    return instrSingleLine

def checkInstrCaseDefault(line):
    return (re.search('case[^A-Za-z_].*:', line) or re.search('default\s*:', line))

def instrNewLine(outf, fileNum):
    global InstrLineIndex
    outf.write("COV_IN(" + str(fileNum) + "," + str(InstrLineIndex) + ")\n")
    instrLine(fileNum)

def instrExistingLinePlusBraces(codeLine, insertIndex, fileNum):
    global InstrLineIndex
    codeLine = codeLine[:insertIndex] + \
        "{ COV_IN(" + str(fileNum) + "," + str(InstrLineIndex) + ")" + \
        codeLine[insertIndex:] + "}\n"
    instrLine(fileNum)
    return codeLine

def instrExistingLine(codeLine, insertIndex, fileNum):
    global InstrLineIndex
    codeLine = codeLine[:insertIndex] + \
        "COV_IN(" + str(fileNum) + "," + str(InstrLineIndex) + ")" + \
        codeLine[insertIndex:]
    instrLine(fileNum)
    return codeLine

def instrLine(fileNum):
    global HighestInstrLineIndex
    global InstrLineIndex
    InstrLineIndex += 1
    if InstrLineIndex > HighestInstrLineIndex:
        HighestInstrLineIndex = InstrLineIndex

def prepDir(outDir, srcFn):
    outFn = outDir + srcFn
    outFn = outFn.replace("../", "").replace("//", "/")
    outDir = outFn[0:outFn.rfind("/")]
    if not os.path.exists(outDir):
        os.makedirs(outDir)
    return outFn

def outputCoverageHeader(coverageSrcPath, numFiles, maxLines):
    outf = open(coverageSrcPath + "/coverage.h", 'w')
    outf.write("extern unsigned short gCoverage[" + str(numFiles) + "][" + str(maxLines) + "];\n")
    outf.write("#define COV_IN(fileIndex, lineIndex) gCoverage[fileIndex][lineIndex]++;\n")
    outf.close()
    
def outputCoverageArray(coverageSrcPath, numFiles, maxLines):
    outf = open(coverageSrcPath + "/coverage.cpp", 'w')
    outf.write("// This file is automatically generated\n")
    outf.write("unsigned short gCoverage[" + str(numFiles) + "][" + str(maxLines) + "];\n")
    lines = "#include <stdio.h>\n" \
        "class cCoverageOutput\n"   \
        "  {\n" \
        "  public:\n"   \
	"  ~cCoverageOutput()\n" \
	"    {\n" \
	"      update();\n" \
	"    }\n"   \
        "  void update()\n" \
	"    {\n" \
	"    read();\n" \
	"    write();\n" \
	"    }\n"   \
        "  void read()\n" \
        "    {\n"   \
        "    FILE *fp = fopen(\"coverageStats.txt\", \"r\");\n" \
	"    if(fp)\n"    \
	"      {\n"   \
	"      int maxLines = 0;\n"   \
	"      int numFiles = 0;\n"   \
	"      fscanf(fp, \"%d%*[^\\n]\", &numFiles);\n"    \
	"      fscanf(fp, \"%d%*[^\\n]\", &maxLines);\n"    \
	"      if(numFiles == " + str(numFiles) + " && maxLines == " + str(maxLines) + ")\n"   \
	"        {\n" \
	"        for(int fi=0; fi<" + str(numFiles) + "; fi++)\n" \
	"          {\n"   \
	"          for(int li=0; li<" + str(maxLines) + "; li++)\n"    \
	"            {\n" \
        "            unsigned int val;\n" \
        "            if(li == 0)    // discard file index\n" \
        "               fscanf(fp, \"\%u%*[^\\n]\", &val);\n" \
	"            fscanf(fp, \"%u\", &val);\n" \
        "            gCoverage[fi][li] += val;\n" \
        "            }\n" \
	"          }\n"   \
	"        }\n" \
	"      fclose(fp);\n"   \
	"      }\n"   \
        "    }\n" \
        "  void write()\n" \
	"    {\n" \
	"    FILE *fp = fopen(\"coverageStats.txt\", \"w\");\n"   \
	"    fprintf(fp, \"%d   # Number of files\\n\", " + str(numFiles) + ");\n"  \
	"    fprintf(fp, \"%d   # Max number of instrumented lines per file\\n\", " + str(maxLines) + ");\n"  \
	"    for(int fi=0; fi<" + str(numFiles) + "; fi++)\n" \
        "      {\n"   \
	"      for(int li=0; li<" + str(maxLines) + "; li++)\n"    \
        "        {\n"   \
	"          if(li == 0)  // add file index for reference (not used)\n" \
	"            fprintf(fp, \"%d   # File Index\\n\", fi);\n" \
	"          fprintf(fp, \"%u\", gCoverage[fi][li]);\n" \
        "          fprintf(fp, \"\\n\");\n" \
	"        }\n"   \
	"      }\n"   \
	"    fclose(fp);\n"   \
	"    }\n"   \
	"  };\n"   \
        "cCoverageOutput coverageOutput;\n"

    outf.write(lines)
    outf.close()
