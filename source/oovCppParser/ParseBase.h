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

// This will do funny things if the str does not contain a type and identifier.
// For example, str=eParamCategories, name="mCategories" will become ePara.
void removeIdentName(char const * const name, std::string &str);
void removeClass(std::string &name);
void removeTypeRefEnd(std::string &name);
void removeLastNonIdentChar(std::string &name);

void buildTokenStringForCursor(CXCursor cursor, std::string &str);
void getMethodQualifiers(CXCursor cursor, std::vector<std::string> &qualifiers);
bool isMethodConst(CXCursor cursor);
bool isIdentC(char c);
std::string getFullSemanticTypeName(CXCursor cursor);

struct RefType
    {
    RefType():
	isConst(false), isRef(false)
	{}
    bool isConst;
    bool isRef;
    };

struct SplitType
    {
    public:
	void setSplitType(char const * const fullDecl, char const * const name);

	std::string baseType;
	bool isConst;
	bool isRef;
    };

void getSplitType(char const * const fullDecl, char const * const name, SplitType &st);

bool childOfCursor(CXCursor cursor, CXCursorKind cursKind);


#endif /* PARSEBASE_H_ */
