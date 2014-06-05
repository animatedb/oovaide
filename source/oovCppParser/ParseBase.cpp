/*
 * ParseBase.cpp
 *
 *  Created on: Jul 23, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */


#include "ParseBase.h"
#include <algorithm>



bool isIdentC(char c)
    {
    return(isalnum(c) || c == '_');
    }

void removeLastNonIdentChar(std::string &name)
    {
    if(name.size() > 0)
	{
	int lastCharIndex = name.length()-1;
	if(!isIdentC(name[lastCharIndex]))
	    {
	    name.resize(lastCharIndex);		// Remove last character
	    }
	}
    }

void buildTokenStringForCursor(CXCursor cursor, std::string &str)
    {
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = 0;
    unsigned int nTokens = 0;
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    clang_tokenize(tu, range, &tokens, &nTokens);
    for (size_t i = 0; i < nTokens; i++)
	{
	CXTokenKind kind = clang_getTokenKind(tokens[i]);
	if(kind != CXToken_Comment)
	    {
	    if(str.length() != 0)
		str += " ";
	    CXStringDisposer spelling = clang_getTokenSpelling(tu, tokens[i]);
	    str += spelling;
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
    CXStringDisposer sp = clang_getTypeSpelling(clang_getCursorType(cursor));
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

struct ChildKindVisitor
{
    ChildKindVisitor(CXCursorKind cursKind):
	mCursKind(cursKind), mFound(false)
	{}
    CXCursorKind mCursKind;
    bool mFound;
};
static CXChildVisitResult IsChildKindVisitor(CXCursor cursor, CXCursor parent,
	CXClientData client_data)
    {
    ChildKindVisitor *context = static_cast<ChildKindVisitor*>(client_data);
    CXCursorKind cursKind = clang_getCursorKind(cursor);
    if(cursKind == context->mCursKind)
	context->mFound = true;
    return CXChildVisit_Recurse;
    }

bool childOfCursor(CXCursor cursor, CXCursorKind cursKind)
    {
    ChildKindVisitor visitorData(cursKind);
    clang_visitChildren(cursor, IsChildKindVisitor, &visitorData);
    return visitorData.mFound;
    }

std::string getFullBaseTypeName(CXCursor cursor)
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
    CXType cursorType = clang_getCursorType(cursor);
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
    CXStringDisposer spell = clang_getTypeSpelling(cursorType);
    return spell;
    }
