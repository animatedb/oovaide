/*
 * CppParser.cpp
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
*/

#include "CppParser.h"
#include "Project.h"
#include "Debug.h"
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
// Prevent "error: 'off_t' has not been declared"
#define off_t _off_t
#include <unistd.h>             // for unlink
#include <limits.h>
#include <algorithm>


// This must save a superset of what gets written to the file. For exmample,
// when parsing a class in a header file, the access specifiers must be
// saved for the operations in the class, so that when an operation is
// defined in a source file, the operation can be defined with the
// correct access specifier.
//
// setModule is called for every class defined in the current TU.
// setModule is called for every operation defined in the current TU.

#define DEBUG_HASH 0
#if(DEBUG_PARSE)
static DebugFile sLog("DebugCppParse.txt", false);
#endif


static void dumpCursor(FILE *fp, char const * const str, CXCursor cursor)
    {
    if(fp)
        {
        CXStringDisposer name(clang_getCursorSpelling(cursor));
        CXStringDisposer spstr = clang_getCursorKindSpelling(clang_getCursorKind(cursor));
        std::string tokenStr;
        appendCursorTokenString(cursor, tokenStr);
        if(tokenStr.length() > 100)
            {
            tokenStr.resize(100);
            tokenStr += "...";
            }

        fprintf(fp, "%s: %s %s   %s\n", str, spstr.c_str(), name.c_str(), tokenStr.c_str());
        fflush(fp);
        }
    }

void SwitchContext::startCase(ModelStatements *parentFuncStatements,
        CXCursor cursor, OovString &opStr)
    {
    mParentFuncStatements = parentFuncStatements;

    // This will only output the case statement if there is some functionality
    // after the case.
    bool hasFunctionality = false;
    CXCursor c1 = getNthChildCursor(cursor, 1);
    if(c1.kind != CXCursor_CaseStmt && c1.kind != CXCursor_DefaultStmt)
        {
        hasFunctionality = true;
        }
    if(!mInCase)
        {
        mConditionalStr = opStr;
        if(hasFunctionality)
            {
            mParentFuncStatements->addStatement(ModelStatement(mConditionalStr, ST_OpenNest));
            }
        }
    else
        {
        std::string expr = mConditionalStr;
        removeLastNonIdentChar(expr);   // Remove the ']'
        removeLastNonIdentChar(expr);   // Remove the ' '
        expr += " || ";
        expr += opStr.substr(1);
        mConditionalStr = expr;
        if(hasFunctionality)
            {
            mParentFuncStatements->addStatement(ModelStatement("", ST_CloseNest));
            mParentFuncStatements->addStatement(ModelStatement(mConditionalStr, ST_OpenNest));
            }
        }
    mInCase = true;
    }

void SwitchContext::endCase()
    {
    if(mInCase)
        {
        mParentFuncStatements->addStatement(ModelStatement("", ST_CloseNest));
        mInCase = false;
        }
    }


static std::string getFileLoc(CXCursor cursor, unsigned int *retLine = nullptr)
    {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    CXFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    std::string fn;
    clang_getExpansionLocation(loc, &file, &line, &column, &offset);
    if(file != nullptr)
        {
        CXStringDisposer tempFn = clang_getFileName(file);
        fn = tempFn;
        }
    if(retLine)
        *retLine = line;
    return fn;
    }

class CrashDiagnostics
    {
    public:
        CrashDiagnostics():
            mCrashed(false), mEnableDumpCursor(true)
            {}
        void saveMostRecentParseLocation(char const * const diagStr, CXCursor cursor)
            {
            mDiagStr = diagStr;
            mMostRecentCursor = cursor;
#if(DEBUG_PARSE)
            dumpCursor(sLog.mFp, diagStr, cursor);
#endif
            }
        void enableDumpCursor(bool enable)
            { mEnableDumpCursor = enable; }
        void setCrashed()
            { mCrashed = true; }
        bool hasCrashed() const
            { return mCrashed; }
        void dumpCrashed(FILE *fp)
            {
            if(mCrashed)
                {
                fprintf(fp, "CRASHED: %s\n", mDiagStr.c_str());
                try
                    {
                    dumpCursor(fp, "", mMostRecentCursor);
                    }
                catch(...)
                    {
                    }
                }
            }
    private:
        bool mCrashed;
        bool mEnableDumpCursor;
        std::string mDiagStr;
        CXCursor mMostRecentCursor;
    };
static CrashDiagnostics sCrashDiagnostics;


static CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    CppParser *parser = static_cast<CppParser*>(client_data);
    return parser->visitTranslationUnit(cursor, parent);
    }

static CXChildVisitResult visitTranslationUnitForIncludes(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    CppParser *parser = static_cast<CppParser*>(client_data);
    return parser->visitTranslationUnitForIncludes(cursor, parent);
    }

static CXChildVisitResult visitRecord(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    CppParser *parser = static_cast<CppParser*>(client_data);
    return parser->visitRecord(cursor, parent);
    }

static CXChildVisitResult visitFunctionAddArgs(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    CppParser *parser = static_cast<CppParser*>(client_data);
    return parser->visitFunctionAddArgs(cursor, parent);
    }

static CXChildVisitResult visitFunctionAddStatements(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    CppParser *parser = static_cast<CppParser*>(client_data);
    return parser->visitFunctionAddStatements(cursor, parent);
    }

static CXChildVisitResult visitFunctionAddDupHashes(CXCursor cursor, CXCursor parent,
        CXClientData client_data)
    {
    CppParser *parser = static_cast<CppParser*>(client_data);
    return parser->visitFunctionAddDupHashes(cursor, parent);
    }


/////////////////////////


void CppParser::addOperationParts(CXCursor cursor, bool addParams)
    {
    // When a function outside of the class is defined, there is no need to add parameters again.
    if(addParams)
        {
        clang_visitChildren(cursor, ::visitFunctionAddArgs, this);
        }
    CXType retType = clang_getResultType(clang_getCursorType(cursor));
    RefType rt;
    mOperation->getReturnType().setDeclType(mParserModelData.createOrGetDataTypeRef(retType, rt));
    mOperation->getReturnType().setConst(rt.isConst);
    mOperation->getReturnType().setRefer(rt.isRef);
    mStatements = &mOperation->getStatements();
    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
    if(mDupHashFile.isOpen())
        {
        FilePath fn(getFileLoc(cursor), FP_File);
        if(fn == mTopParseFn)
            {
            // Originally, this code used to pass all children of the method cursor,
            // but this caused problems because for some reason the breaks were not
            // separating all method signatures.  Most people probably think duplicate
            // code detection should discard the parameters of the methods, so this
            // is what is done now.
            mDupHashFile.appendBreak();
            CXCursor bodyCursor = getCursorChildKind(cursor, CXCursor_CompoundStmt, true);
            if(!clang_Cursor_isNull(bodyCursor))
                {
                clang_visitChildren(bodyCursor, ::visitFunctionAddDupHashes, this);
                }
            }
        }
    }

static bool isField(CXCursor cursor)
    {
    CXCursorKind kind = clang_getCursorKind(cursor);
    bool output = true;
    switch(kind)
        {
        case CXCursor_Constructor:
        case CXCursor_Destructor:
        case CXCursor_VarDecl:
        case CXCursor_FieldDecl:
            break;

        default:
            output=false;
            break;
        }
    return output;
    }

// Takes a CXType_FunctionProto cursor, and finds function arguments.
CXChildVisitResult CppParser::visitFunctionAddArgs(CXCursor cursor, CXCursor /*parent*/)
    {
    sCrashDiagnostics.saveMostRecentParseLocation("FA", cursor);
    CXCursorKind ckind = clang_getCursorKind(cursor);
    if(ckind == CXCursor_ParmDecl)
        {
        CXStringDisposer name = clang_getCursorDisplayName(cursor);
        RefType rt;
        ModelType *type = mParserModelData.createOrGetBaseTypeRef(cursor, rt);
        if(type)
            {
            ModelFuncParam *param = mOperation->addMethodParameter(name, type, false);
            param->setConst(rt.isConst);
            param->setRefer(rt.isRef);
            }
        }
    return CXChildVisit_Continue;
    }

static Visibility getAccess(CXCursor cursor)
    {
    Visibility access;
    CX_CXXAccessSpecifier acc = clang_getCXXAccessSpecifier(cursor);
    switch(acc)
        {
        default:
        case CX_CXXInvalidAccessSpecifier:
        case CX_CXXPrivate:
            access.setVis(Visibility::Private);
            break;

        case CX_CXXProtected:
            access.setVis(Visibility::Protected);
            break;

        case CX_CXXPublic:
            access.setVis(Visibility::Public);
            break;
        }
    return access;
    }

// @todo - newer versions of clang may have this.
/*
static bool isPureVirtual(CXCursor cursor)
    {
    // clang_CXXMethod_isPureVirtual
    bool purevirt = clang_CXXMethod_isVirtual(cursor);
    if(purevirt)
        {
        std::string ts;
        buildTokenStringForCursor(cursor, ts);
        if(ts.find("=0") == std::string::npos)
            purevirt = false;
        }
    return purevirt;
    }
*/

// This is to find part of conditional statements in a for loop.
// The first part of a for loop is a decl stmt, so skip this and get next operator or expression.
// This must catch things like:
// for(; x<y; x++)
// if(x & y)
// for(i=(i==0); i--; )
static CXChildVisitResult getConditionStr(CXCursor cursor, CXCursor /*parent*/,
        CXClientData client_data)
    {
    std::string *op = static_cast<std::string*>(client_data);
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    // This may not be everything. It is also difficult to parse the statements
    // following the expression since it may be: CompoundStmt, NullStmt, DeclStmt, etc.
    if(cursKind == CXCursor_BinaryOperator ||
            cursKind == CXCursor_UnaryOperator ||
            cursKind == CXCursor_FirstExpr ||
            cursKind == CXCursor_CallExpr)
        {
        appendConditionString(cursor, *op);
//      removeLastNonIdentChar(*op);
        return CXChildVisit_Break;
        }
    return CXChildVisit_Continue;
    }

OovString CppParser::buildCondExpr(CXCursor condStmtCursor, int condExprIndex)
    {
    OovString fullop;

    CXCursorKind condStmtCursKind = clang_getCursorKind(condStmtCursor);
    switch(condStmtCursKind)
        {
        case CXCursor_DoStmt:
        case CXCursor_WhileStmt:
        case CXCursor_ForStmt:
        case CXCursor_CXXForRangeStmt:
            fullop += "*";
            break;

        default:
            break;
        }
    fullop += "[";
    if(condStmtCursKind == CXCursor_CaseStmt)
        {
        std::string tempStr;
        fullop += mSwitchContexts.getCurrentContext().mSwitchExprString;
        fullop += " == ";
        }
    appendConditionString(getNthChildCursor(condStmtCursor, condExprIndex), fullop);
    if(condStmtCursKind == CXCursor_CaseStmt)
        {
        removeLastNonIdentChar(fullop); // Remove the ':' from the case.
        }
    fullop += ']';
    return fullop;
    }

void CppParser::addCondStatement(CXCursor condStmtCursor, int condExprIndex,
        int mainBodyIndex)
    {
    // Visit the expression since it may contain calls.
    CXCursor condExprCursor = getNthChildCursor(condStmtCursor, condExprIndex);
#if(DEBUG_PARSE)
    dumpCursor(sLog.mFp, "cond expr visited", condExprCursor);
#endif
    OovString fullop = buildCondExpr(condStmtCursor, condExprIndex);
    mStatements->addStatement(ModelStatement(fullop, ST_OpenNest));
    // Add the expression statements after the conditional since it is a bit
    // easier to find the beginning of the conditional. These should not
    // really be shown as indented since they may or may not be run
    // depending on the expression logic.
    visitFunctionAddStatements(condExprCursor, condStmtCursor);
    CXCursor bodyCursor = getNthChildCursor(condStmtCursor, mainBodyIndex);
// The body cursor can be things such as call expressions, unary operators, etc.
/*
    if(!clang_isStatement(bodyCursor.kind))
        {
        OovString str;
        str += " ";
        CXStringDisposer spstr = clang_getCursorKindSpelling(clang_getCursorKind(condStmtCursor));
        str += spstr;
        str += " ";
        spstr = clang_getCursorKindSpelling(clang_getCursorKind(bodyCursor));
        str += spstr;
        LogAssertFile(__FILE__, __LINE__, str.getStr());
//        LogAssertFile(__FILE__, __LINE__,
//            mParserModelData.getParsedModule()->getName().getStr());
        }
*/
#if(DEBUG_PARSE)
    dumpCursor(sLog.mFp, "body visited", bodyCursor);
#endif
    visitFunctionAddStatements(bodyCursor, condStmtCursor);
    mStatements->addStatement(ModelStatement("", ST_CloseNest));
    }

void CppParser::addElseStatement(CXCursor condStmtCursor, int elseBodyIndex)
    {
    CXCursor elseCursor = getNthChildCursor(condStmtCursor, elseBodyIndex);
    if(!clang_Cursor_isNull(elseCursor))
        {
        CXCursorKind elseCursKind = clang_getCursorKind(elseCursor);
        if(elseCursKind == CXCursor_IfStmt)
            {
            visitFunctionAddStatements(elseCursor, condStmtCursor);
            }
        else
            {
#if(DEBUG_PARSE)
            dumpCursor(sLog.mFp, "else visited", elseCursor);
#endif
            std::string elseop = "[else]";
            mStatements->addStatement(ModelStatement(elseop, ST_OpenNest));
            visitFunctionAddStatements(elseCursor, condStmtCursor);
            mStatements->addStatement(ModelStatement("", ST_CloseNest));
            }
#if(DEBUG_PARSE)
        fprintf(sLog.mFp, "end else visited\n");
#endif
        }
    }

static unsigned int makeHash(OovStringRef const text)
    {
    // djb2 hash function
    unsigned int hash = 5381;
    char const *str = text;

    while(*str)
        {
        int c = *str++;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }
    return hash;
    }

void DupHashFile::open(OovStringRef const fn)
    {
    mFile.open(fn, "w");
    if(!mFile.isOpen())
        {
        fprintf(stderr, "Unable to open hash file");
        }
    }

void DupHashFile::append(OovStringRef const text, unsigned int line)
    {
    if(mFile.isOpen() && text.numBytes() > 0)
        {
        OovString str;
        str.appendInt(makeHash(text), 16);
        str += ' ';
        str.appendInt(line);
#if(DEBUG_HASH)
        str += ' ';
        str += text.getStr());
#endif
        str += '\n';
        OovStatus status = mFile.putString(str);
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to append to hash file");
            }
        mAlreadyAddedBreak = false;
        }
    }

CXChildVisitResult CppParser::visitFunctionAddDupHashes(CXCursor cursor,
    CXCursor /*parent*/)
    {
    CXChildVisitResult res = CXChildVisit_Recurse;
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    // Many statements have children. If it is a statement that has children,
    // break it into separate parts.  For example, the if condition from the main clause
    // or else clause.
    //
    // This discards some information such as nesting, or the type of statement.
    // For example, a while statement is indistinguishable from an if
    // statement, but for duplicate code detection, it probably works well enough.
    // If not, some identifier string could be prepended on to the child, such as
    // "if#", "{#", or "else#".
    if(!clang_isStatement(cursKind))
        {
        std::string text;
        appendCursorTokenString(cursor, text);
        CXSourceLocation loc = clang_getCursorLocation(cursor);
        unsigned int line;
        clang_getExpansionLocation(loc, nullptr, &line, nullptr, nullptr);
        mDupHashFile.append(text, line);

        res = CXChildVisit_Continue;
        }
    return res;
    }

static bool isBaseClass(ParserModelData const &modelData,
        ModelClassifier *classifier, CXCursor memberRefCursor)
    {
    CXCursor attrCursor = clang_getCursorReferenced(memberRefCursor);
    CXCursor attrClassCursor = clang_getCursorSemanticParent(attrCursor);
    CXStringDisposer classTypeName = clang_getCursorDisplayName(attrClassCursor);
    return modelData.isBaseClass(classifier, classTypeName);
    }

void CppParser::handleCallExprCursor(CXCursor cursor, CXCursor parent)
    {
    // For portion diagrams, it is important to display any
    // references to the class. This includes references to
    // base classes.  Some base class references will not have
    // a member expression (instead of "mAttr.call()", it will just
    // be "call()").
    //
    // Global functions calls look similar to base member calls in
    // source code, but they do not have a grandchild of a
    // CXCursor_MemberRefExpr.
    //
    // Conversion constructors also look similar, but they have
    // a parent of CXCursor_VarDecl.
    //
    // Method parameters and globals references do have a
    // CXCursor_MemberRefExpr grandchild cursor.

    // A constructor call to a parent class will be a CallExpr, but
    // does not have any info to indicate it is a this reference
    // from the libclang CIndex interface.

    // This is a bit of a kludge since this is an undocumented way
    // to check if a base class reference is being used.
    //
    //                              first child             first grandchild
    // Call to base class:
    //
    // Call to global function:     CXCursor_UnexposedExpr  CXCursor_DeclRefExpr
    // Call to implicit:            first arg cursor?
    // Call through parameter:      CXCursor_MemberRefExpr  UnexposedExpr
    // Call through member:         CXCursor_MemberRefExpr  CXCursor_MemberRefExpr<bound member...>
    // call through global:         CXCursor_MemberRefExpr  CXCursor_DeclRefExpr
    //
    // A call to the parent class does not have a child cursor of the memberRefExpr.
    CXStringDisposer functionName = clang_getCursorDisplayName(cursor);
    if(functionName.length() != 0)
        {
        // Unknown could be implicit this or member reference.
        // Implicit this can refer to this member or parent class.
        enum CallTypes { CT_Unknown, CT_Constructor, CT_Global };
        CallTypes ct = CT_Unknown;
        // Global functions, constructors, and "implicit this", do not have a
        // child member ref expr. The "implicit this" calls can refer to
        // a member, or parent class member.
        CXCursor child = getNthChildCursor(cursor, 0);

        // Check if the child is either a constructor or global
        if(child.kind != CXCursor_MemberRefExpr)
            {
            CXType cursType = clang_getCursorType(cursor);
            CXCursor typeDecl = clang_getTypeDeclaration(cursType);
            CXCursor method = clang_getNullCursor();
            // A typedecl only exists for non-globals.
            if(typeDecl.kind != CXCursor_NoDeclFound)
                {
                // Look for a the method part of the call. This could be a constructor.
                method = getCursorChildKind(typeDecl, CXCursor_CXXMethod, true);
                if(!clang_Cursor_isNull(method))
                    {
                    // Constructors don't have member ref exprs.
                    CXCursor memberRefCursor = getCursorChildKind(method, CXCursor_MemberRefExpr, false);
                    if(clang_Cursor_isNull(memberRefCursor))
                        {
                        ct = CT_Constructor;
                        }
                    }
                }
            else
                {
                ct = CT_Global;
                }
            }
// Calls through templates are confusing.
// Some of this code could be used to build the typeref?  template<class>->call()
#define CALL_TYPEREF 0
#if(CALL_TYPEREF)
            // Constructor called through templates come here????
            /// typedef std::pair<xx, yy> zz
                else if(child.kind == CXCursor_TypeRef)
                    {
        /*
        CXStringDisposer sp = clang_getCursorSpelling(child);
        if(sp == "ZoneConnection")
            {
            printf("a");
            }
        */
        addedStatement = true;
        RefType rt;
        const ModelType *classType = mParserModelData.createOrGetBaseTypeRef(child, rt);
        ModelStatement stmt(functionName, ST_Call);
        stmt.getClassDecl().setDeclType(classType);
        mStatements->addStatement(stmt);
        }
#endif

        switch(ct)
            {
            case CT_Constructor:
                {
                if(isBaseClass(mParserModelData, mClassifier, cursor))
                    {
                    // The type of the class that the call is a member of is stored
                    // below using clang_getCursorSemanticParent.
                    // The calls to parent classes are distinguished using a symbol, so that
                    // all references to the class data is displayed in portion diagrams.
                    functionName.insert(0, ModelStatement::getBaseClassMemberCallSep());
                    }
                }
                break;

            case CT_Global:
                break;

            case CT_Unknown:
                {
                OovString attrName;
                CXCursor memberRefCursor = getCursorChildKind(cursor, CXCursor_MemberRefExpr, false);
                appendCursorTokenString(memberRefCursor, attrName);
                size_t pos = getFunctionNameFromMemberRefExpr(attrName);
                if(pos != std::string::npos)
                    {
                    // @todo - It is possible that this should also insert the base class member
                    // call sep if it is in the base class.
                    attrName.erase(pos);
                    attrName.replaceStrs(" ", "");
                    // This could look like:        getType().mAttr.funcname
                    functionName.insert(0, attrName);
                    }
                else
                    {
                    if(isBaseClass(mParserModelData, mClassifier, cursor))
                        {
                        functionName.insert(0, ModelStatement::getBaseClassMemberCallSep());
                        }
                    }
                }
                break;
            }

        CXCursor defCursor = clang_getCursorDefinition(cursor);
        if(defCursor.kind == CXCursor_FirstInvalid)
            defCursor = clang_getCursorReferenced(cursor);
        CXCursor classCursor = clang_getCursorSemanticParent(defCursor);
        RefType rt;
        const ModelType *classType = mParserModelData.createOrGetBaseTypeRef(classCursor, rt);
        CXStringDisposer sym = clang_getCursorUSR(defCursor);
        functionName += ModelStatement::getOverloadKeySep();
        functionName += ModelStatement::makeOverloadKeyFromOperUSR(sym);
        ModelStatement stmt(functionName, ST_Call);
        stmt.getClassDecl().setDeclType(classType);
        mStatements->addStatement(stmt);
        }
    }


// Takes a CXType_FunctionProto cursor, and finds conditional statements and
// calls to functions.
CXChildVisitResult CppParser::visitFunctionAddStatements(CXCursor cursor,
    CXCursor parent)
    {
#if(DEBUG_PARSE)
    mStatementRecurseLevel++;
    for(int i=0; i<mStatementRecurseLevel; i++)
        fprintf(sLog.mFp, "  ");
#endif
    static bool modifiedLhs = false;
    sCrashDiagnostics.saveMostRecentParseLocation("FS", cursor);
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    switch(cursKind)
        {
        case CXCursor_SwitchStmt:
            {
            std::string tempStr;
            clang_visitChildren(cursor, getConditionStr, &tempStr);
            mSwitchContexts.push_back(tempStr);
            clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
            SwitchContext &context = mSwitchContexts.getCurrentContext();
            context.endCase();
            mSwitchContexts.pop_back();
            }
            break;

        case CXCursor_DoStmt:
            // addCondstatement visits children using clang_visitChildren.
            addCondStatement(cursor, 1, 0);
            break;

//      case CXCursor_GotoStmt, break, continue, return, CXCursor_DefaultStmt
        /// Conditional statements
        case CXCursor_WhileStmt:        //   enum { VAR, COND, BODY, END_EXPR };
            // addCondstatement visits children using clang_visitChildren.
            addCondStatement(cursor, 0, 1);     // cond body
            break;

        case CXCursor_CaseStmt:
//          addCondStatement(cursor, 0, 1);     // cond body
            {
            OovString fullop = buildCondExpr(cursor, 0);
            SwitchContext &context = mSwitchContexts.getCurrentContext();
            context.startCase(mStatements, cursor, fullop);
            clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
            }
            break;

        case CXCursor_DefaultStmt:
            {
            SwitchContext &context = mSwitchContexts.getCurrentContext();
            OovString defStr("[default]");
            context.startCase(mStatements, cursor, defStr);
            clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
            }
            break;

        // This isn't quite right. If the break or return is under a
        // conditional, then the case hasn't ended.
        case CXCursor_ReturnStmt:
        case CXCursor_BreakStmt:
            {
            SwitchContext &context = mSwitchContexts.getCurrentContext();
            context.endCase();
            clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
            }
            break;

        case CXCursor_CXXForRangeStmt:
            // addCondstatement visits children using clang_visitChildren.
            addCondStatement(cursor, 1, 2);
            break;

        case CXCursor_ForStmt:
            // addCondstatement visits children using clang_visitChildren.
            addCondStatement(cursor, 1, 3);
            break;

        case CXCursor_IfStmt:           //    enum { VAR, COND, THEN, ELSE, END_EXPR };
            // addCondstatement visits children using clang_visitChildren.
            addCondStatement(cursor, 0, 1);     // cond=0, ifbody=1, [elsebody]=2
            addElseStatement(cursor, 2);
            break;

        // When a call expression is like member.func(), the first child is a
        // member ref expression.  The visitChildren call will add the member
        // ref expression as a new type ref in the model.
        case CXCursor_CallExpr:
            clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
            handleCallExprCursor(cursor, parent);
            break;

//      case CXCursor_CXXCatchStmt:
//          break;

        case CXCursor_BinaryOperator:
            {
            // If assignment operator, and on lhs, variable is modified.
            CXCursor lhsChild = getNthChildCursor(cursor, 0);
            std::string lhsStr;
            // This returns the operator characters at the end.
            appendCursorTokenString(lhsChild, lhsStr);

//          CXStringDisposer lhsStr = clang_getCursorSpelling(lhsChild);
            if(lhsStr.length() >= 2)
                {
                std::string lastTwoChars = lhsStr.substr(lhsStr.length()-2);
                if(lastTwoChars[0] != '=' && lastTwoChars[0] != '!' &&
                        lastTwoChars[0] != '>' && lastTwoChars[0] != '<' &&
                        lastTwoChars[1] == '=')
                    {
                    // The CXCursor_FirstExpr could be used to detect lhs, but
                    // we still need to see if it is an assignment.
                    modifiedLhs = true;
                    }
                }
            visitFunctionAddStatements(lhsChild, cursor);
            modifiedLhs = false;
            visitFunctionAddStatements(getNthChildCursor(cursor, 1), cursor);
            }
            break;

        case CXCursor_VarDecl:
            {
            CXType cursType = clang_getCursorType(cursor);
            if(cursType.kind == CXType_Record)
                {
                CXStringDisposer name = clang_getCursorDisplayName(cursor);
                RefType rt;
                ModelType *type = mParserModelData.createOrGetBaseTypeRef(cursor, rt);
                if(type && mOperation)
                    {
                    mOperation->addBodyVarDeclarator(name, type,
                            rt.isConst, rt.isRef);
                    }
                }
            clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
            }
            break;

        // An expression that refers to a member of a struct, union, class, etc.
        // If first child of CXCursor_BinaryOperator, it is read only.
        // If child of CXCursor_UnaryOperator, it may be write or read.
        // This saves multiple type references to things like "class1.class2.member".
        // This works because of the recursive call to clang_visitChildren.
        case CXCursor_MemberRefExpr:
        case CXCursor_MemberRef:
            {
            CXStringDisposer name = clang_getCursorDisplayName(cursor);

            // This returns the CXCursor_CXXMethod cursor
            CXCursor cursorDef = clang_getCursorDefinition(cursor);
            if(cursorDef.kind == CXCursor_FieldDecl)
                {
                CXCursor child = getNthChildCursor(cursor, 0);
                bool haveMemberRef = (child.kind == CXCursor_MemberRefExpr);
                bool haveConstructor = (parent.kind == CXCursor_VarDecl ||
                        parent.kind == CXCursor_Constructor);
                if(!haveMemberRef && !haveConstructor)
                    {
                    if(isBaseClass(mParserModelData, mClassifier, cursor))
                        {
                        // The type of the class that the call is a member of is stored
                        // below using clang_getCursorSemanticParent.
                        // The calls to parent classes are distinguished using a symbol, so that
                        // all references to the class data is displayed in portion diagrams.
                        name.insert(0, ModelStatement::getBaseClassMemberRefSep());
                        }
                    }
                CXCursor classCursor = clang_getCursorSemanticParent(cursorDef);
                if(classCursor.kind == CXCursor_ClassDecl || classCursor.kind == CXCursor_StructDecl)
                    {
                    RefType rt;
                    const ModelType *classType = mParserModelData.createOrGetBaseTypeRef(classCursor, rt);
                    ModelStatement stmt(name, ST_VarRef);
                    stmt.getClassDecl().setDeclType(classType);

                    const ModelType *varType = mParserModelData.createOrGetBaseTypeRef(cursor, rt);
                    stmt.getVarDecl().setDeclType(varType);

                    stmt.setVarAccessWrite(modifiedLhs);
                    mStatements->addStatement(stmt);
                    }
                else
                    {
//                  LogAssertFile(__FILE__, __LINE__);
                    }
                }
            else if(cursorDef.kind != CXCursor_CXXMethod && cursorDef.kind != CXCursor_FirstInvalid &&
                    cursorDef.kind != CXCursor_ConversionFunction)
                {
                LogAssertFile(__FILE__, __LINE__,
                        mParserModelData.getParsedModule()->getName().getStr());
                }
            clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
            }
            break;

//      case CXCursor_BinaryOperator:
//              The first child is the rhs. second is lhs.
//              If first child is memberrefexpr, then it is read only
//          break;

//      case CXCursor_CompoundStmt:
        default:
            clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
            break;
        }
#if(DEBUG_PARSE)
    mStatementRecurseLevel--;
#endif
    return CXChildVisit_Continue;
    }

void CppParser::addClassFileLoc(CXCursor cursor, ModelClassifier *classifier)
    {
    unsigned int line;
    std::string fn = getFileLoc(cursor, &line);
    // Indicate to the modelwriter, that this class is in this TU.
    FilePath normFn(fn, FP_File);
    if(normFn == mTopParseFn)
        {
        classifier->setModule(mParserModelData.getParsedModule());
        }
    classifier->setLineNum(line);
    }

void CppParser::addTypedef(CXCursor cursor)
    {
    CXType cursorType = clang_getCursorType(cursor);
    if(cursorType.kind == CXType_Typedef)
        {
        ModelType *typeDef = mParserModelData.createOrGetTypedef(cursor);
        if(typeDef && typeDef->getDataType() == DT_Class)
            {
            addClassFileLoc(cursor, typeDef->getClass());
            // Only add typedefs that are defined in the compiled file.
            if(typeDef->getClass()->getModule())
                {
                CXType typedefCursorType = clang_getTypedefDeclUnderlyingType(cursor);
                CXType baseType = getBaseType(typedefCursorType);
                RefType rType;
                ModelType const *parentReffedType = mParserModelData.createOrGetDataTypeRef(baseType, rType);
                ModelClassifier const *child = typeDef->getClass();
                ModelClassifier const *parent = parentReffedType->getClass();
                mParserModelData.addAssociation(parent, child, getAccess(cursor));
                }
            }
        }
    }

void CppParser::addRecord(CXCursor cursor, Visibility vis)
    {
    // Save all classes so that the access specifiers in the declared classes
    // from other TU's are saved. (Ex, the header for this source file)
    OovString name = getFullBaseTypeName(cursor);
    mClassifier = mParserModelData.createOrGetClassRef(name);
    if(mClassifier)
        {
        addClassFileLoc(cursor, mClassifier);
        clang_visitChildren(cursor, ::visitRecord, this);
        }
    mClassifier = nullptr;
    mClassMemberAccess = vis;
    }

// Takes a CXType_Record cursor and adds parts of a class including
// inheritance, inline functions, and member variables.
CXChildVisitResult CppParser::visitRecord(CXCursor cursor, CXCursor parent)
    {
    sCrashDiagnostics.saveMostRecentParseLocation("R ", cursor);
    CXCursorKind ckind = clang_getCursorKind(cursor);
    switch(ckind)
        {
        case CXCursor_CXXBaseSpecifier:
            {
            FilePath fn(getFileLoc(cursor), FP_File);
            if(fn == mTopParseFn)
                {
                // These have to be made datatypes (not classes) so that these don't define
                // the type.  For example, std::string may not be defined in a file,
                // so these would create a new model number for the type.
                ModelClassifier *child = static_cast<ModelClassifier*>(
                        mParserModelData.createOrGetDataTypeRef(parent));
                // Base classes are never simple types.
                ModelClassifier *classParent = static_cast<ModelClassifier*>(
                        mParserModelData.createOrGetClassRef(getFullBaseTypeName(cursor)));
                mParserModelData.addAssociation(classParent, child, getAccess(cursor));
                }
            }
            break;

        case CXCursor_CXXAccessSpecifier:
            {
            mClassMemberAccess = getAccess(cursor);
            }
            break;

        case CXCursor_Constructor:
        case CXCursor_CXXMethod:
        case CXCursor_Destructor:
            {
            // Add all operations in all TU's so that the correct access is defined.
            CXStringDisposer str(clang_getCursorSpelling(cursor));
            MethodQualifiers quals(cursor);
            mOperation = mClassifier->addOperation(str, mClassMemberAccess,
                quals.isMethodConst(), quals.isMethodVirtual());
            CXStringDisposer sym = clang_getCursorUSR(cursor);
            mOperation->setOverloadKeyFromOperUSR(sym);
            addOperationParts(cursor, true);
            unsigned int line;
            FilePath fn(getFileLoc(cursor, &line), FP_File);
            if(fn == mTopParseFn)
                {
                mOperation->setLineNum(line);
                mOperation->setModule(mParserModelData.getParsedModule());
                }
            }
            break;

        default:
            {
            if(isField(cursor))
                {
                CXStringDisposer name = clang_getCursorDisplayName(cursor);
                RefType rt;
                ModelType *type = mParserModelData.createOrGetBaseTypeRef(cursor, rt);
                ModelAttribute *attr = mClassifier->addAttribute(name,
                    type, mClassMemberAccess.getVis());
                attr->setConst(rt.isConst);
                attr->setRefer(rt.isRef);
                }
            }
            break;
        }
    return CXChildVisit_Continue;
    }

// DEAD CODE
//void CppParser::addVar(CXCursor /*cursor*/)
/*
    {
// This could be added at some point to add the static/global variables.
// The global class could be added using createOrGetClassRef
// The cursor has two children, TypeRef and CallExpr.
    OovString name = getFullBaseTypeName(cursor);
#if(DEBUG_PARSE)
    dumpCursor(sLog.mFp, "addVar", cursor);
    debugDumpCursor(cursor);
#endif
    }
*/

CXChildVisitResult CppParser::visitTranslationUnit(CXCursor cursor,
    CXCursor /*parent*/)
    {
    sCrashDiagnostics.saveMostRecentParseLocation("TU", cursor);
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    switch(cursKind)
        {
        // Don't do enum
        case CXCursor_ClassTemplate:
            // CXCursor_TemplateTypeParameter
            // CXCursor_NonTypeTemplateParameter
            // CXCursor_TemplateTemplateParameter
            // CXCursor_ClassTemplatePartialSpecialization
            // CXCursor_TemplateRef
            addRecord(cursor, Visibility::Private);
            break;

        case CXCursor_ClassDecl:
            addRecord(cursor, Visibility::Private);
            break;

        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
            addRecord(cursor, Visibility::Public);
            break;

        case CXCursor_TypedefDecl:
            addTypedef(cursor);
            break;

        case CXCursor_VarDecl:
//          addVar(cursor);
            break;

        case CXCursor_FunctionDecl:
            {
            FilePath fn(getFileLoc(cursor), FP_File);
            if(fn == mTopParseFn)
                {
                // If this function is in a namespace, get the namespace and put
                // it into the class name.
                CXCursor classCursor = clang_getCursorSemanticParent(cursor);
                std::string className = getFullBaseTypeName(classCursor);
                mClassifier = mParserModelData.createOrGetClassRef(className);
                if(mClassifier)
                    {
                    CXStringDisposer funcName(clang_getCursorSpelling(cursor));         // Returns func name without namespace
                    mOperation = mClassifier->addOperation(funcName, mClassMemberAccess, false, false);
                    if(mOperation)
                        {
                        CXStringDisposer sym = clang_getCursorUSR(cursor);
                        mOperation->setOverloadKeyFromOperUSR(sym);
                        unsigned int line;
                        FilePath fn(getFileLoc(cursor, &line), FP_File);
                        addOperationParts(cursor, false);
                        mOperation->setModule(mParserModelData.getParsedModule());
                        mOperation->setLineNum(line);
                        }
                    }
                }
            }
            break;

        case CXCursor_Constructor:
        case CXCursor_CXXMethod:
        case CXCursor_Destructor:
//      case CXType_FunctionProto:      //  CXCursorKind = CXCursor_CXXMethod
            {
            mClassifier = nullptr;
            CXCursor classCursor = clang_getCursorSemanticParent(cursor);
            std::string className;
            if(classCursor.kind == CXCursor_ClassDecl ||
                    classCursor.kind == CXCursor_ClassTemplate)
                {
                className = getFullBaseTypeName(classCursor);
                }
            mClassifier = mParserModelData.createOrGetClassRef(className);
            if(mClassifier)
                {
                unsigned int line;
                FilePath fn(getFileLoc(cursor, &line), FP_File);
                // For CPP files, output the class definition also, so that the
                // function definitions get written out as part of the class. The
                // reader of the files will have to resolve duplicate .h/.cpp definitions.
                if(fn == mTopParseFn)
                    {
                    CXStringDisposer funcName(clang_getCursorSpelling(cursor));
                    MethodQualifiers quals(cursor);
                    ModelOperation tempOper(funcName, Visibility(),
                        quals.isMethodConst(), quals.isMethodVirtual());
                    CXStringDisposer sym = clang_getCursorUSR(cursor);
                    tempOper.setOverloadKeyFromOperUSR(sym);
                    mOperation = &tempOper;
                    // Prevent double display of arg parsing since it is in addOperationParts().
                    sCrashDiagnostics.enableDumpCursor(false);
                    clang_visitChildren(cursor, ::visitFunctionAddArgs, this);
                    sCrashDiagnostics.enableDumpCursor(true);
                    mOperation = const_cast<ModelOperation*>(mClassifier->getMatchingOperation(tempOper));
                    if(mOperation)
                        {
                        addOperationParts(cursor, false);
                        mOperation->setModule(mParserModelData.getParsedModule());
                        mOperation->setLineNum(line);
                        }
                    else
                        {
                        LogAssertFile(__FILE__, __LINE__,
                                mParserModelData.getParsedModule()->getName().getStr());
                        }
                    }
                }
            }
            break;

        // Some part of "extern "C" G_MODULE_EXPORT" is an unexposed decl.
        // So to get all function declarations, this must recurse to get children.
        // This must also get namespaces (CXCursor_Namespace).
        case CXCursor_Namespace:
        case CXCursor_UnexposedDecl:
            clang_visitChildren(cursor, ::visitTranslationUnit, this);
            break;

        // Don't recurse for everything in order to increase parse speed for
        // things that aren't interesting.
        default:
            break;
        }
//    return CXChildVisit_Recurse;
    return CXChildVisit_Continue;
    }

CXChildVisitResult CppParser::visitTranslationUnitForIncludes(CXCursor cursor,
    CXCursor /*parent*/)
    {
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    switch(cursKind)
        {
        case CXCursor_InclusionDirective:
            {
            CXFile file = clang_getIncludedFile(cursor);
            if(file)
                {
                std::string includerFn = getFileLoc(cursor);
                CXStringDisposer includedFn = clang_getFileName(file);
                CXStringDisposer includedNameString(clang_getCursorSpelling(cursor));

                FilePath absErFn;
                absErFn.getAbsolutePath(includerFn, FP_File);
                FilePath absEdFn;
                absEdFn.getAbsolutePath(includedFn, FP_File);

                // For each included file listed in the source, the include path for
                // the file is stored, and then the include string from the source file.

                // If it is a relative path, then the include path is the path
                // of the source file.
                if(includedNameString[0] == '.')
                    {
                    FilePath dirSepFilename(absErFn, FP_File);
                    dirSepFilename.discardFilename();
                    dirSepFilename += ';';
                    dirSepFilename +=includedNameString;
                    absEdFn = dirSepFilename;
                    }
                else
                    {
                    // Find the base path minus the included path. This is for
                    // includes like "include <gtk/gtk.h>"
                    FilePath edFullPath(absEdFn, FP_File);
                    FilePath edName(includedNameString, FP_File);
    #ifndef __linux__
                    StringToLower(edFullPath);
                    StringToLower(edName);
    #endif
                    size_t pos = edFullPath.find(edName, absEdFn.length() -
                            includedNameString.length());
                    if(pos != std::string::npos)
                        {
                        absEdFn.insert(pos, 1, ';');
                        }
                    else
                        {
                        fprintf(stderr, "Unable to make %s: \n   %s\n",
                            mIncDirDeps.getFilename().c_str(), absEdFn.c_str());
                        fflush(stdout);
                        DebugAssert(__FILE__, __LINE__);
                        }
                    }

//printf("%s\n  %s\n", absErFn.c_str(), absEdFn.c_str());
//fflush(stdout);
                mIncDirDeps.insert(absErFn, absEdFn);
                }
            }
            break;

        default:
            break;
        }
    return CXChildVisit_Continue;
    }

#define LINE_STATS 1
#if(LINE_STATS)
static ModelModuleLineStats makeLineStats(CXCursor cursor)
    {
    unsigned int numCodeLines = 0;
    unsigned int numCommentLines = 0;
    unsigned int totalLines = 0;
    CXSourceRange tuRange = clang_getCursorExtent(cursor);
    CXToken *tokens = 0;
    unsigned int nTokens = 0;
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    // clang_tokenize is a strange function. Sometimes it returns a last
    // token that is part of the cursor, and sometimes it returns a last token
    // that is part of the next cursor.
    clang_tokenize(tu, tuRange, &tokens, &nTokens);
    unsigned int lastCommentLine = 0;
    unsigned int lastCodeLine = 0;
    for (size_t i = 0; i < nTokens; i++)
        {
        CXTokenKind kind = clang_getTokenKind(tokens[i]);
//      CXStringDisposer spelling = clang_getTokenSpelling(tu, tokens[i]);
//      size_t n = std::count(spelling.begin(), spelling.end(), '\n');  // No line feeds in tokens
        CXSourceRange tokRange = clang_getTokenExtent(tu, tokens[i]);
        CXSourceLocation startLoc = clang_getRangeStart(tokRange);
        CXSourceLocation endLoc = clang_getRangeEnd(tokRange);
        unsigned startLine;
        unsigned endLine;
        clang_getExpansionLocation(startLoc, nullptr, &startLine, nullptr, nullptr);
        clang_getExpansionLocation(endLoc, nullptr, &endLine, nullptr, nullptr);
        unsigned int n = endLine - startLine;
        if(n == 0)
            {
            n = 1;
            }
        totalLines = endLine;
        if(kind == CXToken_Comment)
            {
            if(lastCommentLine != endLine)
                {
                numCommentLines += n;
                lastCommentLine = endLine;
                }
            }
        else
            {
            if(lastCodeLine != endLine)
                {
                numCodeLines += n;
                lastCodeLine = endLine;
                }
            }
        }
    clang_disposeTokens(tu, tokens, nTokens);
    return ModelModuleLineStats(numCodeLines, numCommentLines, totalLines);
    }
#endif

CppParser::eErrorTypes CppParser::parse(bool lineHashes, char const * const srcFn,
        char const * const srcRootDir, char const * const outDir,
        char const * const clang_args[], int num_clang_args)
    {
    eErrorTypes errType = ET_None;

    mTopParseFn.setPath(srcFn, FP_File);
    /// Create a module so the modelwriter has a filename.
    mParserModelData.addParsedModule(srcFn);

    mIncDirDeps.read(outDir, Project::getAnalysisIncDepsFilename());

    CXIndex index = clang_createIndex(1, 1);

    std::string outBaseFileName = Project::makeOutBaseFileName(srcFn,
            srcRootDir, outDir);
    if(lineHashes)
        {
        FilePath fn(outBaseFileName, FP_File);
        FilePath outHashFilePath(outDir, FP_Dir);

        outHashFilePath.discardTail(outHashFilePath.getPosLeftPathSep(
                outHashFilePath.getPosEndDir(), RP_RetPosNatural));
        outHashFilePath.appendDir(DupsDir);
        OovStatus status = outHashFilePath.ensurePathExists();
        if(status.ok())
            {
            outHashFilePath.appendFile(fn.getName());
            outHashFilePath.appendExtension(DupsHashExtension);
            mDupHashFile.open(outHashFilePath);
            }
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to create duplicates dir");
            }
        }

// This doesn't appear to change anything.
//    clang_toggleCrashRecovery(true);
    // Get inclusion directives to be in AST.
    unsigned options = CXTranslationUnit_DetailedPreprocessingRecord;
//    unsigned options = 0;
    CXTranslationUnit tu;

    CXErrorCode errCode = CXError_Success;
    try
        {
        errCode = clang_parseTranslationUnit2(index, srcFn,
                clang_args, num_clang_args, 0, 0, options, &tu);
    }
    catch(...)
        {

        }
    if(errCode == CXError_Success)
        {
        CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
        try
            {
            clang_visitChildren(rootCursor, ::visitTranslationUnit, this);
            clang_visitChildren(rootCursor, ::visitTranslationUnitForIncludes, this);
#if(LINE_STATS)
            mParserModelData.setLineStats(makeLineStats(rootCursor));
#endif
            }
        catch(...)
            {
            errType = ET_ParseError;
            sCrashDiagnostics.setCrashed();
            }

#if(DEBUG_PARSE)
        fprintf(sLog.mFp, "DUMP TYPES\n");
        for(const auto & tp: mParserModelData.DebugGetModelData().mTypes)
            {
            fprintf(sLog.mFp, "%s %p\n", tp.get()->getName().c_str(), tp.get());
            }
        fflush(sLog.mFp);
#endif

        try
            {
            OovString outXmiFileName = outBaseFileName;
            outXmiFileName += ".xmi";
            mParserModelData.writeModel(outXmiFileName);
            }
        catch(...)
            {
            errType = ET_ParseError;
            }
        try
            {
            mIncDirDeps.write();
            }
        catch(...)
            {
            errType = ET_ParseError;
            }
        std::string outErrFileName = outBaseFileName;
        outErrFileName += "-err.txt";
        unsigned int numDiags = clang_getNumDiagnostics(tu);
        if(numDiags > 0 || sCrashDiagnostics.hasCrashed())
            {
            FILE *fp = fopen(outErrFileName.c_str(), "w");
            if(fp)
                {
                sCrashDiagnostics.dumpCrashed(fp);
                for (unsigned int i = 0; i<numDiags; i++)
                    {
                    CXDiagnostic diag = clang_getDiagnostic(tu, i);
                    CXDiagnosticSeverity sev = clang_getDiagnosticSeverity(diag);
                    CXStringDisposer diagStr = clang_formatDiagnostic(diag,
                        clang_defaultDiagnosticDisplayOptions());
                    switch(sev)
                        {
                        case CXDiagnostic_Warning:
                            if(errType != ET_CompileErrors)
                                {
                                errType = ET_CompileWarnings;
                                }
                            // Adding "OovCppParser:" prevents editor from loading properly.
                            fprintf(stdout, "%s\n", diagStr.c_str());
                            break;

                        case CXDiagnostic_Error:
                        case CXDiagnostic_Fatal:
                            errType = ET_CompileErrors;
// @todo - These are displayed elsewhere at the moment.
                            // Adding "OovCppParser:" prevents editor from loading properly.
//                          fprintf(stderr, "%s\n", diagStr.c_str());
                            break;

                        case CXDiagnostic_Note:
                        case CXDiagnostic_Ignored:
                            break;
                        }
                    fprintf(fp, "%s\n", diagStr.c_str());
                    }

                fprintf(fp, "Arguments: %s %s %s ", srcFn, srcRootDir, outDir);
                for(int i=0 ; i<num_clang_args; i++)
                    fprintf(fp, "%s ", clang_args[i]);
                fprintf(fp, "\n");

                fclose(fp);
                }
            }
        else
            {
            unlink(outErrFileName.c_str());
            }
        }
    else
        {
        errType = ET_CLangError;
        }
    fflush(stdout);
    fflush(stderr);
    return errType;
    }
