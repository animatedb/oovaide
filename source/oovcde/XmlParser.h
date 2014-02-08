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
*  @file      xmlParser.h
*
*  @author    dcblaha
*
*  @date      2/1/2009
*
*/

enum XmlErrorType
    {
    ERROR_NONE, ERROR_NO_ELEMS, ERROR_BAD_NAME, ERROR_BAD_VALUE
    };

class XmlError
    {
    public:
	XmlError():
            mError(ERROR_NONE)
            {}
	XmlError(const XmlError &errCode):
            mError(errCode.mError)
            {}
        void operator=(const XmlError &errCode)
            { mError = errCode.mError; }
        void setError(XmlErrorType err)
            { mError = err; }
        operator XmlErrorType() const
            { return mError; }
        bool isOK() const
            { return(mError == ERROR_NONE); }

    private:
        XmlErrorType mError;
    };


class XmlParser
    {
    public:
        XmlError parseXml(char const * const buf);
        virtual ~XmlParser()
            {}

    protected:
        virtual void onOpenElem(char const * const name, int len)=0;
        virtual void onCloseElem(char const * const name, int len)=0;
        virtual void onAttr(char const * const name, int &nameLen,
        	char const * const val, int &valLen)=0;
        virtual void onElemValue(char const * const /*val*/, int /*len*/)
            {}

    private:
        XmlError parseAttr(char const *&buf);
        XmlError parseElem(char const *&buf);
        XmlError parseElemValue(char const *& buf);
        XmlError parseName(char const *&buf, char const *&name, int &nameLen);
        XmlError eatElementEndTag(char const *& buf);
        void stringCbCopy(char *dest, int bytes, char const * const src);
        bool isNameChar(char ch);
    };


