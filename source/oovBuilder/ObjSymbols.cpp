/*
 * ObjSymbols.cpp
 *
 *  Created on: Sep 13, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ObjSymbols.h"
#include <string>
#include <stdio.h>
#include "OovProcess.h"
#include "ComponentBuilder.h"

class FileSymbol
    {
    public:
	FileSymbol(char const * const symbolName, int len, int fileIndex):
	    mSymbolName(symbolName, len), mFileIndex(fileIndex)
	    {}
	// For sorting and searching
	bool operator<(const FileSymbol &rh) const
	    { return(mSymbolName < rh.mSymbolName); }
	std::string mSymbolName;
	int mFileIndex;
    };

class FileSymbols:public std::set<FileSymbol>
    {
    public:
	void add(char const * const symbolName, int len, int fileIndex)
	    { insert(FileSymbol(symbolName, len, fileIndex)); }
	std::set<FileSymbol>::iterator findName(char const * const symbolName)
	    {
	    FileSymbol fs(symbolName, std::string(symbolName).length(), -1);
	    return find(fs);
	    }
    };

class FileList:public std::vector<std::string>
    {
    public:
	void writeFile(FILE *outFp) const
	    {
	    for(const auto &fn : (*this))
		{
		fprintf(outFp, "f:%s\n", fn.c_str());
		}
	    }
    };

class FileIndices:public std::vector<int>
    {
    public:
	void removeValue(int val)
	    {
	    const auto &pos = findValue(val);
	    if(pos != end())
		erase(pos);
	    }
	iterator findValue(int val)
	    {
	    iterator retiter = end();
	    for(iterator iter=begin(); iter!=end(); iter++)
		{
		if(val == (*iter))
		    {
		    retiter = iter;
		    break;
		    }
		}
	    return(retiter);
	    }
    };

// first file (client) is dependent on the second file (supplier) that
// defines the symbol
class FileDependencies:public std::map<int, int>
    {
    public:
	bool dependentOnAny(int fileIndex) const
	    {
	    bool dependent = false;
	    for(const auto &pos : *this)
		{
		if(pos.first == fileIndex)
		    {
		    dependent = true;
		    break;
		    }
		}
	    return dependent;
	    }
	void removeValue(int val)
	    {
	    while(1)
		{
		const auto &pos = findSecond(val);
		if(pos != end())
		    erase(pos);
		else
		    break;
		}
	    }
	iterator findSecond(int val)
	    {
	    iterator retiter = end();
	    for(iterator iter=begin(); iter!=end(); iter++)
		{
		if(val == (*iter).second)
		    {
		    retiter = iter;
		    break;
		    }
		}
	    return retiter;
	    }
    };

static bool writeSymbols(char const * const symFileName, const FileSymbols &symbols)
    {
    FILE *outFp = fopen(symFileName, "w");
    if(outFp)
	{
	for(const auto &symbol : symbols)
	    {
	    fprintf(outFp, "%s %d\n", symbol.mSymbolName.c_str(), symbol.mFileIndex);
	    }
	fclose(outFp);
	}
    return(outFp != NULL);
    }

static bool writeFileInfo(char const *const symFileName, const FileList &fileIndices,
	const FileDependencies &fileDeps, const FileIndices &orderedDeps)
    {
    FILE *outFp = fopen(symFileName, "w");
    if(outFp)
	{
	fileIndices.writeFile(outFp);
	for(const auto &dep : fileDeps)
	    {
	    fprintf(outFp, "d:%d %d\n", dep.first, dep.second);
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
    for(size_t totaliter=0; totaliter<numFiles; totaliter++)
	{
	if(origFiles.size() == 0)
	    break;
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

bool ObjSymbols::makeObjectSymbols(char const * const clumpName,
	const std::vector<std::string> &libFiles, char const * const outPath,
	char const * const objSymbolTool)
    {
    FileSymbols definedSymbols;
    FileSymbols undefinedSymbols;
    FileDependencies fileDependencies;
    FileList fileIndices;
    bool success = true;
    std::string defSymFileName = outPath;
    ensureLastPathSep(defSymFileName);

    std::string undefSymFileName = defSymFileName;
    std::string depSymFileName = defSymFileName;
    undefSymFileName += "LibSym-";
    undefSymFileName += clumpName;
    undefSymFileName += "-Undef.txt";

    defSymFileName += "LibSym-";
    defSymFileName += clumpName;
    defSymFileName += "-Def.txt";

    depSymFileName += "LibSym-";
    depSymFileName += clumpName;
    depSymFileName += "-Dep.txt";
    if(!fileExists(defSymFileName.c_str()) || mForceUpdateSymbols)
	{
	for(const auto &libFilePath : libFiles)
	    {
	    FilePath libName(libFilePath, FP_File);
	    libName.discardDirectory();
	    libName.discardExtension();
	    std::string libSymPath = outPath;
	    ensureLastPathSep(libSymPath);
	    ensurePathExists(libSymPath.c_str());
	    std::string outFileName = libSymPath + "lib" + libName + ".a";
	    std::string outRawFileName = libSymPath + libName + ".txt";

	    OovProcessChildArgs ca;
	    ca.addArg(objSymbolTool);
	    std::string quotedLibFilePath = libFilePath;
	    quoteCommandLinePath(quotedLibFilePath);
	    ca.addArg(quotedLibFilePath.c_str());
	    success = ComponentBuilder::runProcess(objSymbolTool,
		    outRawFileName.c_str(), ca, outRawFileName.c_str());
	    // Now parse files for:
	    // U referenced objects, but not defined
	    // D global data object
	    // T global function object
	    if(success)
		{
		fileIndices.push_back(libFilePath.c_str());
		FILE *inFp = fopen(outRawFileName.c_str(), "r");
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
			    definedSymbols.add(p, endP-p, fileIndices.size()-1);
			    }
			else if(symTypeChar == 'U')
			    {
			    undefinedSymbols.add(p, endP-p, fileIndices.size()-1);
			    }
			}
		    fclose(inFp);
		    deleteFile(outRawFileName.c_str());
		    }
		}
	    }
	if(success)
	    {
	    for(const auto &sym : definedSymbols)
		{
		auto pos = undefinedSymbols.findName(sym.mSymbolName.c_str());
		if(pos != undefinedSymbols.end())
		    {
		    // The file indices can be the same if one library has
		    // an object file where it is defined, and another object
		    // file where it is not defined.
		    if((*pos).mFileIndex != sym.mFileIndex)
			{
			fileDependencies.insert(std::make_pair((*pos).mFileIndex,
			    sym.mFileIndex));
			}
		    undefinedSymbols.erase(pos);
		    }
		}
	    FileIndices orderedDependencies;
	    orderDependencies(fileIndices.size(), fileDependencies, orderedDependencies);

	    writeSymbols(defSymFileName.c_str(), definedSymbols);
	    writeSymbols(undefSymFileName.c_str(), undefinedSymbols);
	    writeFileInfo(depSymFileName.c_str(), fileIndices, fileDependencies,
		    orderedDependencies);
	    }
	}
    return success;
    }

void ObjSymbols::appendOrderedLibFileNames(char const * const clumpName,
	char const * const outPath,
	std::vector<std::string> &sortedLibFileNames)
    {
    std::string depSymFileName = outPath;
    ensureLastPathSep(depSymFileName);
    depSymFileName += "LibSym-";
    depSymFileName += clumpName;
    depSymFileName += "-Dep.txt";

    FILE *fp = fopen(depSymFileName.c_str(), "r");
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

void ObjSymbols::appendOrderedLibs(char const * const clumpName,
	char const * const outPath, std::vector<std::string> &libDirs,
	std::vector<std::string> &orderedLibNames)
    {
    std::vector<std::string> orderedLibFileNames;
    appendOrderedLibFileNames(clumpName, outPath, orderedLibFileNames);
    std::set<std::string> dirs;

    for(auto const &lib : orderedLibFileNames)
	{
	FilePath fp(lib, FP_File);
	orderedLibNames.push_back(fp.getNameExt());
	dirs.insert(fp.getDrivePath());
	}
    std::copy(dirs.begin(), dirs.end(), std::back_inserter(libDirs));
    }
