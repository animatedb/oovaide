/*
 * XmlWriter.h
 * Created on: Sept 11, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef XML_WRITER_H
#define XML_WRITER_H

#include "OovString.h"

// Example - scope determines when a destructor is called to output the
// close element.
//
// Writer xml;
// XmlHeader(xml);
//   {
//   Table(xml);
//     {
//     TableRow(xml);
//     }
//   }

namespace XML
{

enum AppendTextModes
    {
    ATM_AppendPrespace=0x01,
    ATM_AppendText=0x02,
    ATM_AppendEndLine=0x04,
    ATM_AppendLine=0x07
    };

class Writer:public OovString
    {
    public:
        Writer():
            mLevel(0), mHaveOpenElemEnd(false)
            {}
        void incLevel()
            {
            mLevel++;
            }
        void decLevel()
            {
            mLevel--;
            }
        void appendText(OovStringRef str, int atm);
        // This indicates that an open element has not been finished.
        void setNeedOpenElemEnd()
            { mHaveOpenElemEnd = true; }
        // If there are children, then it must be closed with a '>'. Otherwise
        // for no children it will be closed with a "/>".
        // The other case is that <?xml... should be closed with no slash.
        void flushOpenElemEnd(bool closeWithSlash);

    private:
        int mLevel;
        bool mHaveOpenElemEnd;
        void append(OovStringRef str, int atm);
    };

class Element
    {
    public:
        // The close element string is extracted from the open string by
        // looking for the element name that is ended by a space or end quote.
        // @param openStr This should not have the '<' or '>' element characters.
        Element(Writer &writer, OovStringRef openStr);
        ~Element();

    private:
        Writer &mWriter;
        OovString mCloseStr;
        size_t mStartLocation;
    };

class XmlHeader:public Element
    {
    public:
        XmlHeader(Writer &writer):
            Element(writer, "?xml version=\"1.0\" encoding=\"utf-8\"?")
            {}
    };

class Table:public Element
    {
    public:
        Table(Writer &writer):
            Element(writer, "table border=\"1\"")
            {}
    };

class TableHeader:public Element
    {
    public:
        TableHeader(Writer &writer, OovStringRef text):
            Element(writer, "th")
            {
            writer.appendText(text, ATM_AppendLine);
            }
    };

class TableRow:public Element
    {
    public:
        TableRow(Writer &writer):
            Element(writer, "tr")
            {
            }
    };

class TableCol:public Element
    {
    public:
        TableCol(Writer &writer):
            Element(writer, "td")
            {
            }
    };

class XslStyleSheet:public Element
    {
    public:
        XslStyleSheet(Writer &writer):
            Element(writer, "xsl:stylesheet version=\"1.0\" "
                "xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\"")
            {
            }
    };

class XslOutputHtml:public Element
    {
    public:
        XslOutputHtml(Writer &writer):
            Element(writer, "xsl:output method=\"html\"")
            {
            }
    };

class XslTemplate:public Element
    {
    public:
        // vals can be something like: "match=\"/\""
        XslTemplate(Writer &writer, OovStringRef vals):
            Element(writer, OovString("xsl:template ") + OovString(vals))
            {
            }
    };

class XslApplyTemplates:public Element
    {
    public:
        // vals can be something like: "select=\"MemberAttrUseReport/Attr\""
        XslApplyTemplates(Writer &writer, OovStringRef vals):
            Element(writer, OovString("xsl:apply-templates ") + OovString(vals))
            {
            }
    };

class XslText:public Element
    {
    public:
        // text can be something like: "This is text"
        XslText(Writer &writer, OovStringRef text):
            Element(writer, "xsl:text ")
            {
            writer.appendText(text, ATM_AppendLine);
            }
    };

class XslValueOf:public Element
    {
    public:
        // text can be something like: "This is text"
        XslValueOf(Writer &writer, OovStringRef text):
            Element(writer, OovString("xsl:value-of ") + OovString(text))
            {
            }
    };

class XslSort:public Element
    {
    public:
        // text can be something like: "select=\"UseCount\" data-type=\"number\""
        XslSort(Writer &writer, OovStringRef text):
            Element(writer, OovString("xsl:sort ") + OovString(text))
            {
            }
    };

};

#endif
