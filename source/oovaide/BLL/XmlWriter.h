/*
 * XmlWriter.h
 * Created on: Sept 11, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef XML_WRITER_H
#define XML_WRITER_H

#include "OovString.h"
#include <vector>

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

class XmlWriter:public OovString
    {
    public:
        XmlWriter():
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
        /// The close element string is extracted from the open string by
        /// looking for the element name that is ended by a space or end quote.
        /// @param openStr This should not have the '<' or '>' element characters.
        Element(OovStringRef openStr);
        Element(Element *parent, OovStringRef openStr);
        ~Element();
        void setParent(Element *parent)
            { mParent = parent; }
        void setBodyText(OovStringRef text)
            { mBodyStr = text; }
        void writeElementAndChildren(XmlWriter &writer);

    private:
        Element *mParent;
        std::vector<Element*> mChildren;
        OovString mOpenStr;
        OovString mBodyStr;     // Should be output using ATM_AppendLine
        OovString mCloseStr;
        /// This is used to tell if this element has children while writing.
        size_t mStartLocation;
        void addChild(Element *child)
            { mChildren.push_back(child); }
        void removeChild(Element *child);
        void writeOpening(XmlWriter &writer);
        void writeBody(XmlWriter &writer);
        void writeClosing(XmlWriter &writer);
        /// Check if this a root item that has children, but outputs nothing on
        /// its own.
        bool isNilRootElem() const
            { return(mOpenStr.length() == 0); }
    };

// Creates a root that has children, but does not output any text.
class XmlRoot:public Element
    {
    public:
        XmlRoot():
            Element(nullptr)
            {}
    };

class XmlHeader:public Element
    {
    public:
        XmlHeader(Element *parent=nullptr):
            Element(parent, "?xml version=\"1.0\" encoding=\"utf-8\"?")
            {}
    };

class Table:public Element
    {
    public:
        Table(Element *parent=nullptr):
            Element(parent, "table border=\"1\"")
            {}
    };

class TableHeader:public Element
    {
    public:
        TableHeader(Element *parent, OovStringRef text):
            Element(parent, "th")
            {
            setBodyText(text);
            }
    };

class TableRow:public Element
    {
    public:
        TableRow(Element *parent=nullptr):
            Element(parent, "tr")
            {
            }
    };

class TableCol:public Element
    {
    public:
        TableCol(Element *parent=nullptr):
            Element(parent, "td")
            {
            }
    };

class XslStyleSheet:public Element
    {
    public:
        XslStyleSheet(Element *parent=nullptr):
            Element(parent, "xsl:stylesheet version=\"1.0\" "
                "xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\"")
            {
            }
    };


// XSL classes

class XslApplyTemplates:public Element
    {
    public:
        // attrNameVals can be something like: "select=\"MemberAttrUseReport/Attr\""
        //
        // Output:
        //   <xsl:apply-templates select="MemberAttrUseReport/Attr">
        //    </xsl:apply-templates>
        XslApplyTemplates(Element *parent, OovStringRef attrNameVals):
            Element(parent, OovString("xsl:apply-templates ") + OovString(attrNameVals))
            {
            }
    };

class XslAttribute:public Element
    {
    public:
        // attrNameVals can be something like: "name=\"href\""
        // bodyVal can be something like:
        XslAttribute(Element *parent, OovStringRef attrNameVals, OovStringRef bodyVal):
            Element(parent, OovString("xsl:attribute ") + OovString(attrNameVals))
            {
            setBodyText(bodyVal);
            }
    };

class XslCallTemplate:public Element
    {
    public:
        // vals can be something like: "name=\"SumCodeLines\""
        XslCallTemplate(Element *parent, OovStringRef vals):
            Element(parent, OovString("xsl:call-template ") + OovString(vals))
            {
            }
    };

class XslElement:public Element
    {
    public:
        // vals can be something like: "name=\"a\""
        XslElement(Element *parent, OovStringRef vals):
            Element(parent, OovString("xsl:element ") + OovString(vals))
            {
            }
    };

class XslForEach:public Element
    {
    public:
        // vals can be something like: "name=\"a\""
        XslForEach(Element *parent, OovStringRef vals):
            Element(parent, OovString("xsl:for-each ") + OovString(vals))
            {
            }
    };

class XslKey:public Element
    {
    public:
        // vals can be something like: name=\"ModulesByModuleDir\"
        XslKey(Element *parent, OovStringRef vals):
            Element(parent, OovString("xsl:key ") + OovString(vals))
            {
            }
    };

class XslOutputHtml:public Element
    {
    public:
        XslOutputHtml(Element *parent):
            Element(parent, "xsl:output method=\"html\"")
            {
            }
    };

class XslSort:public Element
    {
    public:
        // text can be something like: "select=\"UseCount\" data-type=\"number\""
        XslSort(Element *parent, OovStringRef text):
            Element(parent, OovString("xsl:sort ") + OovString(text))
            {
            }
    };

class XslTemplate:public Element
    {
    public:
        // vals can be something like: "match=\"/\""
        XslTemplate(Element *parent, OovStringRef vals):
            Element(parent, OovString("xsl:template ") + OovString(vals))
            {
            }
    };

class XslText:public Element
    {
    public:
        // text can be something like: "This is text"
        XslText(Element *parent, OovStringRef text):
            Element(parent, "xsl:text ")
            {
            setBodyText(text);
            }
    };

class XslValueOf:public Element
    {
    public:
        // text can be something like: "This is text"
        XslValueOf(Element *parent, OovStringRef text):
            Element(parent, OovString("xsl:value-of ") + OovString(text))
            {
            }
    };

};

#endif
