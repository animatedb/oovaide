/*
 * NameValueFile.cpp
 *
 *  Created on: Aug 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */
#include "NameValueFile.h"
#include "FilePath.h"
#include <stdio.h>
#include <string.h>


std::string CompoundValueRef::getAsString(const std::vector<std::string> &vec,
	char delimiter)
    {
    std::string str;
    for(size_t i=0; i<vec.size(); i++)
	{
	if(i != 0)
	    str += delimiter;
	str += vec[i];
	}
    return str;
    }

std::string CompoundValueRef::getAsString(const std::set<std::string> &stdset,
	char delimiter)
    {
    std::string str;
    for(auto const &item : stdset)
	{
	if(str.empty())
	    str += delimiter;
	str += item;
	}
    return str;
    }


void CompoundValueRef::parseStringRef(char const * const strIn,
	std::vector<std::string> &vec, char delimiter)
    {
    std::string str = strIn;
    size_t endColonPos = 0;
    size_t startArgPos = 0;
    while(startArgPos != std::string::npos)
	{
	endColonPos = str.find(delimiter, startArgPos);
	std::string tempStr = str.substr(startArgPos, endColonPos-startArgPos);
	if(tempStr.length() > 0)
	    vec.push_back(tempStr);
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
    std::string lineBuf;
    str.resize(0);
    lineBuf.resize(1000);
    while(fgets(&lineBuf[0], lineBuf.size(), fp))
	{
	str += &lineBuf[0];
	if(strchr(&lineBuf[0], '\n'))
	    break;
	}
    return(str.length() > 0);
    }

void NameValueRecord::read(FILE *fp)
    {
    std::string lineBuf;
    while(getLine(fp, lineBuf))
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


