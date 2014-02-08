/*
 * Svg.h
 *
 *  Created on: Jul 3, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "DiagramDrawer.h"
#include <stdio.h>
#include <cairo.h>

class SvgDrawer:public DiagramDrawer
    {
    public:
	SvgDrawer(FILE *fp, cairo_t *c):
	    mFp(fp), cr(c)
	    {}
	virtual void setDiagramSize(GraphSize size);
	virtual void drawRect(const GraphRect &rect);
	virtual void drawLine(const GraphPoint &p1, const GraphPoint &p2,
		bool dashed=false);
	virtual void drawCircle(const GraphPoint &p, int radius, Color fillColor);
	virtual void drawPoly(const OovPolygon &poly, Color fillColor);
	virtual void groupShapes(bool start, Color fillColor);
	virtual void groupText(bool start);
	virtual void drawText(const GraphPoint &p, char const * const text);
	virtual int getTextExtentWidth(char const * const name) const;
	virtual int getTextExtentHeight(char const * const name) const;
    private:
	FILE *mFp;
	cairo_t *cr;
    };

class SvgWriter
    {
    public:
	SvgWriter(char const * const fn);
	~SvgWriter();
	FILE *getFile()
	    { return mFp; }

    private:
	FILE *mFp;
    };
