/*
 * DiagramDrawer.h
 *
 *  Created on: Jul 3, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef DIAGRAMDRAWER_H_
#define DIAGRAMDRAWER_H_

#include <vector>
#include "OovString.h"
#include "Graph.h"


typedef std::vector<GraphPoint> OovPolygon;
void splitStrings(OovStringVec &strs, size_t desiredLength);

class DiagramArrow
    {
    public:
	DiagramArrow(GraphPoint producer, GraphPoint consumer, int arrowSize);
	GraphPoint getLeftArrowPosition()
	    { return mLeftPos; }
	GraphPoint getRightArrowPosition()
	    { return mRightPos; }
    private:
	GraphPoint mLeftPos;
	GraphPoint mRightPos;
    };

/// Provides an abstract interface for drawing. This interface can be used for
/// drawing to screen, files (svg) or to a null device.
class DiagramDrawer
    {
    public:
	DiagramDrawer():
	    mFontSize(10.0)
	    {}
	virtual ~DiagramDrawer();
	/// This must be called before any other function.
	virtual void setDiagramSize(GraphSize size) = 0;
	virtual void setFontSize(double size)
	    { mFontSize = size; }
	double getFontSize() const
	    { return mFontSize; }
	virtual void drawRect(const GraphRect &rect)=0;
	virtual void drawLine(const GraphPoint &p1, const GraphPoint &p2,
		bool dashed=false)=0;
	virtual void drawCircle(const GraphPoint &p, int radius, Color fillColor) = 0;
	virtual void drawEllipse(const GraphRect &rect) = 0;
	virtual void drawPoly(const OovPolygon &poly, Color fillColor)=0;
	// The point specifies the bottom left corner of the text.
	virtual void drawText(const GraphPoint &p, OovStringRef const text)=0;
	virtual float getTextExtentWidth(OovStringRef const name) const =0;
	virtual float getTextExtentHeight(OovStringRef const name) const =0;
	/// These functions indicate when the start/end of some grouping
	/// of shapes is performed so similar attributes can be used.
	virtual void groupShapes(bool /*start*/, Color /*lineColor*/,
            Color /*fillColor*/)
	    {}
	virtual void groupText(bool /*start*/)
	    {}
	int getPad(int div=10);

    private:
	double mFontSize;
    };

class DistinctColors
    {
    public:
	static size_t getNumColors();
	static Color getColor(size_t index);
    };

// This launches an editor to view source code at a certain line number in
// the source code.
void viewSource(OovStringRef const module, unsigned int lineNum);

#endif /* DIAGRAMDRAWER_H_ */
