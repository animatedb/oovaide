/*
 * Svg.h
 *
 *  Created on: Jul 3, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "DiagramDrawer.h"
#include "File.h"
#include "OovError.h"
#include <stdio.h>
#include <cairo.h>

/// Defines functions to write to an SVG file.
class SvgDrawer:public DiagramDrawer
    {
    public:
        SvgDrawer(File &file, cairo_t *c):
            mFile(file), mSuccess(true, SC_File), mFontSize(10.0), cr(c),
            mOutputHeader(true)
            {}
        /// This finishes up writing the file, and indicates if any errors
        /// occurred during writing.
        OovStatusReturn writeFile();

        // These should be called before the drawing functions.
        virtual void setDiagramSize(GraphSize size) override;
        virtual void setFontSize(double size) override;

        virtual void drawRect(const GraphRect &rect) override;
        virtual void drawLine(const GraphPoint &p1, const GraphPoint &p2,
                bool dashed=false) override;
        virtual void drawCircle(const GraphPoint &p, int radius, Color fillColor) override;
        virtual void drawEllipse(const GraphRect &rect) override;
        virtual void drawPoly(const OovPolygon &poly, Color fillColor) override;

        virtual void groupShapes(bool start, Color lineColor, Color fillColor) override;
        virtual void groupText(bool start, bool italics) override;
        virtual void drawText(const GraphPoint &p, OovStringRef const text) override;

        virtual float getTextExtentWidth(OovStringRef const name) const override;
        virtual float getTextExtentHeight(OovStringRef const name) const override;

    private:
        File &mFile;
        OovStatus mSuccess;
        double mFontSize;
        cairo_t *cr;
        GraphSize mDrawingSize;
        bool mOutputHeader;

        void maybeOutputHeader();
    };
