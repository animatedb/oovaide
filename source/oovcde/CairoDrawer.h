/*
 * CairoDrawer.h
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CAIRODRAWER_H_
#define CAIRODRAWER_H_

#include "ClassDrawer.h"

/// Defines a basic context for drawing that can be used for screen or
/// printing to files.
class GtkCairoContext
    {
    public:
	GtkCairoContext(GtkWidget *widget)
	    { mCr = gdk_cairo_create(gtk_widget_get_window(widget));  }
	~GtkCairoContext()
	    { cairo_destroy(mCr); }
	cairo_t *getCairo()
	    { return mCr; }
	// Pad is currently defined as 1/10 of font size.
	GraphSize getPad();

    private:
	cairo_t *mCr;
    };

/// Provides a null interface for drawing. This allows drawing to a null device so
/// that calculations can be performed for object sizes based on fonts.
class NullDrawer:public DiagramDrawer
    {
    public:
	NullDrawer(cairo_t *c):
	    cr(c)
	    {}
	bool haveCr() const
	    { return(cr != nullptr); }
	virtual void setDiagramSize(GraphSize size)
	    {}
	virtual void drawRect(const GraphRect &rect)
	    {}
	virtual void drawLine(const GraphPoint &p1, const GraphPoint &p2,
		bool dashed=false)
	    {}
	virtual void drawCircle(const GraphPoint &p, int radius, Color fillColor)
	    {}
	virtual void drawPoly(const OovPolygon &poly, Color fillColor)
	    {}
	virtual void drawText(const GraphPoint &p, char const * const text)
	    {}
	virtual float getTextExtentWidth(char const * const name) const;
	virtual float getTextExtentHeight(char const * const name) const;
    protected:
	cairo_t *cr;
    };

/// This is an interface for drawing using cairo functions.
class CairoDrawer:public NullDrawer
    {
    public:
	CairoDrawer(cairo_t *c):
	    NullDrawer(c)
	    {}
	virtual void setDiagramSize(GraphSize size)
	    {}
	virtual void setFontSize(double size);
	virtual void drawRect(const GraphRect &rect);
	virtual void drawLine(const GraphPoint &p1, const GraphPoint &p2,
		bool dashed=false);
	virtual void drawCircle(const GraphPoint &p, int radius, Color fillColor);
	virtual void drawPoly(const OovPolygon &poly, Color fillColor);
	virtual void drawText(const GraphPoint &p, char const * const text);
	virtual void groupShapes(bool start, Color fillColor);
	virtual void groupText(bool start);

	void setColor(Color c)
	    { cairo_set_source_rgb(cr, c.red/255.0, c.blue/255.0, c.green/255.0); }
    private:
	Color mFillColor;
	Color mLineColor;
	Color mTextColor;
    };


#endif /* CAIRODRAWER_H_ */
