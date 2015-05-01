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
	virtual void setDiagramSize(GraphSize size) override;
	virtual void setFontSize(double size) override;
	virtual void drawRect(const GraphRect &rect) override;
	virtual void drawLine(const GraphPoint &p1, const GraphPoint &p2,
		bool dashed=false) override;
	virtual void drawCircle(const GraphPoint &p, int radius, Color fillColor) override;
	virtual void drawEllipse(const GraphRect &rect) override;
	virtual void drawPoly(const OovPolygon &poly, Color fillColor) override;
	virtual void groupShapes(bool start, Color lineColor, Color fillColor) override;
	virtual void groupText(bool start) override;
	virtual void drawText(const GraphPoint &p, OovStringRef const text) override;
	virtual float getTextExtentWidth(OovStringRef const name) const override;
	virtual float getTextExtentHeight(OovStringRef const name) const override;
    private:
	FILE *mFp;
	double mFontSize;
	cairo_t *cr;
    };

/// Opens a file to write as SVG.
class SvgWriter
    {
    public:
	SvgWriter(OovStringRef const fn);
	~SvgWriter();
	FILE *getFile()
	    { return mFile.getFp(); }

    private:
	File mFile;
    };
