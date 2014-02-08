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
std::vector<std::string> split(const std::string &str, char delimiter);

bool StringToFloat(char const * const str, float min, float max, float &val);
bool StringToInt(char const * const str, int min, int max, int &val);
bool StringToUnsignedInt(char const * const str, unsigned int min,
	unsigned int max, unsigned int &val);

// This class is immutable. It does not do memory allocation or deallocation and
// does not keep any memory.
class OovStringRefInterface
    {
    public:
	virtual ~OovStringRefInterface()
	    {}
	virtual char const * const c_str() const = 0;
	bool getFloat(float min, float max, float &val) const
	    { return StringToFloat(c_str(), min, max, val); }
	bool getInt(int min, int max, int &val) const
	    { return StringToInt(c_str(), min, max, val); }
	bool getUnsignedInt(unsigned int min, unsigned int max,
		unsigned int &val) const
	    { return StringToUnsignedInt(c_str(), min, max, val); }
	size_t findSpace(size_t startPos) const;
	int numChars() const;
	bool isAscii() const;
    };

class OovString:public OovStringRefInterface, public std::string
    {
    public:
	OovString()
	    {}
	OovString(char const * const str):
	    std::string(str)
	    {}
	void setUpperCase(char const * const str);
	void setLowerCase(char const * const str);
	void setInt(int val, int radix=10);
	virtual char const * const c_str() const
	    { return std::string::c_str(); }
    };

#endif /* OOVSTRING_H_ */
