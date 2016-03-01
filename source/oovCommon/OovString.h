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

/// Convert a string to a float. The string can only contain characters that
/// are valid for a floating point number, otherwise this returns false.
/// @param str The string to convert.
/// @param min The minimum allowed value.
/// @param max The maximum allowed value.
/// @param val The returned value.
bool StringToFloat(char const * const str, float min, float max, float &val);

/// Convert a string to an integer. The string can only contain characters that
/// are valid for an integer, otherwise this returns false.
/// @param str The string to convert.
/// @param min The minimum allowed value.
/// @param max The maximum allowed value.
/// @param val The returned value.
bool StringToInt(char const * const str, int min, int max, int &val);

/// Convert a string to an unsigned integer. The string can only contain
///  characters that are valid for an unsigned integer, otherwise this returns
///  false.
/// @param str The string to convert.
/// @param min The minimum allowed value.
/// @param max The maximum allowed value.
/// @param val The returned value.
bool StringToUnsignedInt(char const * const str, unsigned int min,
        unsigned int max, unsigned int &val);

/// Count the number of characters in the string. Note that this can count the
/// number of multibyte or ASCII characters.
/// This counts all first-bytes (the ones that don't match 10xxxxxx).
/// @param str The string to count.
size_t StringNumChars(char const * const str);

/// This counts the number of bytes until the null character.
/// @param str The string to count.
size_t StringNumBytes(char const * const str);

/// This indicates whether all characters in the string are ASCII.
/// @param str The string to check.
bool StringIsAscii(char const * const str);

/// Compares the strings using a case insensitive comparison, and returns zero
/// if they match, or greater or less than zero depending on the mismatch.
/// WARNING - this only work for ASCII
int StringCompareNoCase(char const * const str1, char const * const str2);

/// Compares the strings using a case insensitive comparison, and returns the
///  number of characters that match.
/// This probably only works for ASCII strings.  A tolower is performed for
/// every character in both strings, and then the strings are compared.
/// @param str1 The first string to compare.
/// @param str2 The second string to compare.
int StringCompareNoCaseNumCharsMatch(char const * const str1, char const * const str2);

/// This returns the position of the first space.
/// This only works on ASCII strings.
/// @param str The string to search.
/// @param startPos The starting position to search.
size_t StringFindSpace(char const * const str, size_t startPos);

/// This returns the position of the first non-space.
/// This only works on ASCII strings.
/// @param str The string to search.
/// @param startPos The starting position to search.
size_t StringFindNonSpace(char const * const str, size_t startPos);

/// A vector of strings.
class OovStringVec:public std::vector<class OovString>
    {
    public:
        /// Use the std::vector constructors.
        using vector::vector;

        /// Get a string at the index. This will return an empty string if
        /// the index is out of range.
        /// @param index The index of the string to get.
        OovString getStr(size_t index);

        /// Delete all of the strings that are empty.
        void deleteEmptyStrings();
    };

/// A set of strings.
class OovStringSet:public std::set<class OovString>
    {
    };

/// Get a vector of strings by parsing the string for delimiter characters.
/// @param str The string to parse.
/// @param delimiter The character to use as a delimiter.
OovStringVec StringSplit(char const * const str, char delimiter);

/// Get a vector of strings by parsing the string for delimiter strings.
/// @param str The string to parse.
/// @param delimiterStr The string to use as a delimiter.
OovStringVec StringSplit(char const * const str, char const * const delimiterStr);

/// Get a vector of strings by parsing the string for multiple delimiter strings.
/// @param str The string to parse.
/// @param delimiters The strings to use as delimiters.
/// @param keepZeroLengthStrings Keep all strings in the returned vector.
OovStringVec StringSplit(char const * const str, OovStringVec const &delimiters,
        bool keepZeroLengthStrings);

/// Convert a string to lower case.
/// This only works for ASCII strings.
/// @param str The string use as a source.
void StringToLower(class OovString &str);

/// A string to concatenate together by interspersing the delimiter.
/// @param tokens The tokens to combine.
/// @param delimiter The character to use as a delimiter.
class OovString StringJoin(OovStringVec const &tokens, char delimiter);

/// Gets a converted string where all special characters represented in XML
/// are converted to the XML equivalent.  For example, '>' becomes "&gt;".
/// @param str The string to use as a source
class OovString StringMakeXml(char const * const str);

/// Trims leading and trailing white space.
class OovString StringTrim(char const * const str);

/// This template requires T_Str to have a member "char const * const getStr()".
/// getStr() is used instead of c_str so that references to c_str() can be found and removed.
/// This uses CRTP (Curiously recurring template pattern).
/// This class is immutable. It does not do memory allocation or deallocation and
/// does not keep any memory.
template<typename T_Str> class OovStringRefInterface
    {
    public:
        /// Get the float value of the string. The string can only contain
        ///  characters that are valid for a floating point number, otherwise
        /// this returns false.
        /// @param min The minimum allowed value.
        /// @param max The maximum allowed value.
        /// @param val The returned value.
        bool getFloat(float min, float max, float &val) const
            { return StringToFloat(getThisStr(), min, max, val); }

        /// Get the integer value of the string. The string can only contain
        /// characters that are valid for an integer, otherwise this returns false.
        /// @param min The minimum allowed value.
        /// @param max The maximum allowed value.
        /// @param val The returned value.
        bool getInt(int min, int max, int &val) const
            { return StringToInt(getThisStr(), min, max, val); }

        /// Get the unsigned integer value of the string. The string can only contain
        /// characters that are valid for an integer, otherwise this returns false.
        /// @param min The minimum allowed value.
        /// @param max The maximum allowed value.
        /// @param val The returned value.
        bool getUnsignedInt(unsigned int min, unsigned int max,
                unsigned int &val) const
            { return StringToUnsignedInt(getThisStr(), min, max, val); }

        /// This returns the position of the first space.
        /// This only works on ASCII strings.
        /// @param startPos The starting position to search.
        size_t findSpace(size_t startPos=0) const
            { return StringFindSpace(getThisStr(), startPos);}

        /// This returns the position of the first non-space.
        /// This only works on ASCII strings.
        /// @param startPos The starting position to search.
        size_t findNonSpace(size_t startPos=0) const
            { return StringFindNonSpace(getThisStr(), startPos);}

        /// Count the number of characters in the string. Note that this can
        /// count the number of multibyte or ASCII characters.
        /// This counts all first-bytes (the ones that don't match 10xxxxxx).
        size_t numChars() const
            { return StringNumChars(getThisStr()); }

        /// This counts the number of bytes until the null character.
        /// @param str The string to count.
        size_t numBytes() const
            { return StringNumBytes(getThisStr()); }

        /// This indicates whether all characters in the string are ASCII.
        bool isAscii() const
            { return StringIsAscii(getThisStr()); }

        /// Get a vector of strings by parsing the string for delimiter characters.
        /// @param delim The character to use as a delimiter.
        OovStringVec split(char delim) const
            { return StringSplit(getThisStr(), delim); }

        /// Get a vector of strings by parsing the string for delimiter strings.
        /// @param delimiterStr The string to use as a delimiter.
        OovStringVec split(char const * const delimiterStr) const
            { return StringSplit(getThisStr(), delimiterStr); }

        /// Get a vector of strings by parsing the string for multiple delimiter
        /// strings.
        /// @param str The string to parse.
        /// @param delimiters The strings to use as delimiters.
        /// @param keepZeroLengthStrings Keep all strings in the returned vector.
        OovStringVec split(OovStringVec const &delimiters,
            bool keepZeroLengthStrings) const
            { return StringSplit(getThisStr(), delimiters, keepZeroLengthStrings); }

    protected:
        char const * getThisStr() const
            { return static_cast<T_Str const *>(this)->getStr(); }
    };


/// This is a constant string reference that does no memory allocation.
/// This will not modify the contents of the source string.
///
/// The purpose of this class is to replace "const char *" or
/// "const std::string &"
///
/// The typical usage of this is "OovStringRef const var" and not
/// "OovStringRef const &var".  This allows automatic conversion from a
/// literal string pointer.
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
        /// For common string functions, see OovStringRefInterface

        /// Get the character at the index.
        char operator[] (int index) const
            { return mStr[index]; }

        /// Get the string as a "char const *".
        char const * getStr() const
            { return mStr; }

        /// The conversion operator to get the string.
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
        // std::string crashes if passed a null pointer. Things like getenv()
        // can return a null pointer, so this prevents crashes.
        OovString(char const * const str):
            std::string(str ? str : "")
            {}
        OovString(OovStringRef const &str):
            std::string(str.getStr())
            {}
        OovString(OovString const &str):
            std::string(str)
            {}
        OovString(std::string const &str):
            std::string(str.c_str())
            {}
        OovString(std::string const &str, size_t pos, size_t len = npos):
            std::string(str, pos, len)
            {}
        // This provides the same interface as std::string, even though
        // for consistency, it would be better to take a pos and len.
        OovString(char const * const str, size_t len):
            std::string(str, len)
            {}
        OovString(size_t n, char c):
            std::string(n, c)
            {}
        /// These can prevent temporary OovStrings from being created.
        void operator=(char const * const str)
            { std::string::operator=(str ? str : ""); }
        void operator=(std::string const &str)
            { std::string::operator=(str); }
        void operator=(OovString const &str)
            { std::string::operator=(str); }
        void operator=(OovStringRef const &str)
            { std::string::operator=(str); }
        // Move assignment operator
        void operator=(OovString &&str)
            { std::string::operator=(str); }

        /// Use toupper to convert the string. This only works for ASCII strings.
        /// @param str The string to use as the source.
        void setUpperCase(OovStringRef const str);

        /// Use tolower to convert the string. This only works for ASCII strings.
        /// @param str The string to use as the source.
        void setLowerCase(OovStringRef const str);

        /// Append an integer. This does not append any whitespace.
        /// @param val The integer value to append.
        /// @param radix The radix, typically 10 or 16.
        void appendInt(int val, int radix=10, int minSpaces=0, int minDigits=1);

        /// Append a float. This does not append any whitespace.
        /// @param val The floating value to append.
        /// @param precision The default printf precision is 6.
        void appendFloat(float val, int precision=6);

        /// Concatenate a string together by interspersing the delimiter.
        /// @param tokens The tokens to combine.
        /// @param delimiter The character to use as a delimiter.
        void join(OovStringVec const &tokens, char delim)
            { *this = StringJoin(tokens, delim); }

        /// Replace all strings within a string to a different string.
        /// @param srchStr The string to use as a search string.
        /// @param repStr The string to use as the replace string.
        bool replaceStrs(OovStringRef const srchStr, OovStringRef const repStr);

        /// Convert some characters in a string so they can be encoded into XML.
        /// For example, "<" is converted to "&lt;"
        OovString getXml() const
            { return StringMakeXml(getStr()); }

        OovString getTrimmed() const
            { return StringTrim(getStr()); }

        /// Get the string as a "char const *"
        char const * getStr() const
            { return std::string::c_str(); }

        /// Get the string as an std::string.
        std::string &getStdString()
            { return *this; }
    };

#endif /* OOVSTRING_H_ */
