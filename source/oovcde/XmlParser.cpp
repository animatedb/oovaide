/**
*   Copyright (C) 2008 by dcblaha
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*  @file      xmlParser.cpp
*
*  @author    dcblaha
*
*  @date      2/1/2009
*
*/
// Copied from ntaftawt netload.cpp.


/*

This contains a simple subset XML parser.  It will not read all types of XML.


A specific xmi parser is built on top of the XML parser. This can load
a file such as the following.

<...>
  <UML:DataType stereotype="datatype" elementReference="typeid1" />
  <UML:Class ... name="cExampleClass" />
    <UML:Attribute type="typeid1" name="mAttrName" />
</...>

*/

#include "XmlParser.h"
#include <stdio.h>
#include <string.h>

static char const sWhiteSpaceStr[] = " \t\n\r";
static char const sTokenStr[] = " \t\n\r\"\'=<>";

static bool sDeclarationElement = false;

XmlError XmlParser::parseXml(char const * const buf)
    {
    XmlError errCode;
    char const *p = buf;
    p = strchr(p, '<');
    if(p)
        {
        p++;      // Skip '<'
        errCode = parseElem(p);
        if(sDeclarationElement && p)
            {
            p = strchr(p, '<');
            if(buf)
                {
                p++;
                errCode = parseElem(p);
                }
            }
        }
    else
        errCode.setError(ERROR_NO_ELEMS);
    return errCode;
    }

XmlError XmlParser::parseAttr(char const *&buf)
    {
    char const * attrName;
    int attrNameLen;
    XmlError errCode = parseName(buf, attrName, attrNameLen);
    if(errCode.isOK())
        {
        int quoteChar;
        buf += attrNameLen;
        char const *startVal = strchr(buf, '=');
        if(startVal)
            {
            startVal++;
            startVal = startVal + strcspn(startVal, sTokenStr);
            quoteChar = *startVal;
            startVal++;
            }
        if(startVal)
            {
            char const *end = strchr(startVal, quoteChar);
            if(end)
        	{
		int len = end - startVal;
		onAttr(attrName, attrNameLen, startVal, len);
		buf = startVal + len;
        	}
            }
        else
            errCode.setError(ERROR_BAD_VALUE);
        }
    return errCode;
    }

XmlError XmlParser::parseElemValue(char const *& buf)
    {
    char const * const value = buf;
    char const * const endVal = strchr(buf, '<');
    unsigned int len = endVal - buf;
    XmlError errCode;
    onElemValue(value, len);
    buf = endVal;
    return errCode;
    }

XmlError XmlParser::eatElementEndTag(char const *& buf)
    {
    XmlError errCode;
    buf = strchr(buf, '>');
    if(buf)
        buf++;
    return errCode;
    }

// Recursive
// buf must point to after the '<' character.
XmlError XmlParser::parseElem(char const *&buf)
    {
    char const * elemName = nullptr;
    int elemNameLen = 0;
    XmlError errCode = parseName(buf, elemName, elemNameLen);
    if(errCode.isOK())
        {
        sDeclarationElement = (elemName[0] == '?');
        onOpenElem(elemName, elemNameLen);
        buf += elemNameLen;
        }
    bool inElementStart = true;
    while(buf && errCode.isOK())
        {
        if(*buf == '<' && buf[1] == '/')
            {
            buf++;
            eatElementEndTag(buf);
            break;
            }
        else if(*buf == '<')
            {
            buf++;
            errCode = parseElem(buf);
            }
        else if(*buf == '>')
            {
            inElementStart = false;
            buf++;
            }
        else if((*buf == '/' || *buf == '?') && buf[1] == '>')
            {
            buf+=2;
            break;
            }
        else if(isNameChar(*buf))
            {
            if(inElementStart)
                errCode = parseAttr(buf);
            else
                errCode = parseElemValue(buf);
            }
        else
            buf++; 
        }
    if(errCode.isOK())
        onCloseElem(elemName, elemNameLen);
    return errCode;
    }

// buf can point to the white space before the name, and will be updated to point
// to the first character after the name.
XmlError XmlParser::parseName(char const *&buf, char const *&name,
	int &nameLen)
    {
    XmlError errCode;
    char const * const startName = buf + strspn(buf, sWhiteSpaceStr);
    if(*startName)
        {
        nameLen = strcspn(startName, sTokenStr);
        name = startName;
        }
    else
        errCode.setError(ERROR_BAD_NAME);
    return errCode;
    }

void XmlParser::stringCbCopy(char *dest, int bytes, char const * const src)
    {
    strncpy(dest, src, bytes);
    dest[bytes-1] = '\0';
    }

bool XmlParser::isNameChar(char ch)
    {
    return(!strchr(sTokenStr, ch));
    }


