/*
 * Svg.h
 *
 *  Created on: Jul 3, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "DiagramDrawer.h"
#include "File.h"
#include <stdio.h>
#include <cairo.h>

/// Defines functions to write to an SVG file.
class SvgDrawer:public DiagramDrawer
    {
    public:
	SvgDrawer(FILE *fp, cairo_t *c):
	    mFp(fp), mFontSize(10.0), cr(c)
	    {}
	virtual void setDiagramSize(GraphSize size);
	virtual void setFontSize(double size);
	virtual void drawRect(const GraphRect &rect);
	virtual void drawLine(const GraphPoint &p1, const GraphPoint &p2,
		bool dashed=false);
	virtual void drawCircle(const GraphPoint &p, int radius, Color fillColor);
	virtual void drawPoly(const OovPolygon &poly, Color fillColor);
	virtual void groupShapes(bool start, Color fillColor);
	virtual void groupText(bool start);
	virtual void drawText(const GraphPoint &p, char const * const text);
	virtual float getTextExtentWidth(char const * const name) const;
	virtual float getTextExtentHeight(char const * const name) const;
    private:
	FILE *mFp;
	double mFontSize;
	cairo_t *cr;
    };

/// Opens a file to write as SVG.
class SvgWriter
    {
    public:
	SvgWriter(char const * const fn);
	~SvgWriter();
	FILE *getFile()
	    { return mFile.getFp(); }

    private:
	File mFile;
    };
