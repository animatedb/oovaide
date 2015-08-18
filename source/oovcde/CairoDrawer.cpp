/*
 * CairoDrawer.cpp
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "CairoDrawer.h"

float NullDrawer::getTextExtentWidth(OovStringRef const name) const
    {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, name, &extents);
    return extents.width;
    }

float NullDrawer::getTextExtentHeight(OovStringRef const name) const
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
    setFillAndLine(cr, mFillColor, mLineColor);
    }

void CairoDrawer::setFillAndLine(cairo_t *cr, Color fillColor, Color lineColor)
    {
    setColor(fillColor);
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
    setFillAndLine(cr, fillColor, mLineColor);
    }

void CairoDrawer::drawEllipse(const GraphRect &rect)
    {
    cairo_save(cr);
    int halfX = rect.size.x / 2.0;
    int halfY = rect.size.y / 2.0;
    cairo_translate (cr, rect.start.x + halfX, rect.start.y + halfY);
    cairo_scale(cr, halfX, halfY);
    cairo_arc(cr, 0., 0., 1., 0., 2 * M_PI);
    cairo_restore(cr);
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
        setFillAndLine(cr, fillColor, mLineColor);
        }
    }

void CairoDrawer::drawText(const GraphPoint &p, OovStringRef const text)
    {
    cairo_move_to(cr, p.x, p.y);
    cairo_show_text(cr, text);
    }

void CairoDrawer::groupShapes(bool start, Color lineColor, Color fillColor)
    {
    if(start)
        {
        mFillColor = fillColor;
        mLineColor = lineColor;
        cairo_set_line_width(cr, 1);
        }
    }

class CairoFont
    {
    public:
        CairoFont(cairo_t *cr)
        {
        mFace = cairo_get_font_face(cr);
        mFamily = cairo_toy_font_face_get_family(mFace);
        mSlant = cairo_toy_font_face_get_slant(mFace);
        mWeight = cairo_toy_font_face_get_weight(mFace);
        }
    /// Keep the original family and weight, but alter the slant.
    void setItalicsFont(cairo_t *cr, bool italics)
        {
        cairo_font_slant_t slant;
        if(italics)
            {
            slant = CAIRO_FONT_SLANT_ITALIC;
            }
        else
            {
            slant = CAIRO_FONT_SLANT_NORMAL;
            }
        cairo_select_font_face(cr, mFamily, slant, mWeight);
        }

    private:
        const char *mFamily;
        cairo_font_face_t *mFace;
        cairo_font_slant_t mSlant;
        cairo_font_weight_t mWeight;
    };

void CairoDrawer::groupText(bool start, bool italics)
    {
    CairoFont font(cr);
    if(start)
        {
        setColor(mTextColor);
        font.setItalicsFont(cr, italics);
        }
    }

void CairoDrawer::clearAndSetDefaults()
    {
    cairo_set_source_rgb(cr, 255,255,255);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 0,0,0);
    cairo_set_line_width(cr, 1.0);
    }
