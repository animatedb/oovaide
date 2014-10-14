/*
 * NameValueFile.cpp
 *
 *  Created on: Aug 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */
#include "NameValueFile.h"
#include "FilePath.h"
#include "File.h"
#include "Debug.h"
#include <stdio.h>
#include <string.h>



std::string CompoundValueRef::getAsString(const std::vector<std::string> &vec,
	char delimiter)
    {
    std::string str;
    for(size_t i=0; i<vec.size(); i++)
	{
	str += vec[i];
	str += delimiter;
	}
    return str;
    }

std::string CompoundValueRef::getAsString(const std::set<std::string> &stdset,
	char delimiter)
    {
    std::string str;
    for(auto const &item : stdset)
	{
	str += item;
	str += delimiter;
	}
    return str;
    }


void CompoundValueRef::parseStringRef(char const * const strIn,
	std::vector<std::string> &vec, char delimiter)
    {
    std::string str = strIn;
    size_t startArgPos = 0;
    while(startArgPos != std::string::npos)
	{
	size_t endColonPos = str.find(delimiter, startArgPos);
	std::string tempStr = str.substr(startArgPos, endColonPos-startArgPos);
	// For compatibility with previous files, allow a string that is
	// after the last colon. Previous versions that had the extra string
	// did not allow null strings.
	if(endColonPos != std::string::npos || tempStr.length() > 0)
	    {
	    vec.push_back(tempStr);
	    }
	startArgPos = endColonPos;
	if(startArgPos != std::string::npos)
	    startArgPos++;
	}
    }

std::vector<std::string> CompoundValueRef::parseString(char const * const str,
	char delimiter)
    {
    std::vector<std::string> vec;
    parseStringRef(str, vec, delimiter);
    return vec;
    }

void CompoundValue::quoteCommandLineArg(std::string &str)
    {
    quoteCommandLinePath(str);
    }

void CompoundValue::quoteAllArgs()
    {
    for(size_t i=0; i<size(); i++)
	{
	std::string &tmp = at(i);
	quoteCommandLineArg(tmp);
	}
    }

size_t CompoundValue::find(char const * const str)
    {
    size_t pos = npos;
    for(size_t i=0; i<size(); i++)
	{
	if(at(i).compare(str) == 0)
	    {
	    pos = i;
	    break;
	    }
	}
    return pos;
    }

////////

std::string NameValueRecord::getValue(char const * const optionName) const
    {
    std::string val;
    auto const pos = mNameValues.find(optionName);
    if(pos != mNameValues.end())
	val = (*pos).second;
    return val;
    }

void NameValueRecord::setNameValue(char const * const optionName,
	char const * const value)
    {
    mNameValues[optionName] = value;
    }

void NameValueRecord::setNameValueBool(char const * const optionName, bool val)
    {
    setNameValue(optionName, val ? "Yes" : "No");
    }

bool NameValueRecord::getValueBool(char const * const optionName) const
    {
    return(getValue(optionName) == "Yes");
    }

void NameValueRecord::write(FILE *fp)
    {
    for(const auto &pair : mNameValues)
	{
	if(pair.second.length() > 0 || mSaveNullValues)
	    {
	    fprintf(fp, "%s%c%s\n", pair.first.c_str(), mapDelimiter, pair.second.c_str());
	    }
	}
    }

bool NameValueRecord::getLine(FILE *fp, std::string &str)
    {
    char lineBuf[1000];
    str.resize(0);
    // Read many times until the \n is found.
    while(fgets(lineBuf, sizeof(lineBuf), fp))
	{
	str += lineBuf;
	int len = str.length();
	if(str[len-1] == '\n')
	    {
	    str.resize(len-1);
	    break;
	    }
	else if(len > 50000)
	    {
	    break;
	    }
	}
    return(str.length() > 0);
    }

void NameValueRecord::read(FILE *fp)
    {
    std::string lineBuf;
    while(getLine(fp, lineBuf))
	{
	insertLine(lineBuf);
	}
    }

void NameValueRecord::insertLine(std::string lineBuf)
    {
    size_t colonPos = lineBuf.find(mapDelimiter);
    if(colonPos != std::string::npos)
	{
	std::string key = lineBuf.substr(0, colonPos);
	size_t termPos = lineBuf.find('\n');
	std::string value = lineBuf.substr(colonPos+1, termPos-(colonPos+1));
	setNameValue(key.c_str(), value.c_str());
	}
    }

void NameValueRecord::insertBufToMap(std::string const &buf)
    {
    size_t pos = 0;
    mNameValues.clear();
    while(pos != std::string::npos)
	{
	size_t endPos = buf.find('\n', pos);
	std::string line(buf, pos, endPos-pos);
	insertLine(line);
	pos = endPos;
	if(pos != std::string::npos)
	    pos++;
	}
    }

void NameValueRecord::readMapToBuf(std::string &buf)
    {
    buf.clear();
    for(const auto &pair : mNameValues)
	{
	if(pair.second.length() > 0 || mSaveNullValues)
	    {
	    buf += pair.first;
	    buf += mapDelimiter;
	    buf += pair.second;
	    buf += '\n';
	    }
	}
    }

//////////

bool NameValueFile::writeFile()
	{
	FILE *fp = fopen(mFilename.c_str(), "w");
	if(fp)
	    {
	    write(fp);
	    fclose(fp);
	    }
	return(fp != nullptr);
	}

bool NameValueFile::readFile()
	{
	FILE *fp = fopen(mFilename.c_str(), "r");
	if(fp)
	    {
	    clear();
	    read(fp);
	    fclose(fp);
	    }
	return(fp != nullptr);
	}

bool NameValueFile::readOpenedFile(SharedFile &file)
    {
    clear();
    std::string buf(file.getSize(), 0);
    int actualSize;
    bool success = file.read(&buf[0], buf.size(), actualSize);
    if(success)
	{
	buf.resize(actualSize);
	insertBufToMap(buf);
	}
    return success;
    }

bool NameValueFile::readFileShared()
    {
    SharedFile file;
    bool success = false;
    eOpenStatus status = file.open(mFilename.c_str(), M_ReadShared);
    if(status == OS_Opened)
	{
	success = readOpenedFile(file);
	}
    else if(status == OS_NoFile)
	{
	success = true;
	}
    return(success);
    }

bool NameValueFile::writeFileExclusiveReadUpdate(class SharedFile &file)
    {
    bool success = false;
    eOpenStatus status = file.open(mFilename.c_str(), M_ReadWriteExclusive);
    if(status == OS_Opened)
	{
	success = readOpenedFile(file);
	}
    return success;
    }

bool NameValueFile::writeFileExclusive(class SharedFile &file)
    {
    bool success = file.isOpen();
    if(success)
	{
	std::string buf;
	readMapToBuf(buf);
	file.truncate();
	file.seekBegin();
	success = file.write(&buf[0], buf.size());
	}
    return success;
    }

