/*
 * DebugResult.cpp
 *
 *  Created on: Mar 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "DebugResult.h"
#include "Debug.h"

// A new parsing method should be used, but it seems like a
// waste of time to do this since other debuggers are using
// specialized  python pretty printing to format the output
// for std::string.  It may be more worthwhile to investigate LLDB.
#include <string.h>
#define DBG_RESULT 0
#if(DBG_RESULT)
//static DebugFile sDbgFile("DbgResult.txt", false);
static DebugFile sDbgFile(stdout);
void debugStr(char const *title, char const *str)
    { sDbgFile.printflush("%s %s\n", title, str); }
#else
void debugStr(char const * /*title*/, char const * /*str*/)
    {}
#endif


static char const *skipSpace(char const *p)
    {
    while(isspace(*p))
        p++;
    return p;
    }

static bool isStartListC(char c)
    { return(c == '[' || c == '{'); }

static bool isEndListC(char c)
    { return(c == ']' || c == '}'); }

static bool isListItemSepC(char c)
    { return(c == ','); }

static bool isStringC(char c)
    { return(c == '\"'); }

// A name can be:
//      <std::allocator<char>>
static bool isVarnameC(char c)
    { return(isalpha(c) || c == '<'); }


DebugResult::DebugResult(DebugResult &&src)
    {
    if(this != &src)
        {
        clear();
        mVarName = src.mVarName;
        mValue = src.mValue;
        mChildResults = std::move(src.mChildResults);
        }
    }

DebugResult &DebugResult::addResult()
    {
    DebugResult *newRes = new DebugResult();
   /// @todo - use make_unique when supported.
    mChildResults.push_back(std::unique_ptr<DebugResult>(newRes));
    return *newRes;
    }

void DebugResult::clear()
    {
    mVarName.clear();
    mValue.clear();
    mChildResults.clear();
    }

std::string DebugResult::getAsString(int level) const
    {
    std::string str;

#if(DBG_RESULT)
    std::string title(level*2, ' ');
    title += "getAsString";
    std::string res = "Var: " + mVarName;
    res += "    Val:" + mValue;
    debugStr(title.c_str(), res.c_str());
#endif
    std::string leadSpace(static_cast<size_t>(level*2), ' ');
    str = leadSpace;
    if(mVarName.length())
        {
        str = mVarName;
        str += " = ";
        for(size_t i=0; i<mChildResults.size(); i++)
            {
            if(i != 0)
                {
                str += leadSpace;
                }
            str += mChildResults[i]->getAsString(level+1);
            }
        }
    str += mValue;
    str += "\n";
    return str;
    }

// Strings start and end with a quote.
// The following characters are prefixed with a backslash when they are in
// quotes:
//      backslash, doublequote
static char const *parseString(char const *valStr, std::string &translatedStr)
    {
#if(DBG_RESULT)
    debugStr("parseString", valStr);
    int transPos = translatedStr.length();
#endif
    char const *p = valStr;
    bool quotedValue = false;
    if(*p == '\\' && *(p+1) == '\"')
        {
        quotedValue = true;
        p++;
        translatedStr += *p++;
        }
    bool prevBackslash = false;
    while(*p)
        {
        if(*p == '\\')
            {
            p++;
            // Handle a backslash or quote.
            if(prevBackslash)
                { prevBackslash = false; }
            else if(*p == '\"')
                { break; }
            else
                { prevBackslash = (*p == '\\'); }
            translatedStr += *p++;
            }
        else
            {
            prevBackslash = false;
            translatedStr += *p++;
            }
        }
    if(quotedValue && *p == '\\' && *p == '\"')
        {
        p+=2;
        }
#if(DBG_RESULT)
    debugStr("  parseString", translatedStr.substr(transPos).c_str());
#endif
    return p;
    }

char const *DebugResult::parseVarName(char const *resultStr)
    {
#if(DBG_RESULT)
debugStr("parseVarName-start", resultStr);
#endif
    char const *start = skipSpace(resultStr);
    std::string translatedStr;
    OovString name;
    char const *p = start;
    while(*p && *p != ' ' && !isEndListC(*p) && *p != '=')
        {
        name += *p++;
        }
    mVarName = name;
    while(*p == ' ')
        {
        ++p;
        }
    if(*p == '=')
        {
        ++p;
        }
#if(DBG_RESULT)
debugStr("   parseVarName", mVarName.c_str());
#endif
    return p;
    }

// This parses a single value within a compound list value.
// Parses up to a list separator
char const *DebugResult::parseValue(char const *resultStr)
    {
#if(DBG_RESULT)
debugStr("parseValue-start", resultStr);
#endif
    char const *p = skipSpace(resultStr);
    // Values start with a quote and end with a quote even if they are not
    // C++ strings. C++ strings start with a backslash quote.
    bool quotedValue = false;
    if(isStringC(*p))   // All values should start with a quote.
        {
        quotedValue = true;
        ++p;
        }
    // According to GDB MI documentation, a value can be a const, tuple, or list.
    // Tuples and lists start with a special character, and are the start of values separated by commas.
    // const values can be complex like: 0x40d139 <_ZStL19piecewise_construct+41> \"{curly}\"
    if(isStartListC(*p))
        {
        DebugResult &res = addResult();
        p = res.parseResult(++p);
        while(isListItemSepC(*p))
            {
            DebugResult &res = addResult();
            p = res.parseResult(++p);
            }
        if(isEndListC(*p))
            {
            p++;
            }
        else
            {
            OovString str;
            str.appendInt(*p);
            LogAssertFile(__FILE__, __LINE__, str.getStr());
            }
        }
    else
        {
        // Search for the end of the value by looking for the end quote.
        // C++ quoted strings can be many locations within the value.
        while(*p && !isStringC(*p) && !isListItemSepC(*p) && !isEndListC(*p))
            {
            if(*p == '\\' && isStringC(*(p+1)))
                {
                // In the GDB/MI interface, the value sometimes starts with a quote, and
                // ends with a quote and then \r\n.
                p = parseString(p, mValue);
                }
            mValue += *p++;
            }
        }
    if(quotedValue && isStringC(*p))
        {
        p++;
        }
#if(DBG_RESULT)
debugStr("   parseValue", mValue.c_str());
#endif
    return p;
    }

// "value=" has already been parsed.
// Ex:  value="{mVarName = {
//      static npos = <optimized out>,
//      _M_dataplus =   {
//      <std::allocator<char>> = {<__gnu_cxx::new_allocator<char>> =
//      {<No data fields>},
//      <No data fields>},
//      _M_p = 0x6fcc3fa4 <libstdc++-6!_ZNSs4_Rep20_S_empty_rep_storageE+12> \"\"}}, "
//      "mConst = {
//      static npos = <optimized out>,
//      _M_dataplus ="" {
//      <std::allocator<char>> = {
//      <__gnu_cxx::new_allocator<char>> ={<No data fields>},
//      <No data fields>},
//      _M_p = 0x6fcc3fa4 <libstdc++-6!_ZNSs4_Rep20_S_empty_rep_storageE+12> \"\"}},
//      mChildResults = {c = {<std::_Deque_base<DebugResult*,
//      std::allocator<DebugResult*> >> = {_M_impl = {<std::allocator<DebugResult*>>
//      = {<__gnu_cxx::new_allocator<DebugResult*>> = {<No data fields>},
//      <No data fields>}, _M_map = 0x6a11e0, _M_map_size = 8, _M_start =
//      {_M_cur = 0x6a1218, _M_first = 0x6a1218, _M_last = 0x6a1418, _M_node
//      = 0x6a11ec}, _M_finish = {_M_cur = 0x6a1218, _M_first = 0x6a1218,
//      _M_last = 0x6a1418, _M_node = 0x6a11ec}}}, <No data fields>}}}"};
//
//     char const * const square = "{curly}";
//              value="0x40d1af <_ZStL19piecewise_construct+159> \"{curly}\""
//
//              value="0x40d1b7 <_ZStL19piecewise_construct+167> \"[square]\""
//              value="0x40d1bf <_ZStL19piecewise_construct+175> \"\\\"quotes\\\"\""
//
//      value="{curly = 0x40d1af <_ZStL19piecewise_construct+159> \"{curly}\",
//          square = 0x40d1b7 <_ZStL19piecewise_construct+167> \"[square]\",
//          quotes = 0x40d1c0 <_ZStL19piecewise_construct+176> \"\\\"quotes\\\"\"}"

// General:
//  There is a varname followed by an equal sign.
//  A varname can have less than, greater than chars in the name, but is
//  terminated by space.
//  After an equal sign, there can be a quote, open curly brace, or
char const *DebugResult::parseResult(OovStringRef const resultStr)
    {
#if(DBG_RESULT)
debugStr("parseResult", resultStr.getStr());
#endif
    clear();
    char const *start = skipSpace(resultStr);
    char const *p = start;
    if(*start)
        {
        if(isVarnameC(*p))
            {
            p = parseVarName(p);
            }
        if(mVarName != "<No data fields>")
            {
            p = parseValue(p);
            }
        }
    return p;
    }
