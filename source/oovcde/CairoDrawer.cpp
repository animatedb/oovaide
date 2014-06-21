/*
 * CairoDrawer.cpp
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "CairoDrawer.h"

float NullDrawer::getTextExtentWidth(char const * const name) const
    {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, name, &extents);
    return extents.width;
    }

float NullDrawer::getTextExtentHeight(char const * const name) const
    {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, name, &extents);
    return extents.height;
    }


void CairoDrawer::setFontSize(double size)
    {
    NullDrawer::setFontSize(size);
    cairo_set_font_size(cr, size);
    }

void CairoDrawer::drawRect(const GraphRect &rect)
    {
    cairo_rectangle(cr, rect.start.x, rect.start.y, rect.size.x, rect.size.y);
    setColor(mFillColor);
    cairo_fill_preserve(cr);
    setColor(mLineColor);
    cairo_stroke(cr);
    }

void CairoDrawer::drawLine(const GraphPoint &p1, const GraphPoint &p2, bool dashed)
    {
    setColor(mLineColor);
    cairo_move_to(cr, p1.x, p1.y);
    cairo_line_to(cr, p2.x, p2.y);
    if(dashed)
	{
	double dashes[] = { 4 };
	cairo_set_dash(cr, dashes, sizeof(dashes)/sizeof(dashes[0]), 1);
	}
    cairo_stroke(cr);
    if(dashed)
	{
	cairo_set_dash(cr, NULL, 0, 0);
	}
    }

void CairoDrawer::drawCircle(const GraphPoint &p, int radius, Color fillColor)
    {
    cairo_arc(cr, p.x, p.y, radius, 0, 2 * M_PI);
    setColor(fillColor);
    cairo_fill_preserve(cr);
    setColor(mLineColor);
    cairo_stroke(cr);
    }

void CairoDrawer::drawPoly(const OovPolygon &poly, Color fillColor)
    {
    if(poly.size() > 0)
	{
	cairo_move_to(cr, poly[0].x, poly[0].y);
	for(size_t i=1; i<poly.size(); i++)
	    {
	    cairo_line_to(cr, poly[i].x, poly[i].y);
	    }
	cairo_close_path(cr);
	setColor(fillColor);
	cairo_fill_preserve(cr);
	setColor(mLineColor);
	cairo_stroke(cr);
	}
    }

void CairoDrawer::drawText(const GraphPoint &p, char const * const text)
    {
    cairo_move_to(cr, p.x, p.y);
    cairo_show_text(cr, text);
    }

void CairoDrawer::groupShapes(bool start, Color fillColor)
    {
    if(start)
	{
	mFillColor = fillColor;
	cairo_set_line_width(cr, 1);
	}
    }

void CairoDrawer::groupText(bool start)
    {
    if(start)
	{
	setColor(mTextColor);
	}
    }
