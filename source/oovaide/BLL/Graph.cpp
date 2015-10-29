/*
 * Graph.cpp
 *
 *  Created on: Jan 6, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */
#include "Graph.h"
#include <math.h>       // for fabs

static double slope(GraphPoint p1, GraphPoint p2)
{
    if(p1.x-p2.x != 0)
        return(static_cast<double>(p1.y-p2.y)/(p1.x-p2.x));
    else
        return 1.0e+9;
}


// From p1, find the intersect with the p2 rect.
static GraphPoint findIntersect(GraphPoint p1, GraphPoint p2, GraphSize p2size)
{
    GraphPoint retP;
    int halfRectWidth = p2size.x/2;
    int halfRectHeight = p2size.y/2;
    double slopeRectCorner = slope(p2, GraphPoint(p2.x+halfRectWidth, p2.y-halfRectHeight));
    double slopeLine = slope(p1, p2);
    bool leftOfRect = (p1.x < p2.x);
    bool aboveRect = (p1.y < p2.y);
    if(fabs(slopeLine) > fabs(slopeRectCorner))
        {
        // Mainly vertical lines
        retP.y = aboveRect ? p2.y-halfRectHeight : p2.y+halfRectHeight;
        int xoff = static_cast<int>(static_cast<float>(slopeRectCorner)/
            slopeLine*halfRectWidth);
        retP.x = aboveRect ? p2.x+xoff : p2.x-xoff;
        }
    else
        {
        // Mainly horizontal lines
        retP.x = leftOfRect ? p2.x-halfRectWidth : p2.x+halfRectWidth;
        int yoff = static_cast<int>(static_cast<float>(slopeLine)/
            slopeRectCorner*halfRectHeight);
        retP.y = leftOfRect ? p2.y+yoff : p2.y-yoff;
        }
    return retP;
}


void GraphRect::findConnectPoints(GraphRect const &rect2, GraphPoint &p1e, GraphPoint &p2e) const
    {
    const GraphRect &rect1 = *this;
    GraphPoint p1(rect1.centerx(), rect1.centery());
    GraphPoint p2(rect2.centerx(), rect2.centery());
    p1e = findIntersect(p2, p1, rect1.size);
    p2e = findIntersect(p1, p2, rect2.size);
    }

void GraphRect::unionRect(GraphRect const &rect2)
    {
    GraphRect &rect1 = *this;
    if(rect2.start.x < rect1.start.x)
        rect1.start.x = rect2.start.x;
    if(rect2.start.y < rect1.start.y)
        rect1.start.y = rect2.start.y;
    if(rect2.endx() > rect1.endx())
        rect1.size.x = rect2.endx() - rect1.start.x;
    if(rect2.endy() > rect1.endy())
        rect1.size.y = rect2.endy() - rect1.start.y;
    }

bool GraphRect::isPointIn(GraphPoint point) const
    {
    return(point.x >= start.x && point.x <= endx() &&
            point.y >= start.y && point.y <= endy());
    }

bool GraphRect::isXOverlapped(GraphRect const &rect) const
    {
    return(isBetween(start.x, rect.start.x, rect.endx()) ||
            isBetween(endx(), rect.start.x, rect.endx()) ||
            isBetween(rect.start.x, start.x, endx()));
    }

GraphRect GraphRect::getZoomed(double zoom) const
    {
    GraphRect rect;
    rect.start = start.getZoomed(zoom);
    rect.size = size.getZoomed(zoom);
    return rect;
    }
