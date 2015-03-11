/*
 * ComponentDrawer.cpp
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */
#include "ComponentDrawer.h"
#include <math.h>	// for atan

GraphSize ComponentDrawer::drawNode(const ComponentNode &node)
    {
    GraphPoint startpos = node.getRect().start;
    int pad = mDrawer.getPad();
    std::vector<std::string> drawStrings;

    drawStrings.push_back(node.getName());
    ComponentTypesFile::eCompTypes ct = node.getComponentType();
    if(node.getComponentNodeType() == ComponentNode::CNT_ExternalPackage)
	{
	drawStrings.push_back("<<External>>");
	}
    else if(ct != ComponentTypesFile::CT_Unknown)
	{
	std::string stereotype = "<<";
	stereotype += ComponentTypesFile::getShortComponentTypeName(ct);
	stereotype += ">>";
	drawStrings.push_back(stereotype);
	}

    int rectHeight = mDrawer.getTextExtentHeight("W") * 4;
    int symbolSize = rectHeight / 5;

    int maxWidth = 0;
    for(const auto &str : drawStrings)
	{
	int width = mDrawer.getTextExtentWidth(str) + (symbolSize * 3);
	if(width > maxWidth)
	    maxWidth = width;
	}

    mDrawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
    mDrawer.drawRect(GraphRect(startpos.x+symbolSize, startpos.y, maxWidth-symbolSize, rectHeight));
    mDrawer.drawRect(GraphRect(startpos.x, startpos.y+symbolSize, symbolSize*2, symbolSize));
    mDrawer.drawRect(GraphRect(startpos.x, startpos.y+symbolSize*3, symbolSize*2, symbolSize));
    mDrawer.groupShapes(false, Color(0,0,0), Color(245,245,255));

    mDrawer.groupText(true);
    int y = startpos.y;
    for(const auto &str : drawStrings)
	{
	y += mDrawer.getTextExtentHeight("W") + symbolSize;
	// Drawing text is started from the bottom left corner
	mDrawer.drawText(GraphPoint(startpos.x+symbolSize*2+pad*2, y), str);
	}
    mDrawer.groupText(false);

    return GraphSize(maxWidth, rectHeight);
    }

void ComponentDrawer::drawDiagram(const ComponentGraph &graph)
    {
    if(graph.getNodes().size() > 0)
	{
	mDrawer.setDiagramSize(graph.getGraphSize());
	for(auto const &node : graph.getNodes())
	    {
	    drawNode(node);
	    }

	for(auto const &connect : graph.getConnections())
	    {
	    if(!connect.getImpliedDependency())
		{
		drawConnection(graph, connect);
		}
	    }
	}
    }

void ComponentDrawer::drawConnection(const ComponentGraph &graph,
	ComponentConnection const &connect)
    {
    mDrawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
    GraphPoint producer;
    GraphPoint consumer;
    graph.getNodes()[connect.mNodeConsumer].getRect().findConnectPoints(
	    graph.getNodes()[connect.mNodeSupplier].getRect(), consumer, producer);
    mDrawer.drawLine(producer, consumer, true);

    int xdist = producer.x-consumer.x;
    int ydist = producer.y-consumer.y;
    double lineAngleRadians;
    if(ydist != 0)
        lineAngleRadians = atan2(xdist, ydist);
    else
        {
        if(producer.x > consumer.x)
            lineAngleRadians = M_PI/2;
        else
            lineAngleRadians = -M_PI/2;
        }
    int arrowtop = -10;
    const double triangleAngle = (2 * M_PI) / arrowtop;
    GraphPoint p2;
    // calc left point of symbol
    p2.set(sin(lineAngleRadians-triangleAngle) * arrowtop,
	    cos(lineAngleRadians-triangleAngle) * arrowtop);
    mDrawer.drawLine(producer, p2+producer);
    // calc right point of symbol
    p2.set(sin(lineAngleRadians+triangleAngle) * arrowtop,
	cos(lineAngleRadians+triangleAngle) * arrowtop);
    mDrawer.drawLine(producer, p2+producer);

    mDrawer.groupShapes(false, Color(0,0,0), Color(245,245,255));
    }
