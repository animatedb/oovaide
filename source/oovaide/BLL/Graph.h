/*
 * Graph.h
 *
 *  Created on: Jan 6, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef GRAPH_H_
#define GRAPH_H_

#include <stdint.h>     // For uint..._t

#ifndef M_PI
#define M_PI 3.14159265
#endif

inline bool isBetween(int ref, int v1, int v2)
    { return ref >= v1 && ref <= v2; }

class Color
    {
    public:
        Color(uint8_t r=0, uint8_t g=0, uint8_t b=0)
            {
            red = r;
            green = g;
            blue = b;
            }
        void set(uint8_t r, uint8_t g, uint8_t b)
            {
            red = r;
            green = g;
            blue = b;
            }
        uint32_t getRGB() const
            { return((static_cast<uint32_t>(red) << 16) +
                    (static_cast<uint32_t>(green) << 8) + blue);}
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

class GraphPoint
    {
    public:
        GraphPoint():
            x(0), y(0)
            {}
        GraphPoint(int px, int py):
            x(px), y(py)
            {}
        GraphPoint abs() const;
        void set(int px, int py)
            { x=px; y=py; }
        // Same as +=
        void add(const GraphPoint &p)
            { x+=p.x; y+=p.y; }
        void sub(const GraphPoint &p)
            { x-=p.x; y-=p.y; }
        GraphPoint operator+(const GraphPoint &p) const
            { return GraphPoint(x + p.x, y + p.y); }
        void clear()
            { x=0; y=0; }
        GraphPoint getZoomed(double zoom) const
            {
            return GraphPoint(static_cast<int>(x * zoom),
                static_cast<int>(y * zoom));
            }
        GraphPoint operator-(GraphPoint const &p)
            {
            GraphPoint result(x, y);
            result.sub(p);
            return result;
            }
        // Define an arbitrary convention so that insertion into sets can
        // be performed.
        bool operator<(GraphPoint const &rhs) const
            {
            return(y < rhs.y || (y == rhs.y && x < rhs.x));
            }
        bool operator==(GraphPoint const &rhs) const
            {
            return(y == rhs.y && x == rhs.x);
            }

    public:
        int x;
        int y;
    };

typedef GraphPoint GraphSize;

class GraphLine
    {
    public:
        GraphLine()
            {}
        GraphLine(int startx, int starty, int endx, int endy):
            start(startx, starty), end(endx, endy)
            {}
        GraphLine(GraphPoint startPoint, GraphPoint endPoint):
            start(startPoint), end(endPoint)
            {}
        // Define an arbitrary convention so that insertion into sets can
        // be performed.
        bool operator<(GraphLine const &rhs) const
            {
            return(start < rhs.start || (start == rhs.start && end < rhs.end));
            }
        GraphPoint start;
        GraphPoint end;
    };

class GraphRect
    {
    public:
        GraphRect()
            {}
        GraphRect(GraphPoint const p1, GraphPoint const p2);
        GraphRect(int x, int y, int xsize, int ysize)
            {
            start.x = x;
            start.y = y;
            size.x = xsize;
            size.y = ysize;
            }
        int endx() const
            { return start.x+size.x; }
        int endy() const
            { return start.y+size.y; }
        int centerx() const
            { return start.x+(size.x/2); }
        int centery() const
            { return start.y+(size.y/2); }
        bool isPointIn(GraphPoint point) const;
        bool isXOverlapped(GraphRect const &rect) const;
        GraphPoint getCenter() const
            { return GraphPoint(start.x+size.x/2, start.y+size.y/2); }
        // Given two rectangles, find the points at the edges of the rectangles
        // if a line were drawn to the centers of both rectangles.
        void findConnectPoints(GraphRect const &rect2, GraphPoint &p1e,
                GraphPoint &p2e) const;
        void unionRect(GraphRect const &rect2);
        GraphRect getZoomed(double zoom) const;

    public:
        GraphPoint start;
        GraphSize size;
    };


#endif /* GRAPH_H_ */
