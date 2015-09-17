/*
 * CppParser.h
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CPPPARSER_H_
#define CPPPARSER_H_

#include "IncDirMap.h"
#include "ParserModelData.h"
#include <set>
#include "OovString.h"

#define DEBUG_PARSE 0

// This contains context while parsing a switch statement. Case statements are
// kind of interesting to graph in a sequence diagram since cases without breaks
// are actually ored together.
//
//      switch(a)
//              {
//              case 5:
//                      func1();
//              case 6:
//                      func2();
//                      break;
//              }
//
// This gets represented as:
//      [a == 5]
//              -> func1
//      [a == 5 || a == 6]
//              -> func2
//
// Another interesting problem is a return or break under a conditional.
// This does not get handled correctly at this time since the nested statement
// always ends the case, but it should actually depend on the conditions.
//      switch(a)
//              {
//              case 4:
//                      return;
//              case 5:
//                      if(b == 3)
//                              func1();
//                              return;         // Or break
//                      func2();
//              case 6:
//                      func3();
//                      break;
//              }
//
// Something like the following should not create separate conditionals, but
// should only output a single [a==4 || a==5]:
//      switch(a)
//              {
//              case 4:
//              case 5:
//                      func1();
//                      break;
//              }
class SwitchContext
    {
    public:
        SwitchContext():
            mInCase(false), mParentFuncStatements(nullptr)
            {}
        SwitchContext(std::string exprString):
            mSwitchExprString(exprString), mInCase(false),
            mParentFuncStatements(nullptr)
            {}
        // This will only output the case statement if there is some functionality
        // after the case.
        void startCase(ModelStatements *parentFuncStatements, CXCursor cursor,
                OovString &opStr);
        // This is called for a break, return, or the end of a switch without a break.
        void endCase();
        std::string mSwitchExprString;

    private:
        bool mInCase;
        ModelStatements *mParentFuncStatements;
        OovString mConditionalStr;
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

class DupHashFile
    {
    public:
        DupHashFile():
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
            mClassifier(nullptr), mOperation(nullptr), mStatements(nullptr)
#if(DEBUG_PARSE)
            ,
            mStatementRecurseLevel(0)
#endif
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
        ParserModelData mParserModelData;
        DupHashFile mDupHashFile;
        ModelClassifier *mClassifier;    /// Current class being parsed.
        ModelOperation *mOperation;      /// Current operation being parsed.
        ModelStatements *mStatements;    /// Current statements of a function.
        SwitchContexts mSwitchContexts;
        FilePath mTopParseFn;   /// The top level file that is being parsed.
        Visibility mClassMemberAccess;
        IncDirDependencyMap mIncDirDeps;
#if(DEBUG_PARSE)
        int mStatementRecurseLevel;
#endif
        void addOperationParts(CXCursor cursor, bool addParams);
        void addRecord(CXCursor cursor, Visibility vis);
        void addVar(CXCursor cursor);
        void addClassFileLoc(CXCursor cursor, ModelClassifier *classifier);
        /// This only adds typedefs of class (or template?) types.
        void addTypedef(CXCursor cursor);
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
        void handleCallExprCursor(CXCursor cursor, CXCursor parent);
    };


#endif /* CPPPARSER_H_ */
