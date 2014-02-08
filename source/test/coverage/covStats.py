# Copyright 2013 DCBlaha.  Distributed under the GPL.

import os
import covCommon

HighestInstrLineIndex = 0

def covStats():
    fileNum = 0
    coverage = getCoverage(covCommon.coverageStatsRelPath)
    totalHits = 0
    totalInstrLines = 0
    for (dirPath, dirNames, fileNames) in os.walk(covCommon.srcPath):
        for srcFn in fileNames:
            fullSrcPath = dirPath + '/' + srcFn
            if covCommon.filterFiles(srcFn):
                outFn = prepDir(covCommon.outDir, fullSrcPath)
                (numHits, numInstrLines) = covStatsFile(outFn, fileNum, coverage)
                totalHits += numHits
                totalInstrLines += numInstrLines
                fileNum += 1
    # This is not the % of lines covered. It is % of hits.
    print "Total:", totalHits, '/', totalInstrLines, str(totalHits * 100 / totalInstrLines) + "%"

# This returns the stats for one source file from the coverage stats file.
def getCoverageStatsForFile(fileNum, coverage):
    maxInstrLines = coverage[1]
    covheaderlines = 2
    srcheaderlines = 1
    start = covheaderlines + srcheaderlines + (fileNum * (maxInstrLines + srcheaderlines))
    return coverage[start:start + maxInstrLines]

def covStatsFile(srcFn, fileNum, coverage):
    srcf = open(srcFn, 'r')
    dstf = open(srcFn.replace('.', '_')+".txt", 'w')
    numInstrLines = 0
    fileStats = getCoverageStatsForFile(fileNum, coverage)
    for line in srcf.readlines():
        if line.count("COV_IN"):
            line = line[0:line.find('\n')]
            line += '\t\t// ' + str(fileStats[numInstrLines]) + '\n'
            numInstrLines += 1
        dstf.write(line)
    srcf.close()
    dstf.close()
    numHits = 0
    for x in fileStats:
        if x != 0:
            numHits += 1
    if numInstrLines != 0:
        percent = numHits * 100 / numInstrLines
    else:
        percent = 100
    print srcFn, numHits, numInstrLines, str(percent) + "%"
    return (numHits, numInstrLines)

def prepDir(outDir, srcFn):
    outFn = outDir + srcFn
    outFn = outFn.replace("../", "").replace("//", "/")
    outDir = outFn[0:outFn.rfind("/")]
    if not os.path.exists(outDir):
        os.makedirs(outDir)
    return outFn

# Foverage file format:
# First line is number of files.
# Second line is maximum number of instrumented lines per file.
# Then for every file, the first line is the file index, followed
# by a counter for each instrumented line.
def getCoverage(fn):
    values = []
    f = open(fn, 'r')
    for line in f.readlines():
        tokens = line.split()
        values.append(int(tokens[0]))
    f.close()
    return values
