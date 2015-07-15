/*
 * DebugResult.h
 *
 *  Created on: Mar 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef DEBUGRESULT_H_
#define DEBUGRESULT_H_

#include <deque>
#include <memory>
#include "OovString.h"

// There are many GDB/MI output syntax BNF docs online, but they all
// appear to be incorrect or incomplete. This is close:
// https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Output-Syntax.html
//
// Rough GDB output is:
//  In the BNF, parens are used to show grouping, * is used for zero or more.
//  a const is: 0x5a15a0 or "this\""
//  a tuple is: "{" ( result (",")* )* "} ,*"
//  a list is: "[" ( (value | result) (",")* )* "] ,*"
//
// These are simplified for our needs.
//  A tuple is a comma separated list of results
//  A list is a comma separated list of values or results
//  A result is a name and value(s) separated with equal sign
//  A value is a result without a name
class cDebugResult
    {
    public:
        void setVarName(char const *str, size_t len)
            { mVarName.assign(str, len); }
        char const *parseResult(OovStringRef const resultStr);
        std::string getAsString(int level=0) const;

    private:
        std::string mVarName;
        // A result will contain either child results or a const.
        // If child results is empty, then it contains a const.
        std::string mValue;
        // Using deque because it allows const reading.
        std::deque<std::unique_ptr<class cDebugResult>> mChildResults;
        char const *parseVarName(char const *resultStr);
        char const *parseValue(char const *resultStr);
        char const *parseList(char const *resultStr);
        char const *parseConst(char const *resultStr);
        cDebugResult &addResult()
            {
            cDebugResult *newRes = new cDebugResult();
           /// @todo - use make_unique when supported.
            mChildResults.push_back(std::unique_ptr<cDebugResult>(newRes));
            return *newRes;
            }
    };


#endif /* DEBUGRESULT_H_ */
