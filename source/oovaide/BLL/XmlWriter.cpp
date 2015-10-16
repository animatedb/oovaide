/*
 * XmlWriter.cpp
 * Created on: Sept 11, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "XmlWriter.h"
#include <algorithm>

using namespace XML;

void XmlWriter::appendText(OovStringRef str, int atm)
    {
    flushOpenElemEnd(false);
    append(str, atm);
    }

void XmlWriter::flushOpenElemEnd(bool closeWithSlash)
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

void XmlWriter::append(OovStringRef str, int atm)
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

Element::Element(OovStringRef openStr):
    mParent(nullptr)
    {
    if(openStr)         // Root does not have any output.
        {
        OovString str = std::string("<");
        str += openStr;
        mOpenStr = str;
        mCloseStr = openStr;
        size_t pos = mCloseStr.find(' ');
        if(pos != std::string::npos)
            {
            mCloseStr.erase(pos);
            }
        }
    }

Element::Element(Element *parent, OovStringRef openStr):
    Element(openStr)
    {
    setParent(parent);
    if(parent)
        {
        parent->addChild(this);
        }
    }

Element::~Element()
    {
    if(mParent)
        {
        mParent->removeChild(this);
        }
    }

void Element::removeChild(Element *child)
    {
    mChildren.erase(std::remove(mChildren.begin(), mChildren.end(), child),
        mChildren.end());
    }

void Element::writeElementAndChildren(XmlWriter &writer)
    {
    writeOpening(writer);
    writeBody(writer);
    for(auto const &child : mChildren)
        {
        child->writeElementAndChildren(writer);
        }
    writeClosing(writer);
    }

void Element::writeOpening(XmlWriter &writer)
    {
    if(!isNilRootElem())
        {
        writer.appendText(mOpenStr, ATM_AppendPrespace | ATM_AppendText);
            writer.setNeedOpenElemEnd();
        mStartLocation = writer.length();
        writer.incLevel();
        }
    }

void Element::writeBody(XmlWriter &writer)
    {
    if(mBodyStr.length() > 0)
        {
        writer.appendText(mBodyStr, ATM_AppendLine);
        }
    }

void Element::writeClosing(XmlWriter &writer)
    {
    if(!isNilRootElem())
        {
        writer.decLevel();
        bool hasChildren = (mStartLocation != writer.length());
        bool closeWithSlash = !hasChildren;
        if(mCloseStr.length() > 0 && mCloseStr[0] == '?')
            {
            closeWithSlash = false;
            }
        writer.flushOpenElemEnd(closeWithSlash);
        if(hasChildren)
            {
            OovString str;
            if(mCloseStr.length() > 0)
                {
                str += "</";
                str += mCloseStr;
                str += '>';
                }
            writer.appendText(str, ATM_AppendLine);
            }
        }
    }
