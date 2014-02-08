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
#include "OovString.h"
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
// Prevent "error: 'off_t' has not been declared"
#define off_t _off_t
#include <unistd.h>		// for unlink
#include <limits.h>
#include <algorithm>


#define DEBUG_PARSE 0

#if(DEBUG_PARSE)
static DebugFile sLog("DebugCppParse.txt");
#endif

void IncDirDependencyMap::read(char const * const outDir, char const * const incFn)
    {
    FilePath outIncFileName(outDir, FP_Dir);
    outIncFileName.appendFile(incFn);
    setFilename(outIncFileName.c_str());
    readFile();
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

    // Append new values from parsed includes to the NameValueFile.
    // Update existing values times and make new dependencies.
    for(const auto &mapItem : mParsedIncludeDependencies)
	{
	CompoundValue compVal;
	// First check if the includer filename exists in the dependency file.
	std::string val = getValue(mapItem.first.c_str());
	if(val.size() > 0)
	    {
	    // get the changed time, which is the first value and discard
	    // everything else.
	    compVal.parseString(val.c_str());
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
		OovString timeStr(compVal[0].c_str());
		unsigned int timeVal;
		timeStr.getUnsignedInt(0, UINT_MAX, timeVal);
		changedTime = timeVal;
		}
	    compVal.clear();
	    }

	OovString changeStr;
	changeStr.setInt(changedTime);
	compVal.addArg(changeStr.c_str());

	OovString checkedStr;
	checkedStr.setInt(curTime);
	compVal.addArg(checkedStr.c_str());

	for(const auto &str : mapItem.second)
	    {
	    int pos = compVal.find(str.c_str());
	    if(pos == CompoundValue::npos)
		{
		compVal.addArg(str.c_str());
		}
	    }
	setNameValue(mapItem.first.c_str(), compVal.getAsString().c_str());
	}
    writeFile();
    }

void IncDirDependencyMap::insert(const std::string &includerFn,
	const FilePath &includedFn)
    {
#ifdef __linux__

#else
    if(includedFn.findPathSegment("mingw") == std::string::npos)
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
	buildTokenStringForCursor(cursor, tokenStr);
	if(tokenStr.length() > 100)
	    {
	    tokenStr.resize(100);
	    tokenStr += "...";
	    }

	fprintf(fp, "%s: %s %s   %s\n", str, spstr.c_str(), name.c_str(), tokenStr.c_str());
	fflush(fp);
	}
    }
#if(DEBUG_PARSE)
static void logStr(char const * const str)
    {
    if(sLog.mFp)
	{
	fprintf(sLog.mFp, "%s\n", str);
	fflush(sLog.mFp);
	}
    }
static void logStrPtr(char const * const str, void const * const ptr)
    {
    if(sLog.mFp)
	{
	fprintf(sLog.mFp, "%s %p\n", str, ptr);
	fflush(sLog.mFp);
	}
    }
#endif

class cCrashDiagnostics
    {
    public:
	cCrashDiagnostics():
	    mCrashed(false)
	    {}
	void saveMostRecentParseLocation(char const * const diagStr, CXCursor cursor)
	    {
	    mDiagStr = diagStr;
	    mMostRecentCursor = cursor;
#if(DEBUG_PARSE)
	    sLog.open("DebugCppParse.txt");
	    dumpCursor(sLog.mFp, diagStr, cursor);
#endif
	    }
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

static CXChildVisitResult visitFunctionAddVars(CXCursor cursor, CXCursor parent,
	CXClientData client_data)
    {
    CppParser *parser = static_cast<CppParser*>(client_data);
    return parser->visitFunctionAddVars(cursor, parent);
    }

static CXChildVisitResult visitFunctionAddStatements(CXCursor cursor, CXCursor parent,
	CXClientData client_data)
    {
    CppParser *parser = static_cast<CppParser*>(client_data);
    return parser->visitFunctionAddStatements(cursor, parent);
    }



/////////////////////////


ModelType *CppParser::createOrGetTypeRef(CXCursor cursor, char const * const name,
	SplitType &st)
    {
    std::string typeStr;
    if(cursor.kind == CXCursor_FieldDecl || cursor.kind == CXCursor_ParmDecl ||
	    cursor.kind == CXCursor_VarDecl)
	{
	buildTokenStringForCursor(cursor, typeStr);
	removeTypeRefEnd(typeStr);
	}
    else
	{
	buildTokenStringForCursor(cursor, typeStr);
	typeStr += "-***";
	}
    st.setSplitType(typeStr.c_str(), name);
#if(DEBUG_PARSE)
    sLog.open("DebugCppParse.txt");
    if(sLog.mFp)
	fprintf(sLog.mFp, "    %s\n", st.baseType.c_str());
#endif
    CXType cursortype = clang_getCursorType(cursor);
    ObjectType oType = (cursortype.kind == CXType_Record) ? otClass : otDatatype;
    return mModelData.createOrGetTypeRef(st.baseType.c_str(), oType);
    }

// This upgrades an otDatatype to an otClass in order to handle forward references
ModelClassifier *CppParser::createOrGetClassRef(char const * const name)
    {
    ModelClassifier *classifier = nullptr;
    ModelType *type = mModelData.createOrGetTypeRef(name, otClass);
    if(type->getObjectType() == otClass)
	{
	classifier = static_cast<ModelClassifier*>(type);
	}
    else
	{
	classifier = static_cast<ModelClassifier*>(mModelData.createTypeRef(name, otClass));
	mModelData.replaceType(type, classifier);
	}
    return classifier;
    }

void CppParser::addOperationParts(CXCursor cursor, bool addParams)
    {
    // When a function outside of the class is defined, there is no need to add parameters again.
    if(addParams)
	{
	clang_visitChildren(cursor, ::visitFunctionAddArgs, this);
	}
    clang_visitChildren(cursor, ::visitFunctionAddVars, this);
    mCondStatements = &mOperation->getCondStatements();
    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
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
	SplitType st;
	ModelType *type = createOrGetTypeRef(cursor, name.c_str(), st);
	if(type)
	    {
	    ModelFuncParam *param = mOperation->addMethodParameter(name, type, false);
	    param->setConst(st.isConst);
	    param->setRefer(st.isRef);
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

static std::string getFileLoc(CXCursor cursor, int *retLine = nullptr)
    {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    CXFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    clang_getExpansionLocation(loc, &file, &line, &column, &offset);
    CXStringDisposer fn = clang_getFileName(file);
    if(retLine)
	*retLine = line;
    return fn;
    }

// Takes a CXType_Record cursor and adds parts of a class including
// inheritance, inline functions, and member variables.
CXChildVisitResult CppParser::visitRecord(CXCursor cursor, CXCursor parent)
    {
    sCrashDiagnostics.saveMostRecentParseLocation("R ", cursor);
    CXCursorKind ckind = clang_getCursorKind(cursor);
    if(ckind == CXCursor_CXXBaseSpecifier)
	{
	std::string fn = getFileLoc(cursor);
	if(fn == mTopParseFn)
	    {
	    CXStringDisposer name = clang_getCursorDisplayName(cursor);
	    CXStringDisposer parentName = clang_getCursorDisplayName(parent);
	    removeClass(name);
	    // The parentName contains the child class.
	    ModelClassifier *child = static_cast<ModelClassifier*>(
		    mModelData.createOrGetTypeRef(parentName.c_str(), otClass));
	    ModelClassifier *parent = static_cast<ModelClassifier*>(
		    mModelData.createOrGetTypeRef(name.c_str(), otClass));
	    ModelAssociation *assoc = new ModelAssociation(child,
		parent, getAccess(cursor));
	    mModelData.mAssociations.push_back(assoc);
	    }
	}
    else if(ckind == CXCursor_CXXAccessSpecifier)
	{
	mClassMemberAccess = getAccess(cursor);
	}
    else
	{
	CXType cursortype = clang_getCursorType(cursor);
	switch(cursortype.kind)
	    {
	    case CXType_FunctionProto:
		{
		CXStringDisposer str(clang_getCursorSpelling(cursor));
		bool isConst = isMethodConst(cursor);
		mOperation = mClassifier->addOperation(str, mClassMemberAccess, isConst);
		addOperationParts(cursor, true);
		}
		break;

	    case CXType_Invalid:
		break;

	    default:
		{
		if(isField(cursor))
		    {
		    CXStringDisposer name = clang_getCursorDisplayName(cursor);
		    SplitType st;
		    ModelType *type = createOrGetTypeRef(cursor, name.c_str(), st);
		    ModelAttribute *attr = mClassifier->addAttribute(name,
			    type, mClassMemberAccess.getVis());
		    attr->setConst(st.isConst);
		    attr->setRefer(st.isRef);
		    }
		}
		break;
	    }
	}
    return CXChildVisit_Continue;
    }

// Finds variable declarations inside function bodies.
CXChildVisitResult CppParser::visitFunctionAddVars(CXCursor cursor, CXCursor parent)
    {
    sCrashDiagnostics.saveMostRecentParseLocation("FV", cursor);
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    switch(cursKind)
	{
	case CXCursor_VarDecl:
	    {
	    CXType cursType = clang_getCursorType(cursor);
	    if(cursType.kind == CXType_Record)
		{
		SplitType st;
		CXStringDisposer name = clang_getCursorDisplayName(cursor);
		ModelType *type = createOrGetTypeRef(cursor, name.c_str(), st);
		if(type)
		    {
		    mOperation->addBodyVarDeclarator(name, type,
			    st.isConst, st.isRef);
		    }
		}
	    }
	    break;

	default:
	    break;
	}
    return CXChildVisit_Recurse;
    }

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
	buildTokenStringForCursor(cursor, *op);
	removeLastNonIdentChar(*op);
	return CXChildVisit_Break;
	}
    return CXChildVisit_Continue;
    }

// Takes a CXType_FunctionProto cursor, and finds conditional statements and
// calls to functions.
CXChildVisitResult CppParser::visitFunctionAddStatements(CXCursor cursor, CXCursor parent)
    {
    sCrashDiagnostics.saveMostRecentParseLocation("FS", cursor);
    static std::vector<std::string> sSwitchStrings;
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    switch(cursKind)
	{
	case CXCursor_SwitchStmt:
	    {
	    std::string tempStr;
	    clang_visitChildren(cursor, getConditionStr, &tempStr);
	    sSwitchStrings.push_back(tempStr);
	    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
	    sSwitchStrings.pop_back();
	    }
	    break;

//	case CXCursor_GotoStmt, break, continue, return, CXCursor_DefaultStmt
	/// Conditional statements
	case CXCursor_WhileStmt:
	case CXCursor_DoStmt:
	case CXCursor_ForStmt:
	case CXCursor_IfStmt:
	case CXCursor_CaseStmt:
	case CXCursor_CXXForRangeStmt:
	    {
	    std::string opStr;
	    clang_visitChildren(cursor, getConditionStr, &opStr);
	    std::string fullop;
	    if(cursKind == CXCursor_CaseStmt)
		{
		std::string tempStr;
		if(sSwitchStrings.size() > 0)
		    tempStr += sSwitchStrings[sSwitchStrings.size()-1];
		tempStr += " == ";
		tempStr += opStr;
		opStr = tempStr;
		}
	    else if(cursKind != CXCursor_IfStmt)
		{
		fullop += "*";
		}
	    fullop += "[";
	    fullop += opStr;
	    fullop += ']';

	    ModelCondStatements *savedCD = mCondStatements;
	    mCondStatements = new ModelCondStatements(fullop);
	    savedCD->addStatement(mCondStatements);
	    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
	    mCondStatements = savedCD;
	    }
	    break;

	case CXCursor_CallExpr:
	    {
	    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
	    CXStringDisposer functionName = clang_getCursorDisplayName(cursor);

	    // This returns the CXCursor_CXXMethod cursor
	    CXCursor cursorDef = clang_getCursorDefinition(cursor);
	    if(cursorDef.kind == CXCursor_FirstInvalid)
		cursorDef = clang_getCursorReferenced(cursor);
	    CXCursor classCursor = clang_getCursorSemanticParent(cursorDef);
	    CXStringDisposer className(clang_getCursorSpelling(classCursor));
	    if(functionName.length() != 0)
		{
		const ModelType *classType = mModelData.createOrGetTypeRef(className.c_str(), otClass);
		mCondStatements->addStatement(new ModelOperationCall(
			functionName, classType));
		}
	    }
	    break;

	case CXCursor_NullStmt:
	    break;

	case CXCursor_CompoundStmt:
	    {
	    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
	    std::string stmtStr;
	    buildTokenStringForCursor(cursor, stmtStr);
	    int len = stmtStr.length();
	    if(len >= 4)
		{
		if(stmtStr.find("else", stmtStr.length()-4) != std::string::npos)
		    {
		    /// @todo - There is no CXCursor_ElseStmt yet in clang.  The else is at the end of a
		    /// statement (compound or null) at the moment, then the next compound statement is
		    /// really under the else. So it is fairly difficult to parse here currently. The
		    /// compound statement code that is under the else now shows up under the if.

		    /// @todo - The next statement (compound/null) should really be under this else.
		    /// This is wrong in the XMI since it indicates an operation, but it looks
		    /// fairly good in the graphs.
		    if(mCondStatements)
			mCondStatements->addStatement(new ModelOperationCall("[else]", nullptr));
		    }
		}
	    }
	    break;

	default:
	    clang_visitChildren(cursor, ::visitFunctionAddStatements, this);
	    break;
	}
    return CXChildVisit_Continue;
    }

CXChildVisitResult CppParser::visitTranslationUnit(CXCursor cursor, CXCursor parent)
    {
    sCrashDiagnostics.saveMostRecentParseLocation("TU", cursor);
    CXType cursType = clang_getCursorType(cursor);
    switch(cursType.kind)
	{
	case CXType_Record:	// Class or struct
	    {
// Save all classes so that the access specifiers in the declared classes from other TU's are saved.
	    CXStringDisposer name(clang_getCursorSpelling(cursor));
	    mClassifier = createOrGetClassRef(name.c_str());
	    if(mClassifier)
		{
		int line;
		std::string fn = getFileLoc(cursor, &line);
		// Indicate to the modelwriter, that this class should be output with
		// this TU.
		if(fn == mTopParseFn)
		    {
		    mClassifier->setOutput(ModelClassifier::O_DefineClass);
		    }
		mClassifier->setModule(mModelData.mModules[0]);
		mClassifier->setLineNum(line);
		clang_visitChildren(cursor, ::visitRecord, this);
		}
	    mClassifier = nullptr;
	    }
	    break;

	case CXType_FunctionProto:	//  CXCursorKind = CXCursor_CXXMethod
	    {
	    mClassifier = nullptr;
	    CXCursor classCursor = clang_getCursorSemanticParent(cursor);
	    CXStringDisposer className(clang_getCursorSpelling(classCursor));
	    mClassifier = createOrGetClassRef(className.c_str());
	    if(mClassifier)
		{
		int line;
		std::string fn = getFileLoc(cursor, &line);
		// For CPP files, output the class definition also, so that the
		// function definitions get written out as part of the class. The
		// reader of the files will have to resolve duplicate .h/.cpp definitions.
		if(fn == mTopParseFn)
		    {
		    mClassifier->setOutput(ModelClassifier::O_DefineOperations);
		    }
		CXStringDisposer funcName(clang_getCursorSpelling(cursor));
		/// @todo - this doesn't work for overloaded functions.
		mOperation = mClassifier->getOperation(funcName, isMethodConst(cursor));
		if(mOperation)
		    {
		    addOperationParts(cursor, false);
		    mOperation->setModule(mModelData.mModules[0]);
		    mOperation->setLineNum(line);
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
		absErFn.getAbsolutePath(includerFn.c_str(), FP_File);
		FilePath absEdFn;
		absEdFn.getAbsolutePath(includedFn.c_str(), FP_File);

		// Find the base path minus the included path. This is for
		// includes like "include <gtk/gtk.h>"
		OovString lowerEd;
		OovString edName;
		lowerEd.setLowerCase(absEdFn.c_str());
		edName.setLowerCase(includedNameString.c_str());
		size_t pos = lowerEd.find(edName, absEdFn.length() -
			includedNameString.length());
		if(pos != std::string::npos)
		    {
		    absEdFn.insert(pos, 1, ';');
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

CppParser::eErrorTypes CppParser::parse(char const * const srcFn, char const * const srcRootDir,
	char const * const outDir,
	char const * const clang_args[], int num_clang_args)
    {
    eErrorTypes errType = ET_None;

    mTopParseFn = srcFn;
    /// Create a module so the modelwriter has a filename.
    ModelModule module;
    module.setModulePath(srcFn);
    mModelData.mModules.push_back(&module);
    mIncDirDeps.read(outDir, Project::getAnalysisIncDepsFilename());

    CXIndex index = clang_createIndex(1, 1);

// This doesn't appear to change anything.
//    clang_toggleCrashRecovery(true);
    // Get inclusion directives to be in AST.
    unsigned options = CXTranslationUnit_DetailedPreprocessingRecord;
//    unsigned options = 0;
    CXTranslationUnit tu = clang_parseTranslationUnit(index, srcFn,
	clang_args, num_clang_args, 0, 0, options);
    if(tu != nullptr)
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
	sLog.open("DebugCppParse.txt");
	fprintf(sLog.mFp, "DUMP TYPES\n");
	for(const auto & tp: mModelData.mTypes)
	    {
	    fprintf(sLog.mFp, "%s %p\n", tp->getName().c_str(), tp);
	    }
	fflush(sLog.mFp);
#endif

	std::string outBaseFileName;
	Project::makeOutBaseFileName(std::string(srcFn), std::string(srcRootDir),
	    outDir, outBaseFileName);

	try
	    {
	    std::string outXmiFileName = outBaseFileName;
	    outXmiFileName += ".xmi";
	    ModelWriter writer(mModelData);
	    writer.writeFile(outXmiFileName.c_str());
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
	errType = ET_NoSourceFile;
	}
    return errType;
    }
