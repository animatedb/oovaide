/*
 * DiagramDrawer.h
 *
 *  Created on: Jul 3, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef DIAGRAMDRAWER_H_
#define DIAGRAMDRAWER_H_

#include <vector>
#include "Graph.h"


typedef std::vector<GraphPoint> OovPolygon;

class DiagramDrawer
    {
    public:
	virtual ~DiagramDrawer()
	    {}
	/// This must be called before any other function.
	virtual void setDiagramSize(GraphSize size) = 0;
	virtual void drawRect(const GraphRect &rect)=0;
	virtual void drawLine(const GraphPoint &p1, const GraphPoint &p2,
		bool dashed=false)=0;
	virtual void drawCircle(const GraphPoint &p, int radius, Color fillColor) = 0;
	virtual void drawPoly(const OovPolygon &poly, Color fillColor)=0;
	virtual void drawText(const GraphPoint &p, char const * const text)=0;
	virtual int getTextExtentWidth(char const * const name) const =0;
	virtual int getTextExtentHeight(char const * const name) const =0;
	/// These functions indicate when the start/end of some grouping
	/// of shapes is performed so similar attributes can be used.
	virtual void groupShapes(bool start, Color fillColor)
	    {}
	virtual void groupText(bool start)
	    {}
	int getPad(int div=10);
    };

void viewSource(char const * const module, int lineNum);

#endif /* DIAGRAMDRAWER_H_ */
