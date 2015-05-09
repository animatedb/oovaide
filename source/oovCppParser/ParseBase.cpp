/*
 * ParseBase.cpp
 *
 *  Created on: Jul 23, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */


#include "ParseBase.h"
#include <algorithm>
#include "Debug.h"



#define DEBUG_CHILDREN 1
#if(DEBUG_CHILDREN)
static CXChildVisitResult debugDumpCursorVisitor(CXCursor cursor,
    CXCursor /*parent*/, void *clientData)
    {
    static int depth = 0;
    depth++;
    CXStringDisposer dispName = clang_getCursorDisplayName(cursor);
    std::string tokStr;
    appendCursorTokenString(cursor, tokStr);
    CXType type = clang_getCursorType(cursor);
    CXStringDisposer typeSp = clang_getTypeSpelling(type);
    CXStringDisposer kindSp = clang_getCursorKindSpelling(clang_getCursorKind(cursor));
    size_t maxLen = 80;
    if(tokStr.length() > maxLen)
	{
	tokStr.resize(maxLen);
	tokStr += "...";
	}
    FILE *fp = static_cast<FILE *>(clientData);
    if(depth == 1)
	{
	fprintf(fp, "\n");
	}
    else if(depth > 1)
	{
	fprintf(fp, "  ");
	}
    fprintf(fp, "%d@%s@%s@%s@%s@\n", depth, kindSp.c_str(), typeSp.c_str(),
	dispName.c_str(), tokStr.c_str());
    fflush(fp);

    clang_visitChildren(cursor, ::debugDumpCursorVisitor, clientData);
    depth--;
    return CXChildVisit_Continue;
    }
    void debugDumpCursor(FILE *fp, CXCursor cursor)
        {
        // clang_getCursorLexicalParent (CXCursor cursor)
        debugDumpCursorVisitor(cursor, cursor, fp);
        }
#endif


enum eMatchTypes { MT_None=0, MT_Kind=0x01, MT_TypeSpelling=0x02, MT_NotTypeSpelling=0x04 };
struct ChildKindVisitor
    {
    ChildKindVisitor(eMatchTypes mt):
	mMatchType(mt), mCursKind(CXCursor_UnexposedDecl),
	mFoundCursor(clang_getNullCursor())
	{}
    void setKind(CXCursorKind kind)
	{ mCursKind = kind; }
//    void setName(char const *name)
//	{ mCursName = name; }
    void setTypeSpelling(char const *typeSp)
	{ mCursTypeSpelling = typeSp; }
    eMatchTypes mMatchType;
    CXCursorKind mCursKind;
//    std::string mCursName;
    std::string mCursTypeSpelling;
    CXCursor mFoundCursor;
    };

static CXChildVisitResult GetChildKindVisitor(CXCursor cursor,
    CXCursor /*parent*/, CXClientData client_data)
    {
    ChildKindVisitor *context = static_cast<ChildKindVisitor*>(client_data);
    bool match = true;
    if(context->mMatchType & MT_Kind)
	{
	CXCursorKind kind = clang_getCursorKind(cursor);
	match = (kind == context->mCursKind);
	}
    if((context->mMatchType & MT_TypeSpelling) && match)
	{
	CXStringDisposer typeSp = clang_getTypeSpelling(clang_getCursorType(cursor));
	match = (typeSp == context->mCursTypeSpelling);
	}
    if((context->mMatchType & MT_NotTypeSpelling) && match)
	{
	CXStringDisposer typeSp = clang_getTypeSpelling(clang_getCursorType(cursor));
	match = (typeSp != context->mCursTypeSpelling);
	}
    CXChildVisitResult visitRes = CXChildVisit_Recurse;
    if(match)
	{
	context->mFoundCursor = cursor;
	visitRes = CXChildVisit_Break;
	}
    return visitRes;
    }

CXCursor getCursorChildKind(CXCursor cursor, CXCursorKind cursKind)
    {
    ChildKindVisitor visitorData(MT_Kind);
    visitorData.setKind(cursKind);
    clang_visitChildren(cursor, GetChildKindVisitor, &visitorData);
    return visitorData.mFoundCursor;
    }

CXCursor getCursorChildKindAndTypeSpelling(CXCursor cursor, CXCursorKind cursKind,
	const char *typeSp)
    {
    ChildKindVisitor visitorData(static_cast<eMatchTypes>(MT_Kind | MT_TypeSpelling));
    visitorData.setKind(cursKind);
    visitorData.setTypeSpelling(typeSp);
    clang_visitChildren(cursor, GetChildKindVisitor, &visitorData);
    return visitorData.mFoundCursor;
    }

CXCursor getCursorChildKindAndNotTypeSpelling(CXCursor cursor, CXCursorKind cursKind,
	const char *typeSp)
    {
    ChildKindVisitor visitorData(static_cast<eMatchTypes>(MT_Kind | MT_NotTypeSpelling));
    visitorData.setKind(cursKind);
    visitorData.setTypeSpelling(typeSp);
    clang_visitChildren(cursor, GetChildKindVisitor, &visitorData);
    return visitorData.mFoundCursor;
    }


struct ChildCountVisitor
    {
    ChildCountVisitor(int count):
	mCount(count), mFoundCursor(clang_getNullCursor())
	{}
    int mCount;
    CXCursor mFoundCursor;
    };

static CXChildVisitResult ChildNthVisitor(CXCursor cursor, CXCursor /*parent*/,
    CXClientData client_data)
    {
    ChildCountVisitor *context = static_cast<ChildCountVisitor*>(client_data);
    CXChildVisitResult res = CXChildVisit_Continue;
    if(context->mCount == 0)
	{
	context->mFoundCursor = cursor;
	res = CXChildVisit_Break;
	}
    else
	{
	context->mCount--;
	}
    return res;
    }

CXCursor getNthChildCursor(CXCursor cursor, int nth)
    {
    ChildCountVisitor visitorData(nth);
    clang_visitChildren(cursor, ChildNthVisitor, &visitorData);
    CXCursor childCursor = clang_getNullCursor();
    if(visitorData.mCount == 0)
	{
	childCursor = visitorData.mFoundCursor;
	}
    return childCursor;
    }

bool isIdentC(char c)
    {
    return(isalnum(c) || c == '_');
    }

void removeLastNonIdentChar(std::string &name)
    {
    if(name.size() > 0)
	{
	size_t lastCharIndex = name.length()-1;
	if(!isIdentC(name[lastCharIndex]))
	    {
	    name.resize(lastCharIndex);		// Remove last character
	    }
	}
    }

void appendConditionString(CXCursor cursor, std::string &str)
    {
    appendCursorTokenString(cursor, str, true);
    }

// clang_tokenize will return some arbitrary stuff sometimes, and will create
// multi megabyte output files. This was first noticed on the
// LLVM lib/IR/Verifier.cpp file when run with MinGW-W64 with some math
// include errors using clang 3.5.1.
class ConditionTerminator
    {
    public:
	ConditionTerminator():
	    mNestParenCount(0)
	    {}
	// This should only be called for CXToken_Punctuation tokens so that it
	// does not match parenthesis inside of literals.
	bool endConditionDetected(std::string &str)
	    {
	    bool endDetected = false;
	    if(str[0] == '(')
		{
		mNestParenCount++;
		}
	    else if(str[0] == ')')
		{
		if(mNestParenCount == 0)
		    {
		    endDetected = true;
		    }
		mNestParenCount--;
		}
	    else if(str[0] == '#')
		{
		endDetected = true;
		}
	    return endDetected;
	    }

    private:
	int mNestParenCount;
    };

void appendCursorTokenString(CXCursor cursor, std::string &str, bool endCondition)
    {
    ConditionTerminator condTerm;
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = 0;
    unsigned int nTokens = 0;
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    // clang_tokenize is a strange function. Sometimes it returns a last
    // token that is part of the cursor, and sometimes it returns a last token
    // that is part of the next cursor.
    clang_tokenize(tu, range, &tokens, &nTokens);
    if(nTokens > 0)
	{
	for (size_t i = 0; i < nTokens; i++)
	    {
	    CXTokenKind kind = clang_getTokenKind(tokens[i]);
	    if(kind != CXToken_Comment)
		{
		if(str.length() != 0)
		    str += " ";
		CXStringDisposer spelling = clang_getTokenSpelling(tu, tokens[i]);
		if(kind == CXToken_Punctuation && endCondition)
		    {
		    if(condTerm.endConditionDetected(spelling))
			{
			break;
			}
		    }
		str += spelling;
		}
	    }
	}
    clang_disposeTokens(tu, tokens, nTokens);
    }

void getMethodQualifiers(CXCursor cursor, std::vector<std::string> &qualifiers)
    {
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    qualifiers.clear();

    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken* tokens;
    unsigned int numTokens;
    clang_tokenize(tu, range, &tokens, &numTokens);

    bool insideBrackets = false;
    for (unsigned int i = 0; i < numTokens; i++)
	{
	CXStringDisposer token = clang_getTokenSpelling(tu, tokens[i]);
	if (token == "(")
	    {
	    insideBrackets = true;
	    }
	else if (token == "{" || token == ";")
	    {
	    break;
	    }
	else if (token == ")")
	    {
	    insideBrackets = false;
	    }
	else if (clang_getTokenKind(tokens[i]) == CXToken_Keyword &&
	     !insideBrackets)
	    {
	    qualifiers.push_back(token);
	    }
	}
    }

bool isMethodConst(CXCursor cursor)
    {
    std::vector<std::string> quals;
    getMethodQualifiers(cursor, quals);
    return(std::find(quals.begin(), quals.end(), "const") != quals.end());
    }

// clang_isConstQualifiedType returns false for a type that has a type spelling like "const type &"
// This function returns const for any type with const left of & or *.
bool isConstType(CXCursor cursor)
    {
    return isConstType(clang_getCursorType(cursor));
    }

bool isConstType(CXType cursType)
    {
    CXStringDisposer sp = clang_getTypeSpelling(cursType);
    bool isConst = false;

    size_t constPos = sp.find("const");
    if(constPos != std::string::npos)
	{
	size_t pointPos = sp.find('*');
	if(pointPos == std::string::npos)
	    pointPos = sp.find('&');
	if(pointPos != std::string::npos)
	    {
	    if(constPos < pointPos)
		{
		isConst = true;
		}
	    }
	}
    return isConst;
    }

std::string getFullBaseTypeName(CXCursor cursor)
    {
    CXType cursorType = clang_getCursorType(cursor);
    return getFullBaseTypeName(cursorType);
    }

size_t getFunctionNameFromMemberRefExpr(std::string &expr)
    {
    size_t pos = expr.rfind('.');
    if(pos == std::string::npos)
	{
	pos = expr.rfind('>');		// Find end of ->
	}
    if(pos == std::string::npos)
	{
	pos = expr.rfind(':');		// Find end of ::
	}
    if(pos != std::string::npos)
	{
	pos++;
	}
    return pos;
    }

std::string getFullBaseTypeName(CXType cursorType)
    {
/*    std::string fullName;
    while(1)
	{
	CXStringDisposer name = clang_getTypeSpelling(clang_getCursorType(cursor));
	if(fullName.length() == 0 && name.length() == 0)
	    {
	    // getTypeSpelling for class templates returns "".
//	    name = clang_getCursorSpelling(cursor);
	    // clang_getCursorDisplayName shows Displayer<T>, but cursorSpelling
	    // only displays Displayer.
	    name = clang_getCursorDisplayName(cursor);
	    }
	if(name.length() > 0)
	    {
	    if(fullName.length() == 0)
		fullName = name;
	    else
		{
		fullName = name + "::" + fullName;
		}
	    }
	cursor = clang_getCursorSemanticParent(cursor);
	if(clang_Cursor_isNull(cursor) || cursor.kind !=  CXCursor_Namespace)
	    {
	    break;
	    }
	}
    return fullName;
*/
    CXStringDisposer spell = clang_getTypeSpelling(getBaseType(cursorType));
    if(cursorType.kind == CXType_Typedef)
	{
	spell.insert(0, "<<typedef>> ");
	}
    return spell;
    }

CXType getBaseType(CXType cursorType)
    {
    while(cursorType.kind == CXType_LValueReference ||
	    cursorType.kind == CXType_RValueReference || cursorType.kind == CXType_Pointer)
	{
	// clang_getPointeeType does the following conversions:
	// char *			char
	// const Teaching::Star &	const TeachingStar::Star
	// Imaginary::PretendStar **	Imaginary::PretendStar *
	cursorType = clang_getPointeeType(cursorType);
	}
    if(cursorType.kind == CXType_Unexposed)
	{
	CXCursor newCursor = clang_getTypeDeclaration(cursorType);
	if(newCursor.kind != CXCursor_NoDeclFound)
	    {
	    cursorType = clang_getCursorType(newCursor);
	    }
	}
    return cursorType;
    }
