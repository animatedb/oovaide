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
//static DebugFile sDbgFile("DbgResult.txt");
static DebugFile sDbgFile(stdout);
void debugStr(char const *title, char const *str)
    { sDbgFile.printflush("%s @%s@\n", title, str); }
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

std::string cDebugResult::getAsString(int level) const
    {
    std::string str;

    std::string leadSpace(static_cast<size_t>(level*3), ' ');
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
            str += "\n";
            }
        }
    str += mValue;
#if(DBG_RESULT)
    debugStr("getAsString", str.c_str());
#endif
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
    if(*p == '\\' && *p == '\"')
        {
        quotedValue = true;
        p+=2;
        }
    while(*p && *p != '\"')
        {
        if(*p == '\\')
            {
            p++;
            // Handle a backslash or quote.
            translatedStr += *p++;
            }
        else
            {
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

char const *cDebugResult::parseVarName(char const *resultStr)
    {
#if(DBG_RESULT)
debugStr("parseVarName", resultStr);
#endif
    char const *start = skipSpace(resultStr);
    std::string translatedStr;
    char const *p = strchr(start, '=');
    if(*p == '=')
        {
        start = skipSpace(start);
        int len = p-start;
        if(*(p-1) == ' ')
            {
            len--;
            }
        setVarName(start, static_cast<size_t>(len));
#if(DBG_RESULT)
debugStr("   parseVarName", std::string(start, len).c_str());
#endif
        p++;
        }
    else
        {
        p = start;
        }
    return p;
    }

// Parses up to a list separator
char const *cDebugResult::parseValue(char const *resultStr)
    {
#if(DBG_RESULT)
debugStr("parseValue", resultStr);
#endif
    char const *p = skipSpace(resultStr);
    // Some values are quoted in strings, but are not C++ strings.
    // C++ strings start with a backslash quote.
    bool quotedValue = false;
    if(isStringC(*p))
        {
        quotedValue = true;
        ++p;
        }
    while(*p)
        {
        if(isStartListC(*p))
            {
            cDebugResult &res = addResult();
            p = res.parseResult(++p);
            }
        else if(isEndListC(*p) || isListItemSepC(*p))
            {
            p++;
            break;
            }
        else if(*p == '\\' && isStringC(*(p+1)))
            {
            // In the GDB/MI interface, the value sometimes starts with a quote, and
            // ends with a quote and then \r\n.
            p = parseString(p, mValue);
            }
        else if(quotedValue && isStringC(*p))
            {
            quotedValue = false;
            p++;
            }
        else
            {
            mValue += *p++;
            }
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
//      mChildResults = {c = {<std::_Deque_base<cDebugResult*,
//      std::allocator<cDebugResult*> >> = {_M_impl = {<std::allocator<cDebugResult*>>
//      = {<__gnu_cxx::new_allocator<cDebugResult*>> = {<No data fields>},
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
char const *cDebugResult::parseResult(OovStringRef const resultStr)
    {
#if(DBG_RESULT)
debugStr("parseResult", resultStr.getStr());
#endif
    char const *start = skipSpace(resultStr);
    char const *p = start;
    if(*start)
        {
        if(isVarnameC(*p))
            {
            p = parseVarName(p);
            }
        p = parseValue(p);
        }
    return p;
    }
