/*
 * CppParser.h
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CPPPARSER_H_
#define CPPPARSER_H_

#include "ModelObjects.h"
#include "ParseBase.h"
#include <map>
#include <set>
#include "NameValueFile.h"
#include "FilePath.h"
#include "OovString.h"

/// This works to build include paths, and to build include file dependencies
/// This makes a file that keeps a map of paths, and for each path:
///	the last time the path dependencies changed,
///	the last time the paths were checked,
///	the included filepath (such as "gtk/gtk.h"), and the directory to get to that file.
/// This file does not store paths that the compiler searches by default.
class IncDirDependencyMap:public NameValueFile
    {
    public:
	void read(char const * const outDir, char const * const incPath);
	void write();
	void insert(const std::string &includerPath, const FilePath &includedPath);

    private:
	/// This map keeps all included paths for every includer that was parsed.
	/// This map is <includerPath, includedPath's>
	std::map<std::string, std::set<std::string>> mParsedIncludeDependencies;
    };

// This contains context while parsing a switch statement. Case statements are
// kind of interesting to graph in a sequence diagram since cases without breaks
// are actually ored together.
//
// The following code:
//	switch(a)
//		{
//		case '5':
//			func1();
//		case '6':
//			func2();
//			break;
//		}
//
// Gets represented as:
//	[a == 5]
//		-> func1
//	[a == 5 || a == 6]
//		-> func2
class SwitchContext
    {
    public:
	SwitchContext():
	    mInCase(false), mOpenStatementIndex(5000)
	    {}
	SwitchContext(std::string exprString):
	    mSwitchExprString(exprString), mInCase(false), mOpenStatementIndex(5000)
	    {}
	void maybeStartCase(ModelStatements *statements, OovString &opStr)
	    {
	    if(!mInCase)
		{
		statements->addStatement(ModelStatement(opStr, ST_OpenNest));
		mOpenStatementIndex = statements->size()-1;
		mInCase = true;
		}
	    else
		{
		if(mOpenStatementIndex < statements->size())
		    {
		    std::string expr = (*statements)[mOpenStatementIndex].getName();
		    expr.erase(expr.size()-1);
		    expr += " || ";
		    expr += opStr.substr(1);
		    statements->addStatement(ModelStatement("", ST_CloseNest));
//		    (*statements)[mOpenStatementIndex].setName(expr);
		    statements->addStatement(ModelStatement(expr, ST_OpenNest));
		    mOpenStatementIndex = statements->size()-1;
		    }
		}
	    }
	// This is called for a break, or the end of a switch without a break.
	void endCase(ModelStatements *statements)
	    {
	    if(mInCase)
		{
		statements->addStatement(ModelStatement("", ST_CloseNest));
		mInCase = false;
		}
	    }
	std::string mSwitchExprString;

    private:
	bool mInCase;
	size_t mOpenStatementIndex;
    };

// This holds the context during a switch statement. Switch statements can
// be nested, so this is a stack of switch statements. The containing
// switch statement is always the last added to the stack.
class SwitchContexts:public std::vector<SwitchContext>
    {
    public:
	SwitchContext &getCurrentContext()
	    {
	    return((size() > 0) ? (*this)[size()-1] : mDummyContext);
	    }
    private:
	SwitchContext mDummyContext;
    };

class cDupHashFile
    {
    public:
	cDupHashFile():
	    mAlreadyAddedBreak(true)
	    {}
        void open(OovStringRef const fn)
            { mFile.open(fn, "w"); }
        bool isOpen() const
            { return mFile.isOpen(); }
        void append(OovStringRef const text, unsigned int line);
        void appendBreak()
	    {
            if(!mAlreadyAddedBreak)
        	{
		fprintf(mFile.getFp(), "\n");
		mAlreadyAddedBreak = true;
        	}
	    }

    private:
        File mFile;
        bool mAlreadyAddedBreak;
    };

/// This parses a C++ source file, then saves important data into a file.
class CppParser
    {
    public:
	CppParser():
	    mClassifier(nullptr), mOperation(nullptr), mStatements(nullptr),
	    mStatementRecurseLevel(0)
	    {}
	enum eErrorTypes { ET_None, ET_CompileWarnings, ET_CompileErrors,
	    ET_CLangError, ET_ParseError };
        /// Parses a C++ source file.
	eErrorTypes parse(bool lineHashes, char const * const srcFn, char const * const srcRootDir,
		char const * const outDir,
		char const * const clang_args[], int num_clang_args);
	/// These are public for access by global functions callbacks.
	CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitTranslationUnitForIncludes(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitRecord(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitFunctionAddArgs(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitFunctionAddStatements(CXCursor cursor, CXCursor parent);
	CXChildVisitResult visitFunctionAddDupHashes(CXCursor cursor, CXCursor parent);

    private:
        /// This contains all parsed information.
	ModelData mModelData;
        cDupHashFile mDupHashFile;
	ModelClassifier *mClassifier;    /// Current class being parsed.
	ModelOperation *mOperation;      /// Current operation being parsed.
	ModelStatements *mStatements; 	 /// Current statements of a function.
	SwitchContexts mSwitchContexts;
	FilePath mTopParseFn;   /// The top level file that is being parsed.
	Visibility mClassMemberAccess;
	IncDirDependencyMap mIncDirDeps;
	int mStatementRecurseLevel;
	ModelType *createOrGetBaseTypeRef(CXCursor cursor, RefType &rt);
	ModelType *createOrGetDataTypeRef(CXType type, RefType &rt);
	ModelClassifier *createOrGetClassRef(OovStringRef const name);
	ModelType *createOrGetDataTypeRef(CXCursor cursor);
	void addOperationParts(CXCursor cursor, bool addParams);
	void addRecord(CXCursor cursor, Visibility vis);
	OovString buildCondExpr(CXCursor condStmtCursor, int condExprIndex);
	// Adds a conditional statement and its body. This gets added as an
	// open nest and close nest statements. The name of the open nest
	// statement is the conditional expression. This recurses to find
	// all child conditionals and calls.
	void addCondStatement(CXCursor stmtCursor, int condExprIndex,
		int condStatementIndex);
	// Adds an else statement and its body. This gets added as an
	// open nest and close nest statements. The name of the open nest
	// statement is "[else]". This recurses to find all child
	// conditionals and calls.
	void addElseStatement(CXCursor condStmtCursor, int elseBodyIndex);
    };


#endif /* CPPPARSER_H_ */
