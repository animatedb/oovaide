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

SvgDrawer::~SvgDrawer()
    {
    if(mFp)
        {
        fprintf(mFp, "</svg>");
        }
    }

void SvgDrawer::setDiagramSize(GraphSize size)
    {
    mDrawingSize = size;
    mOutputHeader = true;
    }

void SvgDrawer::setFontSize(double size)
    {
    mOutputHeader = true;
    DiagramDrawer::setFontSize(size);
    mFontSize = size;
    }

void SvgDrawer::maybeOutputHeader()
    {
    if(mOutputHeader)
        {
        const char *fontFamily = "Arial, Helvetica, sans-serif";
        fprintf(mFp, "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"\n"
                "  font-family=\"%s\" font-size=\"%f\" \n"
                "  width=\"%d\" height=\"%d\">\n",
                fontFamily, mFontSize, mDrawingSize.x, mDrawingSize.y);
        mOutputHeader = false;
        }
    }

void SvgDrawer::drawRect(const GraphRect &rect)
    {
    maybeOutputHeader();
    fprintf(mFp, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
            rect.start.x, rect.start.y, rect.size.x, rect.size.y);
    }

void SvgDrawer::drawLine(const GraphPoint &p1, const GraphPoint &p2, bool dashed)
    {
    char const * style = "";
    if(dashed)
        {
        style = "style=\"stroke-dasharray: 4, 4 \"";
        }
    maybeOutputHeader();
    fprintf(mFp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" %s/>\n",
            p1.x, p1.y, p2.x, p2.y, style);
    }

void SvgDrawer::drawCircle(const GraphPoint &p, int radius, Color fillColor)
    {
    maybeOutputHeader();
    fprintf(mFp, "<circle cx=\"%d\" cy=\"%d\" r=\"%d\" style=\"fill:#%06x\" />\n",
            p.x, p.y, radius, fillColor.getRGB());
    }

void SvgDrawer::drawEllipse(const GraphRect &rect)
    {
    int halfX = rect.size.x/2;
    int halfY = rect.size.y/2;
    maybeOutputHeader();
    fprintf(mFp, "<ellipse cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
            rect.start.x+halfX, rect.start.y+halfY, halfX, halfY);
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
    fprintf(mFp, "<polygon points=\"%s\" style=\"fill:#%06x\" />\n",
            pointsStr.c_str(), fillColor.getRGB());
    }

void SvgDrawer::groupShapes(bool start, Color lineColor, Color fillColor)
    {
    maybeOutputHeader();
    if(start)
        {
        std::string argStr;
        char temp[40];
        snprintf(temp, sizeof(temp), "#%06x", fillColor.getRGB());
        addArg(argStr, "fill", temp);
        snprintf(temp, sizeof(temp), "#%06x", lineColor.getRGB());
        addArg(argStr, "stroke", temp);
        addArg(argStr, "style", "stroke-width:1");
        fprintf(mFp, "<g %s>\n", argStr.c_str());
        }
    else
        {
        fprintf(mFp, "</g>\n");
        }
    }

void SvgDrawer::groupText(bool start)
    {
    maybeOutputHeader();
        // stroke none means no outline, only fill
    if(start)
        fprintf(mFp, "<g stroke=\"none\">\n");
    else
        fprintf(mFp, "</g>\n");
    }

void SvgDrawer::drawText(const GraphPoint &p, OovStringRef const text)
    {
    maybeOutputHeader();
    std::string str;
    translateText(text, str);
    fprintf(mFp, "<text x=\"%d\" y=\"%d\">%s</text>\n",
            p.x, p.y, str.c_str());
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

