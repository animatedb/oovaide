/*
 * OovString.h
 *
 *  Created on: Oct 6, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

// This module works on UTF-8 strings.

#ifndef OOVSTRING_H_
#define OOVSTRING_H_

#include <string>
#include <vector>
#include <set>

// Immutable functions
bool StringToFloat(char const * const str, float min, float max, float &val);
bool StringToInt(char const * const str, int min, int max, int &val);
bool StringToUnsignedInt(char const * const str, unsigned int min,
	unsigned int max, unsigned int &val);
size_t StringNumChars(char const * const str);
bool StringIsAscii(char const * const str);
int StringCompareNoCase(char const * const str1, char const * const str2);
size_t StringFindSpace(char const * const str, size_t startPos);

class OovStringVec:public std::vector<class OovString>
    {
    public:
	using vector::vector;
	OovString getStr(size_t index);
    };

class OovStringSet:public std::set<class OovString>
    {
    };

OovStringVec StringSplit(char const * const str, char delimiter);
OovStringVec StringSplit(char const * const str, char const * const delimiterStr);
OovStringVec StringSplit(char const * const str, OovStringVec const &delimiters);
void StringToLower(class OovString &str);
class OovString StringJoin(OovStringVec const &tokens, char delimiter);
class OovString StringMakeXml(char const * const str);

// This template requires T_Str to have a member "char const * const getStr()".
// getStr() is used instead of c_str so that references to c_str() can be found and removed.
// This uses CRTP (Curiously recurring template pattern).
// This class is immutable. It does not do memory allocation or deallocation and
// does not keep any memory.
template<typename T_Str> class OovStringRefInterface
    {
    public:
	bool getFloat(float min, float max, float &val) const
	    { return StringToFloat(getThisStr(), min, max, val); }
	bool getInt(int min, int max, int &val) const
	    { return StringToInt(getThisStr(), min, max, val); }
	bool getUnsignedInt(unsigned int min, unsigned int max,
		unsigned int &val) const
	    { return StringToUnsignedInt(getThisStr(), min, max, val); }
	size_t findSpace(size_t startPos=0) const
	    { return StringFindSpace(getThisStr(), startPos);}
	size_t numChars() const
	    { return StringNumChars(getThisStr()); }
	bool isAscii() const
	    { return StringIsAscii(getThisStr()); }
	OovStringVec split(char delim) const
	    { return StringSplit(getThisStr(), delim); }
	OovStringVec split(char const * const delimiterStr) const
	    { return StringSplit(getThisStr(), delimiterStr); }
	OovStringVec split(OovStringVec const &delimiters) const
	    { return StringSplit(getThisStr(), delimiters); }

    protected:
	char const * const getThisStr() const
	    { return static_cast<T_Str const *>(this)->getStr(); }
    };


// This is a constant string reference that does no memory allocation.
// This will not modify the contents of the source string.
//
// The purpose of this class is to replace "const char *" or
// "const std::string &"
//
// The typical usage of this is "OovStringRef const var" and not
// "OovStringRef const &var".  This allows automatic conversion from a
// literal string pointer.
class OovStringRef:public OovStringRefInterface<OovStringRef>
    {
    public:
	OovStringRef(OovStringRef const &str):
	    mStr(str.mStr)
	    {}
	OovStringRef(char const * const str):
	    mStr(str)
	    {}
	OovStringRef(std::string const &str):
	    mStr(str.c_str())
	    {}
	char operator[] (int index) const
	    { return mStr[index]; }
	char const * const getStr() const
	    { return mStr; }
	operator char const * const() const
	    { return mStr; }

    private:
	char const * const mStr;
    };

class OovString:public OovStringRefInterface<OovString>, public std::string
    {
    public:
	OovString()
	    {}
	OovString(char const * const str):
	    std::string(str)
	    {}
	OovString(OovStringRef const &str):
	    std::string(str.getStr())
	    {}
	OovString(std::string const &str):
	    std::string(str.c_str())
	    {}
	OovString(std::string const &str, size_t pos, size_t len = npos):
	    std::string(str, pos, len)
	    {}
	OovString(char const * const str, size_t n):
	    std::string(str, n)
	    {}
	void setUpperCase(OovStringRef const str);
	void setLowerCase(OovStringRef const str);
	void appendInt(int val, int radix=10);
	void join(OovStringVec const &tokens, char delim)
	    { *this = StringJoin(tokens, delim); }
	OovString makeXml() const
	    { return StringMakeXml(getStr()); }
	bool replaceStrs(OovStringRef const srchStr, OovStringRef const repStr);
	char const * const getStr() const
	    { return std::string::c_str(); }
	std::string &getStdString()
	    { return *this; }
    };

#endif /* OOVSTRING_H_ */
