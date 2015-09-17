/*
 * XmlWriter.cpp
 * Created on: Sept 11, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "XmlWriter.h"

using namespace XML;

void Writer::appendText(OovStringRef str, int atm)
    {
    flushOpenElemEnd(false);
    append(str, atm);
    }

void Writer::flushOpenElemEnd(bool closeWithSlash)
    {
    if(mHaveOpenElemEnd)
        {
        std::string str;
        if(closeWithSlash)
            {
            str = "/>";
            }
        else
            {
            str = ">";
            }
        append(str, ATM_AppendText | ATM_AppendEndLine);
        mHaveOpenElemEnd = false;
        }
    }

void Writer::append(OovStringRef str, int atm)
    {
    if(atm & ATM_AppendPrespace)
        {
        OovString levelStr(mLevel*2, ' ');
        OovString::operator+=(levelStr);
        }
    if(atm & ATM_AppendText)
        {
        OovString::operator+=(str);
        }
    if(atm & ATM_AppendEndLine)
        {
        OovString::operator+=("\n");
        }
    }

Element::Element(Writer &writer, OovStringRef openStr):
    mWriter(writer)
    {
    OovString str = std::string("<");
    str += openStr;

    mCloseStr = openStr;
    size_t pos = mCloseStr.find(' ');
    if(pos != std::string::npos)
        {
        mCloseStr.erase(pos);
        }

    mWriter.appendText(str, ATM_AppendPrespace | ATM_AppendText);
    mWriter.setNeedOpenElemEnd();
    mStartLocation = mWriter.length();
    mWriter.incLevel();
    }

Element::~Element()
    {
    mWriter.decLevel();
    bool hasChildren = (mStartLocation != mWriter.length());
    bool closeWithSlash = !hasChildren;
    if(mCloseStr.length() > 0 && mCloseStr[0] == '?')
        {
        closeWithSlash = false;
        }
    mWriter.flushOpenElemEnd(closeWithSlash);
    if(hasChildren)
        {
        OovString str;
        if(mCloseStr.length() > 0)
            {
            str += "</";
            str += mCloseStr;
            str += '>';
            }
        mWriter.appendText(str, ATM_AppendLine);
        }
    }
