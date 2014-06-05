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

void removeLastNonIdentChar(std::string &name);

void buildTokenStringForCursor(CXCursor cursor, std::string &str);
void getMethodQualifiers(CXCursor cursor, std::vector<std::string> &qualifiers);
bool isMethodConst(CXCursor cursor);
bool isConstType(CXCursor cursor);
bool isIdentC(char c);
// Gets the type name without pointer/reference.
std::string getFullBaseTypeName(CXCursor cursor);

struct RefType
    {
    RefType():
	isConst(false), isRef(false)
	{}
    bool isConst;
    bool isRef;
    };

bool childOfCursor(CXCursor cursor, CXCursorKind cursKind);


#endif /* PARSEBASE_H_ */
