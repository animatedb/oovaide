/*
 * ParseBase.cpp
 *
 *  Created on: Jul 23, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */


#include "ParseBase.h"
#include <algorithm>


CXStringDisposer::CXStringDisposer(const CXString &xstr)
    {
    append(clang_getCString(xstr));
    clang_disposeString(xstr);
    }


void removeIdentName(char const * const name, std::string &str)
    {
    size_t pos = str.rfind(name);
    if(pos != std::string::npos)
	{
	str.erase(pos, std::string(str).length());
	}
    }

void removeClass(std::string &name)
    {
    size_t pos = name.find("class ");
    if(pos != std::string::npos)
	name.erase(pos, 5);
    }

void removeTypeRefEnd(std::string &name)
    {
    size_t pos = name.find_first_of(",;)=");
    if(pos != std::string::npos)
	name.resize(pos);
    }

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

void SplitType::setSplitType(char const * const fullDecl, char const * const name)
    {
    isConst = false;
    isRef = false;
    std::string stdType = fullDecl;
    if(stdType.find("const") != std::string::npos)
	isConst = true;
    if((stdType.find('*') != std::string::npos) || (stdType.find('&') != std::string::npos))
	isRef = true;
    removeIdentName(name, stdType);
    size_t ei = stdType.find_last_not_of(" \t\n");
    if(ei != std::string::npos)
	stdType.resize(ei+1);
    baseType = stdType;
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

