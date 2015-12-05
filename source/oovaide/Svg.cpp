/*
 * Svg.cpp
 *
 *  Created on: Jul 3, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */


#include "Svg.h"
#include "DiagramDrawer.h"
#include "OovString.h"

/*
Simple SVG example.

<svg xmlns="http://www.w3.org/2000/svg" version="1.1">
  <g fill="rgb(245,245,255)" stroke="rgb(0,0,0)" style="stroke-width:1">
  <rect width="300" height="100" />
  </g>
  <text x="5" y="40" fill="black">This is SVG</text>
</svg>
*/

static void addArg(std::string &str, OovStringRef const argName, OovStringRef const value)
    {
    str += " ";
    str += argName;
    str += "=\"";
    str += value;
    str += "\"";
    }

static void translateText(OovStringRef const text, std::string &str)
    {
    str = StringMakeXml(text);
    }

OovStatusReturn SvgDrawer::writeFile()
    {
    if(mSuccess.ok())
        {
        mSuccess = mFile.putString("</svg>");
        }
    return mSuccess;
    }

void SvgDrawer::setDiagramSize(GraphSize size)
    {
    mDrawingSize = size;
    mOutputHeader = true;
    }

void SvgDrawer::setCurrentDrawingFontSize(double size)
    {
    mOutputHeader = true;
    DiagramDrawer::setCurrentDrawingFontSize(size);
    }

static void appendColorInt(OovString &str, int rgbColor)
    {
    str.appendInt(rgbColor, 16, 0, 6);
    }

static void outArg(OovStringRef argName, OovStringRef argVal, OovString &outStr)
    {
    outStr += ' ';
    outStr += argName;
    outStr += "=\"";
    outStr += argVal;
    outStr += '\"';
    }

static void outArgInt(OovStringRef argName, int argVal, OovString &outStr)
    {
    OovString argValStr;
    argValStr.appendInt(argVal);
    outArg(argName, argValStr, outStr);
    }

static void outArgFloat(OovStringRef argName, double argVal, int precision, OovString &outStr)
    {
    OovString argValStr;
    argValStr.appendFloat(argVal, precision);
    outArg(argName, argValStr, outStr);
    }

static void outArg4Int(OovStringRef argName, int argVal1, int argVal2,
    int argVal3, int argVal4, OovString &outStr)
    {
    OovString argValStr;
    argValStr.appendInt(argVal1);
    argValStr += " ";
    argValStr.appendInt(argVal2);
    argValStr += " ";
    argValStr.appendInt(argVal3);
    argValStr += " ";
    argValStr.appendInt(argVal4);
    outArg(argName, argValStr, outStr);
    }

void SvgDrawer::maybeOutputHeader()
    {
    if(mOutputHeader && mSuccess.ok())
        {
        const char *fontFamily = "Arial, Helvetica, sans-serif";
        OovString str = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"\n";
        outArg("font-family", fontFamily, str);
        outArgFloat("font-size", getCurrentDrawingFontSize(), 2, str);
        outArgInt("width", mDrawingSize.x, str);
        outArgInt("height", mDrawingSize.y, str);
        outArg4Int("viewbox", 0, 0, mDrawingSize.x, mDrawingSize.y, str);
        str += ">\n";
        mSuccess = mFile.putString(str);
        mOutputHeader = false;
        }
    }

void SvgDrawer::drawRect(const GraphRect &rect)
    {
    maybeOutputHeader();
    if(mSuccess.ok())
        {
        OovString str = "<rect";
        outArgInt("x", rect.start.x, str);
        outArgInt("y", rect.start.y, str);
        outArgInt("width", rect.size.x, str);
        outArgInt("height", rect.size.y, str);
        str += " />\n";
        mSuccess = mFile.putString(str);
        }
    }

void SvgDrawer::drawLine(const GraphPoint &p1, const GraphPoint &p2, bool dashed)
    {
    maybeOutputHeader();
    if(mSuccess.ok())
        {
        OovString str = "<line";
        outArgInt("x1", p1.x, str);
        outArgInt("y1", p1.y, str);
        outArgInt("x2", p2.x, str);
        outArgInt("y2", p2.y, str);
        if(dashed)
            {
            str += " style=\"stroke-dasharray: 4, 4 \"";
            }
        str += " />\n";
        mSuccess = mFile.putString(str);
        }
    }

void SvgDrawer::drawCircle(const GraphPoint &p, int radius, Color fillColor)
    {
    maybeOutputHeader();
    if(mSuccess.ok())
        {
        OovString str = "<circle";
        outArgInt("cx", p.x, str);
        outArgInt("cy", p.y, str);
        outArgInt("r", radius, str);

        OovString style = " style=\"fill:#";
        appendColorInt(style, fillColor.getRGB());
        style += '\"';

        str += style;
        str += " />\n";
        mSuccess = mFile.putString(str);
        }
    }

void SvgDrawer::drawEllipse(const GraphRect &rect)
    {
    int halfX = rect.size.x/2;
    int halfY = rect.size.y/2;
    maybeOutputHeader();
    if(mSuccess.ok())
        {
        OovString str = "<ellipse";
        outArgInt("cx", rect.start.x+halfX, str);
        outArgInt("cy", rect.start.y+halfY, str);
        outArgInt("rx", halfX, str);
        outArgInt("ry", halfY, str);
        str += " />\n";
        mSuccess = mFile.putString(str);
        }
    }

void SvgDrawer::drawPoly(const OovPolygon &poly, Color fillColor)
    {
    std::string pointsStr;
    for(size_t i=0; i<poly.size(); i++)
        {
        char str[10];
        snprintf(str, sizeof(str), "%d,", poly[i].x);
        pointsStr += str;
        snprintf(str, sizeof(str), "%d ", poly[i].y);
        pointsStr += str;
        }
    maybeOutputHeader();
    if(mSuccess.ok())
        {
        OovString str = "<polygon";
        outArg("points", pointsStr, str);

        OovString styleArg = "fill:#";
        appendColorInt(styleArg, fillColor.getRGB());
        addArg(str, "style", styleArg);

        str += " />\n";
        mSuccess = mFile.putString(str);
        }
    }

void SvgDrawer::groupShapes(bool start, Color lineColor, Color fillColor)
    {
    maybeOutputHeader();
    if(mSuccess.ok())
        {
        if(start)
            {
            std::string argStr;
            char temp[40];
            snprintf(temp, sizeof(temp), "#%06x", fillColor.getRGB());
            addArg(argStr, "fill", temp);
            snprintf(temp, sizeof(temp), "#%06x", lineColor.getRGB());
            addArg(argStr, "stroke", temp);
            addArg(argStr, "style", "stroke-width:1");

            OovString str = "<g ";
            str += argStr;
            str += ">\n";
            mSuccess = mFile.putString(str);
            }
        else
            {
            mSuccess = mFile.putString("</g>\n");
            }
        }
    }

void SvgDrawer::groupText(bool start, bool italic)
    {
    maybeOutputHeader();
    if(mSuccess.ok())
        {
        if(start)
            {
            // stroke none means no outline, only fill
            OovString str = "<g stroke=\"none\"";
            if(italic)
                {
                str += " font-style=\"italic\"";
                }
            str += ">\n";
            mSuccess = mFile.putString(str);
            }
        else
            {
            mSuccess = mFile.putString("</g>\n");
            }
        }
    }

void SvgDrawer::drawText(const GraphPoint &p, OovStringRef const text)
    {
    maybeOutputHeader();
    if(mSuccess.ok())
        {
        std::string textStr;
        translateText(text, textStr);

        OovString str = "<text";
        outArgInt("x", p.x, str);
        outArgInt("y", p.y, str);
        str += '>';
        str += textStr;
        str += "</text>\n";
        mSuccess = mFile.putString(str);
        }
    }

float SvgDrawer::getTextExtentWidth(OovStringRef const name) const
    {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, name, &extents);
    return extents.width;
    }

float SvgDrawer::getTextExtentHeight(OovStringRef const name) const
    {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, name, &extents);
    return extents.height;
    }

