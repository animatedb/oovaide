/*
 * CppParser.h
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CPPINSTR_H_
#define CPPINSTR_H_

#include <map>
#include <set>
#include "FilePath.h"
#include "OovString.h"
#include "clang-c/Index.h"

class CXStringDisposer:public std::string
    {
    public:
	CXStringDisposer(const CXString &xstr):
	    std::string(clang_getCString(xstr))
	    {
	    clang_disposeString(xstr);
	    }
    };

class SourceLocation
    {
    public:
	SourceLocation(CXSourceLocation loc):
	    mLoc(loc), mFile(nullptr), mLine(0), mColumn(0), mOffset(0)
	    {
	    clang_getExpansionLocation(mLoc, &mFile, &mLine, &mColumn, &mOffset);
	    }
	SourceLocation(CXCursor cursor):
	    SourceLocation(clang_getCursorLocation(cursor))
	    {
	    }
	std::string getFn() const
	    {
	    CXStringDisposer fn = clang_getFileName(mFile);
	    return fn;
	    }
	int getOffset() const
	    { return mOffset; }
	int getLine() const
	    { return mLine; }
	CXSourceLocation getLoc() const
	    { return mLoc; }

    private:
	CXSourceLocation mLoc;
	CXFile mFile;
	unsigned mLine;
	unsigned mColumn;
	unsigned mOffset;
    };

class SourceRange
    {
    public:
	SourceRange(CXCursor cursor):
	    mRange(clang_getCursorExtent(cursor))
	    {
	    }
	SourceLocation getEndLocation()
	    {
	    return clang_getRangeEnd(mRange);
	    }
	SourceLocation getStartLocation()
	    {
	    return clang_getRangeStart(mRange);
	    }

    private:
	CXSourceRange mRange;
    };

class CppFileContents
    {
    public:
	bool read(char const *fn);
	bool write(char const *fn);
	// The origFileOffset is the offset into the original file.
	void insert(char const *str, int origFileOffset);

    private:
	std::multimap<int, std::string> mInsertMap;
	std::vector<char> mFileContents;

	/// Reads the mInsertMap and writes the mFileContents.
	void updateMemory();
    };

/// This parses a C++ source file, then instruments with coverage code.
class CppInstr
    {
    public:
	CppInstr():
	    mInstrCount(0)
	    {}
	enum eErrorTypes { ET_None, ET_CompileWarnings, ET_CompileErrors,
	    ET_CLangError, ET_ParseError };
        /// Parses a C++ source file.
	eErrorTypes parse(char const * const srcFn, char const * const srcRootDir,
		char const * const outDir,
		char const * const clang_args[], int num_clang_args);
	/// These are public for access by global functions callbacks.
	CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitFunctionAddInstr(CXCursor cursor, CXCursor parent);

    private:
	FilePath mTopParseFn;   /// The top level file that is being parsed.
	int mInstrCount;
	CppFileContents mOutputFileContents;

	bool isParseFile(SourceLocation const &loc) const;
	bool isParseFile(CXFile const &file) const;

	void makeCovInstr(OovString &covStr);
	void insertOutputText(OovString &covStr, int offset)
	    { mOutputFileContents.insert(covStr.c_str(), offset); }
	void insertOutputText(char const *covStr, int offset)
	    { mOutputFileContents.insert(covStr, offset); }
	void insertCovInstr(int offset);
	void insertNonCompoundInstr(CXCursor cursor);
//	void instrChildNonCompoundStatements(CXCursor cursor);

	static void updateCoverageHeader(std::string const &fn, char const * const covDir,
		int numInstrLines);
	static void updateCoverageSource(std::string const &fn, char const * const covDir);
    };


#endif /* CPPINSTR_H_ */
