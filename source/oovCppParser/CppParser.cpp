/*
 * CppParser.cpp
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
*/

#include "CppParser.h"
#include "Project.h"
#include "ModelWriter.h"
#include "Debug.h"
#include "NameValueFile.h"
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
// Prevent "error: 'off_t' has not been declared"
#define off_t _off_t
#include <unistd.h>		// for unlink
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
#define DEBUG_PARSE 0
#if(DEBUG_PARSE)
static DebugFile sLog("DebugCppParse.txt", false);
#endif

#define SHARED_FILE 1

void IncDirDependencyMap::read(char const * const outDir, char const * const incFn)
    {
    FilePath outIncFileName(outDir, FP_Dir);
    outIncFileName.appendFile(incFn);
    setFilename(outIncFileName);
#if(SHARED_FILE)
    if(!readFileShared())
	{
	fprintf(stderr, "\nOovCppParser - Read file sharing error\n");
	}
#else
    readFile();
#endif
    }

/// For every includer file that is run across during parsing, this means that
/// the includer file was fully parsed, and that no old included information
/// needs to be kept, except to check if the old included information has not
/// changed.  This implies that tricky ifdefs must be the same for all
/// included files.
void IncDirDependencyMap::write()
    {
    time_t curTime;
    time_t changedTime = 0;
    time(&curTime);

#if(SHARED_FILE)
    SharedFile file;
    if(!writeFileExclusiveReadUpdate(file))
	{
	fprintf(stderr, "\nOovCppParser - Write file sharing error\n");
	}
#endif
    // Append new values from parsed includes to the NameValueFile.
    // Update existing values times and make new dependencies.
    for(const auto &mapItem : mParsedIncludeDependencies)
	{
	CompoundValue compVal;
	// First check if the includer filename exists in the dependency file.
	OovString val = getValue(mapItem.first);
	if(val.size() > 0)
	    {
	    // get the changed time, which is the first value and discard
	    // everything else.
	    compVal.parseString(val);
	    bool changed = false;
	    if(compVal.size()-2 != mapItem.second.size())
		{
		changed = true;
		}
	    else
		{
		for(const auto &included : mapItem.second)
		    {
		    if(std::find(compVal.begin(), compVal.end(), included) ==
			    compVal.end())
			{
			changed = true;
			}
		    }
		}
	    if(changed)
		changedTime = curTime;
	    else
		{
		OovString timeStr(compVal[0]);
		unsigned int timeVal;
		timeStr.getUnsignedInt(0, UINT_MAX, timeVal);
		changedTime = timeVal;
		}
	    compVal.clear();
	    }

	OovString changeStr;
	changeStr.appendInt(changedTime);
	compVal.addArg(changeStr);

	OovString checkedStr;
	checkedStr.appendInt(curTime);
	compVal.addArg(checkedStr);

	for(const auto &str : mapItem.second)
	    {
	    size_t pos = compVal.find(str);
	    if(pos == CompoundValue::npos)
		{
		compVal.addArg(str);
		}
	    }
	setNameValue(mapItem.first, compVal.getAsString());
	}
    if(file.isOpen())
	{
#if(SHARED_FILE)
	writeFileExclusive(file);
#else
	writeFile();
#endif
	}
    }

void IncDirDependencyMap::insert(const std::string &includerFn,
	const FilePath &includedFn)
    {
#ifdef __linux__

#else
    if(includedFn.getPosPathSegment("mingw") == std::string::npos)
#endif
	{
	mParsedIncludeDependencies[includerFn].insert(includedFn);
	}
    }


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
	removeLastNonIdentChar(expr);	// Remove the ']'
	removeLastNonIdentChar(expr);	// Remove the ' '
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


static std::string getFileLoc(CXCursor cursor, int *retLine = nullptr)
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

class cCrashDiagnostics
    {
    public:
	cCrashDiagnostics():
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
static cCrashDiagnostics sCrashDiagnostics;


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


ModelType *CppParser::createOrGetBaseTypeRef(CXCursor cursor, RefType &rt)
    {
    CXType cursorType = clang_getCursorType(cursor);
    eModelDataTypes dType = (cursorType.kind == CXType_Record) ? DT_Class : DT_DataType;
    rt.isConst = isConstType(cursor);
    switch(cursorType.kind)
	{
	case CXType_LValueReference:
	case CXType_RValueReference:
	case CXType_Pointer:
	    rt.isRef = true;
	    break;

	default:
	    break;
	}
    std::string typeName = getFullBaseTypeName(cursor);

#if(DEBUG_PARSE)
    if(sLog.mFp && !mModelData.getTypeRef(typeName))
	fprintf(sLog.mFp, "    Create Type Ref: %s\n", typeName.c_str());
#endif
    return mModelData.createOrGetTypeRef(typeName, dType);
    }

ModelType *CppParser::createOrGetDataTypeRef(CXType cursType, RefType &rt)
    {
    ModelType *type = nullptr;
    rt.isConst = isConstType(cursType);
    switch(cursType.kind)
	{
	case CXType_LValueReference:
	case CXType_RValueReference:
	case CXType_Pointer:
	    rt.isRef = true;
	    break;

	default:
	    break;
	}
//    CXCursor retTypeDeclCursor = clang_getTypeDeclaration(cursType);
    CXStringDisposer retTypeStr(clang_getTypeSpelling(cursType));
//    if(retTypeDeclCursor.kind != CXCursor_NoDeclFound)
//	{
//	type = mModelData.createOrGetTypeRef(retTypeStr, DT_Class);
//	}
//    else
	{
	type = mModelData.createOrGetTypeRef(retTypeStr, DT_DataType);
	}
    return type;
    }

// This upgrades an otDatatype to an otClass in order to handle forward references
ModelClassifier *CppParser::createOrGetClassRef(OovStringRef const name)
    {
    ModelClassifier *classifier = nullptr;
#if(DEBUG_PARSE)
    if(sLog.mFp && !mModelData.getTypeRef(name))
	fprintf(sLog.mFp, "    Create Type Class: %s\n", name.getStr());
#endif
    ModelType *type = mModelData.createOrGetTypeRef(name, DT_Class);
    if(type->getDataType() == DT_Class)
	{
	classifier = static_cast<ModelClassifier*>(type);
	}
    else
	{
	classifier = static_cast<ModelClassifier*>(mModelData.createTypeRef(name, DT_Class));
	mModelData.replaceType(type, classifier);
	}
    return classifier;
    }

ModelType *CppParser::createOrGetDataTypeRef(CXCursor cursor)
    {
    std::string typeName = getFullBaseTypeName(cursor);
    return mModelData.createOrGetTypeRef(typeName, DT_DataType);
    }

void CppParser::addOperationParts(CXCursor cursor, bool addParams)
    {
    // When a function outside of the class is defined, there is no need to add parameters again.
    if(addParams)
	{
	clang_visitChildren(cursor, ::visitFunctionAddArgs, this);
	}
#if(OPER_RET_TYPE)
    CXType retType = clang_getResultType(clang_getCursorType(cursor));
    RefType rt;
    mOperation->getReturnType().setDeclType(createOrGetDataTypeRef(retType, rt));
    mOperation->getReturnType().setConst(rt.isConst);
    mOperation->getReturnType().setRefer(rt.isRef);
#endif
    mStatements = &mOperation->getStatements();
    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
    if(mDupHashFile.isOpen())
	{
	FilePath fn(getFileLoc(cursor), FP_File);
	if(fn == mTopParseFn)
	    {
	    mDupHashFile.appendBreak();
	    clang_visitChildren(cursor, ::visitFunctionAddDupHashes, this);
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
CXChildVisitResult CppParser::visitFunctionAddArgs(CXCursor cursor, CXCursor parent)
    {
    sCrashDiagnostics.saveMostRecentParseLocation("FA", cursor);
    CXCursorKind ckind = clang_getCursorKind(cursor);
    if(ckind == CXCursor_ParmDecl)
	{
	CXStringDisposer name = clang_getCursorDisplayName(cursor);
	RefType rt;
	ModelType *type = createOrGetBaseTypeRef(cursor, rt);
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
	case CX_CXXPrivate:		access.setVis(Visibility::Private);	break;
	case CX_CXXProtected:		access.setVis(Visibility::Protected);	break;
	case CX_CXXPublic:		access.setVis(Visibility::Public);	break;
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
static CXChildVisitResult getConditionStr(CXCursor cursor, CXCursor parent,
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
//	removeLastNonIdentChar(*op);
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
	removeLastNonIdentChar(fullop);	// Remove the ':' from the case.
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

void cDupHashFile::append(OovStringRef const text, unsigned int line)
    {
#if(DEBUG_HASH)
    fprintf(mFile.getFp(), "%x %u %s\n", makeHash(text), line, text.getStr());
#else
    fprintf(mFile.getFp(), "%x %u\n", makeHash(text), line);
#endif
    mAlreadyAddedBreak = false;
    }

CXChildVisitResult CppParser::visitFunctionAddDupHashes(CXCursor cursor,
    CXCursor parent)
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


// Takes a CXType_FunctionProto cursor, and finds conditional statements and
// calls to functions.
CXChildVisitResult CppParser::visitFunctionAddStatements(CXCursor cursor, CXCursor parent)
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
	    addCondStatement(cursor, 1, 0);
	    break;

//	case CXCursor_GotoStmt, break, continue, return, CXCursor_DefaultStmt
	/// Conditional statements
	case CXCursor_WhileStmt:	//   enum { VAR, COND, BODY, END_EXPR };
	    addCondStatement(cursor, 0, 1);	// cond body
	    break;

	case CXCursor_CaseStmt:
//	    addCondStatement(cursor, 0, 1);	// cond body
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
	    addCondStatement(cursor, 1, 5);
	    break;

	case CXCursor_ForStmt:
	    addCondStatement(cursor, 1, 3);
	    break;

	case CXCursor_IfStmt:	    	//    enum { VAR, COND, THEN, ELSE, END_EXPR };
	    addCondStatement(cursor, 0, 1);	// cond=0, ifbody=1, [elsebody]=2
	    addElseStatement(cursor, 2);
	    break;

	case CXCursor_CallExpr:
	    {
	    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
	    CXStringDisposer functionName = clang_getCursorDisplayName(cursor);

	    // This returns the CXCursor_CXXMethod cursor
	    CXCursor cursorDef = clang_getCursorDefinition(cursor);
	    if(cursorDef.kind == CXCursor_FirstInvalid)
		cursorDef = clang_getCursorReferenced(cursor);
	    if(functionName.length() != 0)
		{
		/// @todo - fix to support class1.class2.call(). This is tricky because
		/// the sequence diagram needs to know the type of class2 for class2.call,
		/// but complexity or class relations know the type of the class1 relation.
		CXCursor classCursor = clang_getCursorSemanticParent(cursorDef);
		RefType rt;
		CXCursor memberRefCursor = getCursorCallMemberRef(cursor);
		if(!clang_Cursor_isNull(memberRefCursor))
		    {
		    CXStringDisposer attrName = clang_getCursorDisplayName(memberRefCursor);
		    attrName += '.';
		    functionName.insert(0, attrName);
		    }
		const ModelType *classType = createOrGetBaseTypeRef(classCursor, rt);
		ModelStatement stmt(functionName, ST_Call);
		stmt.getClassDecl().setDeclType(classType);
		mStatements->addStatement(stmt);
		}
	    }
	    break;

//	case CXCursor_CXXCatchStmt:
//	    break;

	case CXCursor_BinaryOperator:
	    {
	    // If assignment operator, and on lhs, variable is modified.
	    CXCursor lhsChild = getNthChildCursor(cursor, 0);
	    std::string lhsStr;
	    // This returns the operator characters at the end.
	    appendCursorTokenString(lhsChild, lhsStr);

//	    CXStringDisposer lhsStr = clang_getCursorSpelling(lhsChild);
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
		ModelType *type = createOrGetBaseTypeRef(cursor, rt);
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
	case CXCursor_MemberRefExpr:
	case CXCursor_MemberRef:
	    {
	    CXStringDisposer name = clang_getCursorDisplayName(cursor);

	    // This returns the CXCursor_CXXMethod cursor
	    CXCursor cursorDef = clang_getCursorDefinition(cursor);
	    if(cursorDef.kind == CXCursor_FieldDecl)
		{
		/// @todo - fix to support class1.class2.member
		CXCursor classCursor = clang_getCursorSemanticParent(cursorDef);
		if(classCursor.kind == CXCursor_ClassDecl || classCursor.kind == CXCursor_StructDecl)
		    {
		    RefType rt;
		    const ModelType *classType = createOrGetBaseTypeRef(classCursor, rt);
		    ModelStatement stmt(name, ST_VarRef);
		    stmt.getClassDecl().setDeclType(classType);

		    const ModelType *varType = createOrGetBaseTypeRef(cursor, rt);
		    stmt.getVarDecl().setDeclType(varType);

		    stmt.setVarAccessWrite(modifiedLhs);
		    mStatements->addStatement(stmt);
		    }
		else
		    {
//		    LogAssertFile(__FILE__, __LINE__);
		    }
		}
	    else if(cursorDef.kind != CXCursor_CXXMethod && cursorDef.kind != CXCursor_FirstInvalid &&
		    cursorDef.kind != CXCursor_ConversionFunction)
		{
		LogAssertFile(__FILE__, __LINE__);
		}
	    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
	    }
	    break;

//	case CXCursor_BinaryOperator:
//		The first child is the rhs. second is lhs.
//		If first child is memberrefexpr, then it is read only
//	    break;

//	case CXCursor_CompoundStmt:
	default:
	    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
	    break;
	}
#if(DEBUG_PARSE)
    mStatementRecurseLevel--;
#endif
    return CXChildVisit_Continue;
    }

void CppParser::addRecord(CXCursor cursor, Visibility vis)
    {
    // Save all classes so that the access specifiers in the declared classes
    // from other TU's are saved. (Ex, the header for this source file)
    OovString name = getFullBaseTypeName(cursor);
    mClassifier = createOrGetClassRef(name);
    if(mClassifier)
	{
	int line;
	std::string fn = getFileLoc(cursor, &line);
	// Indicate to the modelwriter, that this class is in this TU.
	FilePath normFn(fn, FP_File);
	if(normFn == mTopParseFn)
	    {
	    mClassifier->setModule(mModelData.mModules[0].get());
	    }
	else
	    {
//	    mClassifier->setModule(mModelData.mModules[0]);
	    }
	mClassifier->setLineNum(line);
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
		ModelClassifier *child = static_cast<ModelClassifier*>(createOrGetDataTypeRef(parent));
		ModelClassifier *parent = static_cast<ModelClassifier*>(createOrGetDataTypeRef(cursor));
		ModelAssociation *assoc = new ModelAssociation(child,
		    parent, getAccess(cursor));
		/// @todo - use make_unique when supported.
		mModelData.mAssociations.push_back(std::unique_ptr<ModelAssociation>(assoc));
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
	    bool isConst = isMethodConst(cursor);
	    mOperation = mClassifier->addOperation(str, mClassMemberAccess, isConst);
	    addOperationParts(cursor, true);
	    int line;
	    FilePath fn(getFileLoc(cursor, &line), FP_File);
	    if(fn == mTopParseFn)
		{
		mOperation->setLineNum(line);
		mOperation->setModule(mModelData.mModules[0].get());
		}
	    }
	    break;

	default:
	    {
	    if(isField(cursor))
		{
		CXStringDisposer name = clang_getCursorDisplayName(cursor);
		RefType rt;
		ModelType *type = createOrGetBaseTypeRef(cursor, rt);
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

CXChildVisitResult CppParser::visitTranslationUnit(CXCursor cursor, CXCursor parent)
    {
    sCrashDiagnostics.saveMostRecentParseLocation("TU", cursor);
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    switch(cursKind)
	{
	case CXCursor_Namespace:
	    clang_visitChildren(cursor, ::visitTranslationUnit, this);
	    break;

	// Don't do enum
	case CXCursor_ClassTemplate:
	    // CXCursor_TemplateTypeParameter
	    // CXCursor_NonTypeTemplateParameter
	    // CXCursor_TemplateTemplateParameter
	    // CXCursor_ClassTemplatePartialSpecialization
	    // CXCursor_TemplateRef
	    addRecord(cursor, Visibility::Public);
	    break;

	case CXCursor_ClassDecl:
	    addRecord(cursor, Visibility::Public);
	    break;

	case CXCursor_StructDecl:
	case CXCursor_UnionDecl:
	    addRecord(cursor, Visibility::Private);
	    break;

	case CXCursor_Constructor:
	case CXCursor_CXXMethod:
//	case CXType_FunctionProto:	//  CXCursorKind = CXCursor_CXXMethod
	    {
	    mClassifier = nullptr;
	    CXCursor classCursor = clang_getCursorSemanticParent(cursor);
	    std::string className;
	    if(classCursor.kind == CXCursor_ClassDecl ||
		    classCursor.kind == CXCursor_ClassTemplate)
		{
		className = getFullBaseTypeName(classCursor);
		}
	    mClassifier = createOrGetClassRef(className);
	    if(mClassifier)
		{
		int line;
		FilePath fn(getFileLoc(cursor, &line), FP_File);
		// For CPP files, output the class definition also, so that the
		// function definitions get written out as part of the class. The
		// reader of the files will have to resolve duplicate .h/.cpp definitions.
		if(fn == mTopParseFn)
		    {
		    CXStringDisposer funcName(clang_getCursorSpelling(cursor));
		    ModelOperation tempOper(funcName, Visibility(), isMethodConst(cursor));
		    mOperation = &tempOper;
		    // Prevent double display of arg parsing since it is in addOperationParts().
		    sCrashDiagnostics.enableDumpCursor(false);
		    clang_visitChildren(cursor, ::visitFunctionAddArgs, this);
		    sCrashDiagnostics.enableDumpCursor(true);
		    mOperation = const_cast<ModelOperation*>(mClassifier->findExactMatchingOperation(tempOper));
		    if(mOperation)
			{
			addOperationParts(cursor, false);
			mOperation->setModule(mModelData.mModules[0].get());
			mOperation->setLineNum(line);
			}
		    else
			{
			LogAssertFile(__FILE__, __LINE__);
			}
		    }
		}
	    }
	    break;

	default:
	    break;
	}
//    return CXChildVisit_Recurse;
    return CXChildVisit_Continue;
    }

CXChildVisitResult CppParser::visitTranslationUnitForIncludes(CXCursor cursor, CXCursor parent)
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
		    fprintf(stderr, "Unable to make oovcde-incdeps.txt: \n   %s\n",
			    absEdFn.c_str());
		    DebugAssert(__FILE__, __LINE__);
		    }
		mIncDirDeps.insert(absErFn, absEdFn);
		}
	    }
	    break;

	default:
	    break;
	}
    return CXChildVisit_Continue;
    }

CppParser::eErrorTypes CppParser::parse(bool lineHashes, char const * const srcFn,
	char const * const srcRootDir, char const * const outDir,
	char const * const clang_args[], int num_clang_args)
    {
    eErrorTypes errType = ET_None;

    mTopParseFn.setPath(srcFn, FP_File);
    /// Create a module so the modelwriter has a filename.
    ModelModule *module = new ModelModule();;
    module->setModulePath(srcFn);
    /// @todo - use make_unique when supported.
    mModelData.mModules.push_back(std::unique_ptr<ModelModule>(module));
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
	outHashFilePath.ensurePathExists();
	outHashFilePath.appendFile(fn.getName());
	outHashFilePath.appendExtension(DupsHashExtension);
	mDupHashFile.open(outHashFilePath);
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
	    }
	catch(...)
	    {
	    errType = ET_ParseError;
	    sCrashDiagnostics.setCrashed();
	    }

#if(DEBUG_PARSE)
	fprintf(sLog.mFp, "DUMP TYPES\n");
	for(const auto & tp: mModelData.mTypes)
	    {
	    fprintf(sLog.mFp, "%s %p\n", tp.get()->getName().c_str(), tp.get());
	    }
	fflush(sLog.mFp);
#endif

	try
	    {
	    OovString outXmiFileName = outBaseFileName;
	    outXmiFileName += ".xmi";
	    ModelWriter writer(mModelData);
	    writer.writeFile(outXmiFileName);
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
	int numDiags = clang_getNumDiagnostics(tu);
	if(numDiags > 0 || sCrashDiagnostics.hasCrashed())
	    {
	    FILE *fp = fopen(outErrFileName.c_str(), "w");
	    if(fp)
		{
		sCrashDiagnostics.dumpCrashed(fp);
		for (int i = 0; i<numDiags; i++)
		    {
		    CXDiagnostic diag = clang_getDiagnostic(tu, i);
		    CXDiagnosticSeverity sev = clang_getDiagnosticSeverity(diag);
		    if(errType == ET_None || errType == ET_CompileWarnings)
			{
			if(sev >= CXDiagnostic_Error)
			    errType = ET_CompileErrors;
			else
			    errType = ET_CompileWarnings;
			}
		    CXStringDisposer diagStr = clang_formatDiagnostic(diag,
			clang_defaultDiagnosticDisplayOptions());
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
    return errType;
    }
