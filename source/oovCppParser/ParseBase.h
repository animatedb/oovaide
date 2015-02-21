/*
 * ParseBase.h
 *
 *  Created on: Jul 23, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef PARSEBASE_H_
#define PARSEBASE_H_

#include <string>
#include <vector>
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

struct RefType
    {
    RefType():
	isConst(false), isRef(false)
	{}
    bool isConst;
    bool isRef;
    };

CXCursor getNthChildCursor(CXCursor cursor, int nth);
CXCursor getCursorChildKind(CXCursor cursor, CXCursorKind cursKind);
CXCursor getCursorChildKindAndTypeSpelling(CXCursor cursor, CXCursorKind cursKind,
	const char *type);
CXCursor getCursorChildKindAndNotTypeSpelling(CXCursor cursor, CXCursorKind cursKind,
	const char *type);
CXCursor getCursorCallMemberRef(CXCursor cursor);
void debugDumpCursor(CXCursor cursor, bool recurse=true);
void removeLastNonIdentChar(std::string &name);
void appendConditionString(CXCursor cursor, std::string &str);
// endCondition=true is the same as appendConditionString().
void appendCursorTokenString(CXCursor cursor, std::string &str, bool endCondition=false);
void getMethodQualifiers(CXCursor cursor, std::vector<std::string> &qualifiers);
bool isMethodConst(CXCursor cursor);
bool isConstType(CXCursor cursor);
bool isConstType(CXType cursType);
bool isIdentC(char c);
// Gets the type name without pointer/reference.
std::string getFullBaseTypeName(CXCursor cursor);


#endif /* PARSEBASE_H_ */
