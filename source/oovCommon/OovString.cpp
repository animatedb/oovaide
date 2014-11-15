/*
 * OovString.cpp
 *
 *  Created on: Oct 6, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */
#include "OovString.h"
#include <memory.h>
#include <string.h>
#include <stdio.h>

OovString OovStringVec::getStr(size_t index)
    {
    OovString str;
    if(index < size())
	str = at(index);
    return str;
    }

static int isAsciiLenOk(char const * const buf, int maxLen)
    {
    bool success = false;
    for(int i=0; i<maxLen; i++)
	{
	if(buf[i] == '\0')
	    {
	    success = true;
	    break;
	    }
	}
    return success;
    }

// srcBytes must include null.
static bool copyString(char * const dstStr, int maxDstBytes,
    char const * const srcStr, int srcBytes)
    {
    unsigned int len = std::min(maxDstBytes, srcBytes);
    memcpy(dstStr, srcStr, len);
    dstStr[maxDstBytes-1] = '\0';
    return(srcBytes > maxDstBytes);
    }

bool StringToFloat(char const * const mbStr, float min, float max, float &val)
    {
    bool success = false;
    float valNum = 0;
    char dummyc = 0;
    int ret = sscanf(mbStr, "%f%c", &valNum, &dummyc);
    if(ret == 1 && (valNum >= min && valNum <= max))
	{
	val = valNum;
	success = true;
	}
    return success;
    }

bool StringToInt(const char *mbStr, int min, int max, int &val)
    {
    bool success = false;
    int valNum = 0;
    char dummyc = 0;
    int ret = sscanf(mbStr, "%d%c", &valNum, &dummyc);
    if(ret == 1 && (valNum >= min && valNum <= max))
	{
	val = valNum;
	success = true;
	}
    return success;
    }

bool StringToUnsignedInt(const char *mbStr, unsigned int min, unsigned int max,
	unsigned int &val)
    {
    bool success = false;
    unsigned int valNum = 0;
    char dummyc = 0;
    int ret = sscanf(mbStr, "%u%c", &valNum, &dummyc);
    if(ret == 1 && (valNum >= min && valNum <= max))
	{
	val = valNum;
	success = true;
	}
    return success;
    }

void StringToLower(std::string &str)
    {
    for(size_t i=0; i<str.length(); ++i)
	{
	str.at(i) = tolower(str.at(i));
	}
    }

int StringCompareNoCase(char const * str1, char const * str2)
    {
    while(*str1)
	{
	if(tolower(*str1) != tolower(*str2))
	    break;
	str1++;
	str2++;
	}
    return(*str1-*str2);
    }

bool IntToAsciiString(int value, char * const buffer, size_t dstSizeInBytes, int radix)
    {
    bool success = false;
    if(dstSizeInBytes > 0)
    	{
	const int numBufSize = 30;
    	char numBuf[numBufSize];
	if(radix == 10)
	    snprintf(numBuf, sizeof(numBuf), "%d", value);
	else
	    snprintf(numBuf, sizeof(numBuf), "%x", value);
	if(isAsciiLenOk(numBuf, dstSizeInBytes))
	    {
	    copyString(buffer, dstSizeInBytes, numBuf, numBufSize);
	    success = true;
	    }
	else if(dstSizeInBytes >= 2)
	    {
	    buffer[0] = '*';
	    buffer[1] = '\0';
	    }
	else
	    buffer[0] = '\0';
    	}
    return success;
    }

// This can count the number of multibyte or ASCII characters.
// Count all first-bytes (the ones that don't match 10xxxxxx).
int StringNumChars(const char *s)
    {
    int len = 0;
    while(*s)
	    {
	    len += ((*s++ & 0xc0) != 0x80);
	    }
    return len;
    }

bool StringIsAscii(const char *p)
    {
    while(*p)
	    {
	    if(*p & 0x80)	// Stop on any character that is not ASCII
		    break;
	    p++;
	    }
    return *p == '\0';	// If at end, there is no non-ASCII character
    }

size_t StringFindSpace(char const *str, size_t startPos)
    {
    char const * p = str + startPos;
    while(*p)
	{
	if(isspace(*p))
	    break;
	else
	    p++;
	}
    if(*p == '\0')
	return std::string::npos;
    else
	return p - str;
    }

//////////////

void OovString::setUpperCase(char const * const str)
{
    char const *p = str;
    while(*p)
	{
	*this += toupper(*p);
	p++;
	}
}

void OovString::setLowerCase(char const * const str)
{
    *this = str;
    StringToLower(*this);
}

void OovString::appendInt(int val, int radix)
    {
    char buf[30];
    IntToAsciiString(val, buf, sizeof(buf), radix);
    append(buf);
    }

OovStringVec StringSplit(char const * str, char delimiter)
    {
    return StringSplit(str, std::string(1, delimiter).c_str());
    }

OovStringVec StringSplit(char const * str, char const *delimiterStr)
{
    std::string tempStr = str;
    OovStringVec tokens;
    size_t start = 0;
    size_t end = 0;
    const int delimLen = strlen(delimiterStr);
    while(end != std::string::npos)
	{
        end = tempStr.find(delimiterStr, start);
        tokens.push_back(tempStr.substr(start,
	   (end == std::string::npos) ? std::string::npos : end - start));
        start = (( end > (std::string::npos - delimLen)) ?
        	std::string::npos : end + delimLen);
	}
    return tokens;
    }

OovStringVec StringSplit(char const * str, std::vector<std::string> const &delimiters)
    {
    std::string tempStr = str;
    OovStringVec tokens;
    size_t start = 0;
    size_t end = 0;
    size_t lowestEnd = 0;
    while(end != std::string::npos)
	{
	lowestEnd = std::string::npos;
	size_t delimLen = std::string::npos;
	for(auto const &delim : delimiters)
	    {
	    end = tempStr.find(delim, start);
	    if(end < lowestEnd)
		{
		delimLen = strlen(delim.c_str());
		lowestEnd = end;
		}
	    }
	end = lowestEnd;
        tokens.push_back(tempStr.substr(start,
	   (end == std::string::npos) ? std::string::npos : end - start));
        start = (( end > (std::string::npos - delimLen)) ?
        	std::string::npos : end + delimLen);
	}
    return tokens;
    }

OovString StringJoin(OovStringVec const &tokens, char delimiter)
    {
    OovString str;
    for(size_t i=0; i<tokens.size(); i++)
	{
	str += tokens[i];
	if(i < tokens.size()-1)
	    {
	    str += delimiter;
	    }
	}
    return str;
    }

void OovString::replaceStrs(char const *srchStr, char const *repStr)
    {
    size_t starti = 0;
    do
	{
	starti = find(srchStr);
	if(starti != OovString::npos)
	    {
	    std::string::replace(starti, strlen(srchStr), repStr);
	    }
	} while(starti != OovString::npos);
    }
