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

// These typedefs should be used as rarely as possible. The inner parts of the
// modules should use the two string classes. The typedefs should mainly be
// used in interfaces (DLL's, file I/O, etc.)
typedef char tAsciiChar;
// There is no tIntlChar because this is not a fixed sized type.
typedef char tHugeIntlChar[6];	// This is large enough to hold any international character.
typedef char* tIntlStrPtr;
typedef const char* tIntlStrPtrConst;
typedef char tIntlByte;		// This is one byte of a UTF-8 character

typedef std::vector<std::string> StdStringVec;
class OovStringVec:public std::vector<class OovString>
    {
    public:
	OovString getStr(size_t index);
    };

bool StringToFloat(char const * const str, float min, float max, float &val);
bool StringToInt(char const * const str, int min, int max, int &val);
bool StringToUnsignedInt(char const * const str, unsigned int min,
	unsigned int max, unsigned int &val);
void StringToLower(std::string &str);
int StringNumChars(const char *s);
bool StringIsAscii(const char *p);
int StringCompareNoCase(char const * str1, char const * str2);
size_t StringFindSpace(char const *str, size_t startPos);
OovStringVec StringSplit(char const * str, char delimiter);
OovStringVec StringSplit(char const * str, char const *delimiterStr);
OovStringVec StringSplit(char const * str, std::vector<std::string> const &delimiters);
class OovString StringJoin(OovStringVec const &tokens, char delimiter);
std::string StringMakeXml(std::string const &text);

/// WARNING: the class that contains the c_str should be placed first in
/// the inheritance tree of the derived class because this class uses a
/// reinterpret_cast.
// This template requires T_Str to have a member "char const * const c_str()". This
// uses the CRTP pattern.
// This class is immutable. It does not do memory allocation or deallocation and
// does not keep any memory.
template<typename T_Str> class OovStringRefInterface
    {
    public:
	bool getFloat(float min, float max, float &val) const
	    { return StringToFloat(derivedThis()->c_str(), min, max, val); }
	bool getInt(int min, int max, int &val) const
	    { return StringToInt(derivedThis()->c_str(), min, max, val); }
	bool getUnsignedInt(unsigned int min, unsigned int max,
		unsigned int &val) const
	    { return StringToUnsignedInt(derivedThis()->c_str(), min, max, val); }
	size_t findSpace(size_t startPos=0) const
	    { return StringFindSpace(derivedThis()->c_str(), startPos);}
	int numChars() const
	    { return StringNumChars(derivedThis()->c_str()); }
	bool isAscii() const
	    { return StringIsAscii(derivedThis()->c_str()); }
	OovStringVec split(char delim) const
	    { return StringSplit(derivedThis()->c_str(), delim); }

    protected:
	/// WARNING: No data members should be added to this class because
	/// of the reinterpret_cast in case a derived class is
	/// inherited after this class.
	T_Str const *derivedThis() const
	    { return reinterpret_cast<T_Str const *>(this); }
    };

class OovStringRef:public OovStringRefInterface<OovStringRef>
    {
    public:
	OovStringRef(char const * const str):
	    mStr(str)
	    {}
	char const * const c_str() const
	    { return mStr; }

    private:
	char const * const mStr;
    };

class OovString:public std::string, public OovStringRefInterface<std::string>
    {
    public:
	OovString()
	    {}
	OovString(char const * const str):
	    std::string(str)
	    {}
	OovString(std::string const &str):
	    std::string(str)
	    {}
	OovString(std::string const &str, size_t pos, size_t len = npos):
	    std::string(str, pos, len)
	    {}
	OovString(char const * const str, size_t n):
	    std::string(str, n)
	    {}
	void setUpperCase(char const * const str);
	void setLowerCase(char const * const str);
	void appendInt(int val, int radix=10);
	void join(OovStringVec const &tokens, char delim)
	    { *this = StringJoin(tokens, delim); }
	void replaceStrs(char const *srchStr, char const *repStr);
//	char const * const c_str() const
//	    { return std::string::c_str(); }
    };

#endif /* OOVSTRING_H_ */
