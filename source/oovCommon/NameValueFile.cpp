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
#include "OovError.h"
#include <stdio.h>
#include <string.h>



OovString CompoundValueRef::getAsString(const OovStringVec &vec,
        char delimiter)
    {
    OovString str;
    for(size_t i=0; i<vec.size(); i++)
        {
        str += vec[i];
        str += delimiter;
        }
    return str;
    }

OovString CompoundValueRef::getAsString(const OovStringSet &stdset,
        char delimiter)
    {
    OovString str;
    for(auto const &item : stdset)
        {
        str += item;
        str += delimiter;
        }
    return str;
    }


void CompoundValueRef::parseStringRef(OovStringRef const strIn,
        OovStringVec &vec, char delimiter)
    {
    OovString str = strIn;
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

OovStringVec CompoundValueRef::parseString(OovStringRef const str,
        char delimiter)
    {
    OovStringVec vec;
    parseStringRef(str, vec, delimiter);
    return vec;
    }

void CompoundValue::quoteCommandLineArg(std::string &str)
    {
    FilePathQuoteCommandLinePath(str);
    }

void CompoundValue::quoteAllArgs()
    {
    for(size_t i=0; i<size(); i++)
        {
        std::string &tmp = at(i);
        quoteCommandLineArg(tmp);
        }
    }

size_t CompoundValue::find(OovStringRef const str)
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

OovString NameValueRecord::getValue(OovStringRef const optionName) const
    {
    OovString val;
    auto const pos = mNameValues.find(optionName);
    if(pos != mNameValues.end())
        val = (*pos).second;
    return val;
    }

void NameValueRecord::setNameValue(OovStringRef const optionName,
        OovStringRef const value)
    {
    mNameValues[optionName] = value;
    }

void NameValueRecord::setNameValueBool(OovStringRef const optionName, bool val)
    {
    setNameValue(optionName, val ? "Yes" : "No");
    }

bool NameValueRecord::getValueBool(OovStringRef const optionName) const
    {
    return(getValue(optionName) == "Yes");
    }

bool NameValueRecord::write(File &file)
    {
    bool success = true;
    for(const auto &pair : mNameValues)
        {
        if(pair.second.length() > 0 || mSaveNullValues)
            {
            OovString str = pair.first;
            str += mapDelimiter;
            str += pair.second;
            str += '\n';
            file.putString(str);
            }
        }
    return success;
    }

bool NameValueRecord::getLine(File &file, OovString &str, bool &success)
    {
    char lineBuf[1000];
    str.resize(0);
    success = true;
    // Read many times until the \n is found.
    while(file.getString(lineBuf, sizeof(lineBuf), success))
        {
        str += lineBuf;
        size_t len = str.length();
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

bool NameValueRecord::read(File &file)
    {
    OovString lineBuf;
    bool success = true;
    while(getLine(file, lineBuf, success))
        {
        insertLine(lineBuf);
        }
    return success;
    }

void NameValueRecord::insertLine(OovString lineBuf)
    {
    size_t colonPos = lineBuf.find(mapDelimiter);
    if(colonPos != std::string::npos)
        {
        std::string key = lineBuf.substr(0, colonPos);
        size_t termPos = lineBuf.find('\n');
        std::string value = lineBuf.substr(colonPos+1, termPos-(colonPos+1));
        setNameValue(key, value);
        }
    }

void NameValueRecord::insertBufToMap(OovString const buf)
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

void NameValueRecord::readMapToBuf(OovString &buf)
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

bool NameValueFile::writeFile(File &file)
    {
    return write(file);
    }

bool NameValueFile::writeFile()
    {
    bool success = false;
    File file(mFilename.getStr(), "w");
    if(file.isOpen())
        {
        success = writeFile(file);
        }
    return success;
    }

bool NameValueFile::readFile(File &file)
    {
    return read(file);
    }

bool NameValueFile::readFile()
    {
    bool success = false;
    File file(mFilename.getStr(), "r");
    if(file.isOpen())
        {
        clear();
        success = readFile(file);
        }
    return success;
    }

bool NameValueFile::readOpenedFile(SharedFile &file)
    {
    clear();
    std::string buf(file.getSize(), 0);
    int actualSize;
    bool success = file.read(&buf[0], static_cast<int>(buf.size()), actualSize);
    if(success)
        {
        buf.resize(static_cast<size_t>(actualSize));
        insertBufToMap(buf);
        }
    return success;
    }

bool NameValueFile::readFileShared()
    {
    SharedFile file;
    bool success = false;
    eOpenStatus status = file.open(mFilename.getStr(), M_ReadShared);
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
    eOpenStatus status = file.open(mFilename.getStr(), M_ReadWriteExclusive);
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
        OovString buf;
        readMapToBuf(buf);
        file.truncate();
        file.seekBegin();
        success = file.write(&buf[0], static_cast<int>(buf.size()));
        }
    return success;
    }

