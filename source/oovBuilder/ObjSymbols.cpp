/*
 * ObjSymbols.cpp
 *
 *  Created on: Sep 13, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ObjSymbols.h"
#include <string>
#include <stdio.h>
#include <algorithm>
#include "OovProcess.h"
#include "ComponentBuilder.h"

class FileSymbol
    {
    public:
	FileSymbol(OovStringRef const symbolName, int len, int fileIndex):
	    mSymbolName(symbolName, len), mFileIndex(fileIndex)
	    {}
	// For sorting and searching
	bool operator<(const FileSymbol &rh) const
	    { return(mSymbolName < rh.mSymbolName); }
	OovString mSymbolName;
	int mFileIndex;
    };

// The first is the client file index, and the second is a container of supplier file indices.
class FileDependencies:public std::map<int, std::set<int> >
    {
    public:
	void addDependency(int clientIndex, int supplierIndex);
	// Check if the file index passed in is dependent on any other files.
	bool dependentOnAny(int fileIndex) const;
	// Remove all references to the file index.
	void removeValue(int fileIndex);

    private:
	// Remove a client if there are no suppliers
	bool removeEmptyClient();
    };

void FileDependencies::addDependency(int clientIndex, int supplierIndex)
    {
    auto iter = find(clientIndex);
    if(iter == end())
	{
	std::set<int> suppliers;
	suppliers.insert(supplierIndex);
	insert(std::pair<int, std::set<int> >(clientIndex, suppliers));
	}
    else
	{
	iter->second.insert(supplierIndex);
	}
    }

bool FileDependencies::dependentOnAny(int clientIndex) const
    {
    return(find(clientIndex) != end());
    }

void FileDependencies::removeValue(int fileIndex)
    {
    for(auto &dep : *this)
	{
	dep.second.erase(fileIndex);
	}
    while(removeEmptyClient())
	{
	}
    }

bool FileDependencies::removeEmptyClient()
    {
    bool removed = false;
    for(auto dep=begin(); dep != end(); dep++)
	{
	if((*dep).second.size() == 0)
	    {
	    erase(dep);
	    removed = true;
	    break;
	    }
	}
    return removed;
    }


class FileSymbols:public std::set<FileSymbol>
    {
    public:
	void add(OovStringRef const symbolName, int len, int fileIndex)
	    { insert(FileSymbol(symbolName, len, fileIndex)); }
	std::set<FileSymbol>::iterator findName(OovStringRef const symbolName)
	    {
	    FileSymbol fs(symbolName, std::string(symbolName).length(), -1);
	    return find(fs);
	    }
	bool writeSymbols(OovStringRef const symFileName);
    };

bool FileSymbols::writeSymbols(OovStringRef const symFileName)
    {
    FILE *outFp = fopen(symFileName, "w");
    if(outFp)
	{
	for(const auto &symbol : *this)
	    {
	    fprintf(outFp, "%s %d\n", symbol.mSymbolName.getStr(), symbol.mFileIndex);
	    }
	fclose(outFp);
	}
    return(outFp != NULL);
    }


class FileList:public OovStringVec
    {
    public:
	void writeFile(FILE *outFp) const;
    };

void FileList::writeFile(FILE *outFp) const
    {
    for(const auto &fn : (*this))
	{
	fprintf(outFp, "f:%s\n", fn.getStr());
	}
    }


class FileIndices:public std::vector<int>
    {
    public:
	void removeValue(int val);
	iterator findValue(int val);
    };


class ClumpSymbols
    {
    public:
	// Parse files for:
	// U referenced objects, but not defined
	// D global data object
	// T global function object
	void addSymbols(OovStringRef const libFilePath, OovStringRef const outFileName);
	void writeClumpFiles(OovStringRef const clumpName,
		OovStringRef const outPath);
    private:
	FileSymbols mDefinedSymbols;
	FileSymbols mUndefinedSymbols;
	FileDependencies mFileDependencies;
	FileList mFileIndices;
	FileIndices mOrderedDependencies;
        std::mutex mDataMutex;
	void resolveUndefinedSymbols();
	void readRawSymbolFile(OovStringRef const outRawFileName, int fileIndex);
    };


void FileIndices::removeValue(int val)
    {
    const auto &pos = findValue(val);
    if(pos != end())
	erase(pos);
    }

std::vector<int>::iterator FileIndices::findValue(int val)
    {
    iterator retiter = end();
    for(iterator iter=begin(); iter!=end(); ++iter)
	{
	if(val == (*iter))
	    {
	    retiter = iter;
	    break;
	    }
	}
    return(retiter);
    }


// The .dep file is the master file. The filenames are listed first starting
// with "f:". The are listed in order, so the first file is file index 0.
// Then the "o:" indicates the order that the files should be linked.
static bool writeDepFileInfo(OovStringRef const symFileName, const FileList &fileIndices,
	const FileDependencies &fileDeps, const FileIndices &orderedDeps)
    {
    FILE *outFp = fopen(symFileName, "w");
    if(outFp)
	{
	fileIndices.writeFile(outFp);
	for(const auto &dep : fileDeps)
	    {
	    fprintf(outFp, "d:%d ", dep.first);
	    for(const auto &sup : dep.second)
		{
		fprintf(outFp, "%d ", sup);
		}
	    fprintf(outFp, "\n");
	    }
	fprintf(outFp, "o:");
	for(const auto &dep : orderedDeps)
	    {
	    fprintf(outFp, "%d ", dep);
	    }
	fprintf(outFp, "\n");
	fclose(outFp);
	}
    return(outFp != NULL);
    }

static void orderDependencies(size_t numFiles, const FileDependencies &fileDependencies,
	FileIndices &orderedDependencies)
    {
    FileDependencies fileDeps = fileDependencies;
    FileIndices origFiles;
    for(size_t i=0; i<numFiles; i++)
	{
	origFiles.push_back(i);
	}
    // Go through all libraries
    for(size_t totaliter=0; totaliter<numFiles; totaliter++)
	{
	if(origFiles.size() == 0)
	    break;
	// If any library is not dependent on any others, then remove it
	// as a client or provider for all libraries.
	for(size_t i=0; i<origFiles.size(); i++)
	    {
	    int fileIndex = origFiles[i];
	    if(!fileDeps.dependentOnAny(fileIndex))
		{
		orderedDependencies.insert(orderedDependencies.begin()+0, fileIndex);
		origFiles.removeValue(fileIndex);
		fileDeps.removeValue(fileIndex);
		break;
		}
	    }
	}
    // If there were any left over, there were circular dependencies.
    for(size_t i=0; i<origFiles.size(); i++)
	{
	int fileIndex = origFiles[i];
	orderedDependencies.insert(orderedDependencies.begin()+0, fileIndex);
	}
    }

void ClumpSymbols::readRawSymbolFile(OovStringRef const outRawFileName, int fileIndex)
    {
    FILE *inFp = fopen(outRawFileName, "r");
    if(inFp)
	{
	char buf[2000];
	while(fgets(buf, sizeof(buf), inFp))
	    {
	    char const *p = buf;
	    while(isspace(*p) || isxdigit(*p))
		p++;
	    char symTypeChar = *p;

	    p++;
	    while(isspace(*p))
		p++;
	    char const *endP = p;
	    while(!isspace(*endP))
		endP++;
	    if(symTypeChar == 'D' || symTypeChar == 'T')
		{
		std::unique_lock<std::mutex> lock(mDataMutex);
		mDefinedSymbols.add(p, endP-p, fileIndex);
		}
	    else if(symTypeChar == 'U')
		{
		std::unique_lock<std::mutex> lock(mDataMutex);
		mUndefinedSymbols.add(p, endP-p, fileIndex);
		}
	    }
	fclose(inFp);
	}
    }

void ClumpSymbols::resolveUndefinedSymbols()
    {
    for(const auto &sym : mDefinedSymbols)
	{
	auto pos = mUndefinedSymbols.findName(sym.mSymbolName);
	if(pos != mUndefinedSymbols.end())
	    {
	    // The file indices can be the same if one library has
	    // an object file where it is defined, and another object
	    // file where it is not defined.
	    if((*pos).mFileIndex != sym.mFileIndex)
		{
		mFileDependencies.addDependency((*pos).mFileIndex, sym.mFileIndex);
		}
	    mUndefinedSymbols.erase(pos);
	    }
	}
    }

void ClumpSymbols::addSymbols(OovStringRef const libFilePath, OovStringRef const outRawFileName)
    {
	{
	std::unique_lock<std::mutex> lock(mDataMutex);
	mFileIndices.push_back(libFilePath);
	}
    readRawSymbolFile(outRawFileName, mFileIndices.size()-1);
    }

void ClumpSymbols::writeClumpFiles(OovStringRef const clumpName,
	OovStringRef const outPath)
    {
    resolveUndefinedSymbols();
    orderDependencies(mFileIndices.size(), mFileDependencies, mOrderedDependencies);

    OovString defSymFileName = outPath;
    OovString undefSymFileName = defSymFileName;
    OovString depSymFileName = defSymFileName;
    undefSymFileName += "LibSym-";
    undefSymFileName += clumpName;
    undefSymFileName += "-Undef.txt";

    defSymFileName += "LibSym-";
    defSymFileName += clumpName;
    defSymFileName += "-Def.txt";

    depSymFileName += "LibSym-";
    depSymFileName += clumpName;
    depSymFileName += "-Dep.txt";
    mDefinedSymbols.writeSymbols(defSymFileName);
    mUndefinedSymbols.writeSymbols(undefSymFileName);
    writeDepFileInfo(depSymFileName, mFileIndices, mFileDependencies,
	    mOrderedDependencies);
    }

class ObjTaskListener:public TaskQueueListener
    {
    public:
	ObjTaskListener(ClumpSymbols &clumpSymbols):
	    mClumpSymbols(clumpSymbols)
	    {}
    private:
	ClumpSymbols &mClumpSymbols;
	virtual void extraProcessing(bool success, OovStringRef const outFile,
		OovStringRef const stdOutFn, ProcessArgs const &item) override
	    {
	    if(success)
		{
		mClumpSymbols.addSymbols(item.mLibFilePath, stdOutFn);
		}

	    }
    };

bool ObjSymbols::makeObjectSymbols(OovStringVec const &libFiles,
	OovStringRef const outSymPath, OovStringRef const objSymbolTool,
	ComponentTaskQueue &queue, ClumpSymbols &clumpSymbols)
    {
    struct LibFileNames
	{
	LibFileNames(OovString const &libFileName,
		OovString const &libSymFileName,
		OovString const &libFilePath, bool genSymbolFile):
	    mLibFileName(libFileName), mLibFilePath(libFilePath),
	    mLibSymFileName(libSymFileName), mGenSymbolFile(genSymbolFile)
	    {}
	OovString mLibFileName;
	OovString mLibFilePath;
	OovString mLibSymFileName;
	bool mGenSymbolFile;
	};
    bool generatedSymbols = false;
    std::vector<LibFileNames> libFileNames;
    for(const auto &libFilePath : libFiles)
	{
	FilePath libSymName(libFilePath, FP_File);
	libSymName.discardDirectory();
	libSymName.discardExtension();
	OovString libSymPath = outSymPath;
	ensureLastPathSep(libSymPath);
	ensurePathExists(libSymPath);
	OovString outSymRawFileName = libSymPath + libSymName + ".txt";
/*
	std::string libName = libFilePath;
	size_t libNamePos = rfindPathSep(libName);
	if(libNamePos != std::string::npos)
	    libNamePos++;
	else
	    libNamePos = 0;
	libName.insert(libNamePos, "lib");
	std::string libFileName = // std::string(outLibPath) +
	    libName + ".a";
*/
	if(FileStat::isOutputOld(outSymRawFileName, libFilePath))
	    {
	    libFileNames.push_back(LibFileNames(libFilePath, outSymRawFileName,
		    libFilePath, true));
	    generatedSymbols = true;
	    }
	else
	    {
	    libFileNames.push_back(LibFileNames(libFilePath, outSymRawFileName,
		    libFilePath, false));
	    }
	}

    if(generatedSymbols)
	{
#define MULTI_THREAD 1
#if(MULTI_THREAD)
	ObjTaskListener listener(clumpSymbols);
	queue.setTaskListener(&listener);
	queue.setupQueue(queue.getNumHardwareThreads());
#endif
	for(auto const &libFn : libFileNames)
	    {
	    if(libFn.mGenSymbolFile)
		{
		OovProcessChildArgs ca;
		ca.addArg(objSymbolTool);
		std::string quotedLibFilePath = libFn.mLibFilePath;
		quoteCommandLinePath(quotedLibFilePath);
		ca.addArg(quotedLibFilePath);

		ProcessArgs procArgs(objSymbolTool, libFn.mLibSymFileName, ca,
			libFn.mLibSymFileName.getStr());
		procArgs.mLibFilePath = libFn.mLibFilePath;
#if(MULTI_THREAD)
		queue.addTask(procArgs);
#else
		success = ComponentBuilder::runProcess(objSymbolTool,
		    outRawFileName, ca, mListenerStdMutex, outRawFileName);
		if(success)
		    clumpSymbols.addSymbols(libFilePath, outRawFileName);
#endif
		}
	    else
		{
		clumpSymbols.addSymbols(libFn.mLibFilePath,
			libFn.mLibSymFileName);
		}
	    }
#if(MULTI_THREAD)
	queue.waitForCompletion();
	queue.setTaskListener(nullptr);
#endif
	}
    return generatedSymbols;
    }

bool ObjSymbols::makeClumpSymbols(OovStringRef const clumpName,
	OovStringVec const &libFiles, OovStringRef const outSymPath,
	OovStringRef const objSymbolTool, ComponentTaskQueue &queue)
    {
    bool success = true;
    ClumpSymbols clumpSymbols;
//    std::string defSymFileName = outSymPath;
//    ensureLastPathSep(defSymFileName);

//    defSymFileName += "LibSym-";
//    defSymFileName += clumpName;
//    defSymFileName += "-Def.txt";
    if(//!fileExists(defSymFileName) ||
	    makeObjectSymbols(libFiles, outSymPath, objSymbolTool, queue, clumpSymbols))
	{
	clumpSymbols.writeClumpFiles(clumpName, outSymPath);
	}
    return success;
    }

void ObjSymbols::appendOrderedLibFileNames(OovStringRef const clumpName,
	OovStringRef const outPath,
	OovStringVec &sortedLibFileNames)
    {
    OovString depSymFileName = outPath;
    ensureLastPathSep(depSymFileName);
    depSymFileName += "LibSym-";
    depSymFileName += clumpName;
    depSymFileName += "-Dep.txt";

    FILE *fp = fopen(depSymFileName.getStr(), "r");
    if(fp)
	{
	std::vector<std::string> filenames;
	std::vector<int> indices;
	char buf[500];
	while(fgets(buf, sizeof(buf), fp))
	    {
	    if(buf[0] == 'f')
		{
		std::string fn = &buf[2];
		size_t pos = fn.rfind('\n');
		if(pos)
		    fn.resize(pos);
		filenames.push_back(fn);
		}
	    else if(buf[0] == 'o')
		{
		char const *p = &buf[2];
		while(*p)
		    {
		    if(isdigit(*p))
			{
			int num;
			if(sscanf(p, "%d", &num) == 1)
			    {
			    indices.push_back(num);
			    while(isdigit(*p))
				p++;
			    }
			}
		    else
			p++;
		    }
		}
	    }
	for(const auto &fileIndex : indices)
	    {
	    sortedLibFileNames.push_back(filenames[fileIndex]);
	    }
	fclose(fp);
	}
    }

void ObjSymbols::appendOrderedLibs(OovStringRef const clumpName,
	OovStringRef const outPath, OovStringVec &libDirs,
	OovStringVec &orderedLibNames)
    {
    OovStringVec orderedLibFileNames;
    appendOrderedLibFileNames(clumpName, outPath, orderedLibFileNames);
    OovStringSet dirs;

    for(auto const &lib : orderedLibFileNames)
	{
	FilePath fp(lib, FP_File);
	orderedLibNames.push_back(fp.getNameExt());
	dirs.insert(fp.getDrivePath());
	}
    std::copy(dirs.begin(), dirs.end(), std::back_inserter(libDirs));
    }
