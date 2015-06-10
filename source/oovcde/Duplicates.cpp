/*
 * Duplicates.cpp
 * Created on: Feb 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */
#include "Project.h"
#include "DirList.h"
#include "Duplicates.h"
#include <memory.h>


/// This prevents output of duplicate comparisons. For example:
/// File1         File2
/// ..ABCD...    ..........ABCD...BCE.
/// HashIndices
///              ..........2345...34..
///
/// The comparison should not output: ABCD, BCD, CD, D,    BC, C
class cFilterOutputHashIndices
    {
    public:
        void init(size_t size)
            {
            if(size > mFile2HashIndices.size())
                {
                mFile2HashIndices.resize(size);
                }
            memset(&mFile2HashIndices[0], 0, size);
            }
        void markAlreadyOutput(size_t startIndexFile1, size_t len,
            size_t startIndexFile2)
            {
            for(size_t i=0; i<len; i++)
                {
                mFile2HashIndices[startIndexFile2+i] = startIndexFile1+i;
                }
            }
        bool isAlreadyOutput(size_t startIndexFile1, size_t startIndexFile2)
            {
            return(mFile2HashIndices[startIndexFile2] == startIndexFile1);
            }

    private:
        std::vector<size_t> mFile2HashIndices;
    };


class HashItem
    {
    public:
	HashItem():
	    mHash(0), mLineNum(0)
	    {}
    size_t mHash;
    size_t mLineNum;
    };

class DupOptions
    {
    public:
	DupOptions():
	    mFindDupsInLines(false), mNumTokenMatches(4)
	    {}
	// The default for dups in lines is false since it is fairly rare, but when it
	// does happen, can create huge dup files.
        // See LLVM_DEFINE_OVERLOAD in VariadicFunction.h in the LLVM project for an example.
	bool mFindDupsInLines;
	size_t mNumTokenMatches;
    };

class HashFile
    {
    public:
        bool readHashFile(OovStringRef filePath);
        void compareHashFiles(HashFile const &refFile,
            cFilterOutputHashIndices &outHashIndices, DupOptions const &options,
            File &outFile);

    private:
        OovString mFilePath;
        std::vector<HashItem> mHashItems;

        /// Returns the number of hash lines that match.
        size_t getDupLinesLength(size_t thisStartIndex, HashFile const &refFile,
            size_t refFileStartIndex);
        OovString getActualFileName() const;
        OovString getRelativeFileName() const;
    };

bool HashFile::readHashFile(OovStringRef const filePath)
    {
    mFilePath = filePath;
    FILE *fp = fopen(filePath.getStr(), "r");
    if(fp)
        {
        char buf[60];
        while(fgets(buf, sizeof(buf), fp))
            {
            HashItem item;
            if(strlen(buf) > 0)
                {
#ifdef __linux__
                sscanf(buf, "%lx %lu", &item.mHash, &item.mLineNum);
#else
                sscanf(buf, "%x %u", &item.mHash, &item.mLineNum);
#endif
                }
            /// @todo - This could be more efficient if we new the
            /// file size.
            mHashItems.push_back(item);
            }
        fclose(fp);
        }
    return(fp != nullptr);
    }

void HashFile::compareHashFiles(HashFile const &refFile,
    cFilterOutputHashIndices &outHashIndices, DupOptions const &options, File &outFile)
    {
//    fprintf(outFile.getFp(), "Compare %s\n   %s\n", mFilePath.getStr(),
//	    refFile.mFilePath.getStr());
    outHashIndices.init(refFile.mHashItems.size());
    for(size_t i1=0; i1<mHashItems.size(); i1++)
	{
	size_t i2=i1;
        while(i2<refFile.mHashItems.size())
            {
            size_t inc=1;
            bool samePlace = false;
            if(options.mFindDupsInLines)
        	samePlace = ((this == &refFile) && (i1 == i2));
            else
        	samePlace = ((this == &refFile) && mHashItems[i1].mLineNum == mHashItems[i2].mLineNum);
            if(!samePlace)
        	{
		inc = getDupLinesLength(i1, refFile, i2);
		if(inc > options.mNumTokenMatches)
		    {
                    if(!outHashIndices.isAlreadyOutput(i1, i2))
                        {
                        outHashIndices.markAlreadyOutput(i1, inc, i2);
                        size_t totalLines = (mHashItems[i1+inc-1].mLineNum - mHashItems[i1].mLineNum) + 1;
#ifdef __linux__
                        fprintf(outFile.getFp(), "lines %lu  :  %s %lu  :  %s %lu\n",
                            totalLines,
                            getRelativeFileName().getStr(), mHashItems[i1].mLineNum,
                            refFile.getRelativeFileName().getStr(),
                            refFile.mHashItems[i2].mLineNum);
#else
                        fprintf(outFile.getFp(), "lines %u  :  %s %u  :  %s %u\n",
                            totalLines,
                            getRelativeFileName().getStr(), mHashItems[i1].mLineNum,
                            refFile.getRelativeFileName().getStr(),
                            refFile.mHashItems[i2].mLineNum);
#endif
                        }
		    }
		else
		    inc = 1;
        	}

	    i2 += inc;
            }
	}
    }

size_t HashFile::getDupLinesLength(size_t thisStartIndex, HashFile const &refFile,
    size_t refFileStartIndex)
    {
    size_t matchLen = 0;
    while(mHashItems[thisStartIndex].mHash == refFile.mHashItems[refFileStartIndex].mHash)
	{
	if(mHashItems[thisStartIndex].mLineNum == 0 ||
		refFile.mHashItems[refFileStartIndex].mLineNum == 0)
	    {
	    break;
	    }
	if(thisStartIndex < mHashItems.size() &&
		refFileStartIndex < refFile.mHashItems.size())
	    {
	    matchLen++;
	    thisStartIndex++;
	    refFileStartIndex++;
	    }
	else
	    {
	    break;
	    }
	}
    return matchLen;
    }

OovString HashFile::getActualFileName() const
    {
    FilePath fn(mFilePath, FP_File);
    fn.discardExtension();
    return Project::recoverFileName(fn);
    }

OovString HashFile::getRelativeFileName() const
    {
    FilePath dupDir(Project::getProjectDirectory(), FP_Dir);
    dupDir.appendDir(DupsDir);
    OovString fn = Project::getSrcRootDirRelativeSrcFileName(getActualFileName(),
	    dupDir);
    return fn;
    }



class cAllHashFiles
    {
    public:
        cAllHashFiles(std::vector<std::string> const &filePaths);
        /// Compare every file with every other file, including itself
        void compareAllFiles(File &outFile, DupOptions const &options);

    private:
        std::vector<HashFile> mHashFiles;
        // This is here to reduce the number of memory allocations.
        cFilterOutputHashIndices mFilterOutputHashIndices;
    };

cAllHashFiles::cAllHashFiles(std::vector<std::string> const &filePaths):
    mHashFiles(filePaths.size())
    {
    for(size_t i=0; i<filePaths.size(); i++)
        {
        mHashFiles[i].readHashFile(filePaths[i]);
        }
    }

void cAllHashFiles::compareAllFiles(File &outFile, DupOptions const &options)
    {
    for(size_t i1=0; i1<mHashFiles.size(); i1++)
        {
        for(size_t i2=i1; i2<mHashFiles.size(); i2++)
            {
            mHashFiles[i1].compareHashFiles(mHashFiles[i2],
                mFilterOutputHashIndices, options, outFile);
            }
        }
    }


eDupReturn createDuplicatesFile(/*OovStringRef const projDir,*/ std::string &outFn)
    {
    eDupReturn ret = DR_NoDupFilesFound;

    FilePath path(Project::getProjectDirectory(), FP_Dir);
    path.appendDir(DupsDir);
    FilePath ext("hsh", FP_Ext);
    std::vector<std::string> filePaths;
    if(getDirListMatchExt(path, ext, filePaths))
	{
	FilePath outPath(path, FP_Dir);
	outPath.appendFile("Dups.txt");
	File outFile(outPath, "w");
	if(outFile.isOpen())
	    {
	    outFn = outPath;
	    cAllHashFiles hashFiles(filePaths);
	    DupOptions options;
	    hashFiles.compareAllFiles(outFile, options);
	    ret = DR_Success;
	    }
	else
	    ret = DR_UnableToCreateDirectory;
	}
    return ret;
    }

