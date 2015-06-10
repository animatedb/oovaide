/*
 * PortionDrawer.cpp
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "PortionDrawer.h"
#include "Debug.h"
#include <algorithm>

void PortionDrawer::fillDepths(size_t nodeIndex, std::vector<size_t> &depths) const
    {
    // Use the depths container to figure out if the depth for some index
    // has already been calculated.  This prevents recursion.
    int minDepth = 0;
    if(mGraph->getNodes()[nodeIndex].getNodeType() != PNT_Attribute)
        {
        minDepth = 1;
        }
    if(mGraph->getConnections().size() == 0)
        {
        depths[nodeIndex] = minDepth;
        }
    else
        {
        size_t maxDepth = minDepth;
        for(size_t i=0; i<mGraph->getConnections().size(); i++)
            {
            PortionConnection const &conn = mGraph->getConnections()[i];
            if(nodeIndex == conn.mConsumerNodeIndex)
                {
                size_t supIndex = conn.mSupplierNodeIndex;
                size_t depth = depths[supIndex];
                if(depth == NO_INDEX)
                    {
                    depths[supIndex] = maxDepth;        // Prevent recursion
                    fillDepths(supIndex, depths);
                    depth = depths[supIndex];
                    }
                if(depth > maxDepth)
                    {
                    maxDepth = depth;
                    }
                }
            }
        depths[nodeIndex] = maxDepth + 1;
        }
    }

std::vector<size_t> PortionDrawer::getCallDepths() const
    {
    std::vector<size_t> depths(mGraph->getNodes().size());
    size_t initIndex = NO_INDEX;
    std::fill(depths.begin(), depths.end(), initIndex);
    for(size_t ni=0; ni<mGraph->getNodes().size(); ni++)
        {
        fillDepths(ni, depths);
        }
    return depths;
    }

void PortionDrawer::updateNodePositions()
    {
    if(mDrawer && mGraph)
	{
	if(mGraph->getNodes().size() != mNodePositions.size())
	    {
	    mNodePositions.resize(mGraph->getNodes().size());
	    }
	std::vector<size_t> depths = getCallDepths();
	std::vector<int> columnPositions = getColumnPositions(depths);
	int yOffset = 0;
	const int margin = 20;
	int pad = static_cast<int>(mDrawer->getTextExtentHeight("W"));
	for(size_t i=0; i<mNodePositions.size(); i++)
	    {
	    GraphRect rect = getNodeRect(i);
	    rect.start.y = yOffset;
	    yOffset += pad + rect.size.y;
	    rect.start.y += margin;
	    rect.start.x = margin + columnPositions[depths[i]];
	    mNodePositions[i] = rect.start;
	    }
	}
#define PORTION_GENES 1
#if(PORTION_GENES)
    if(mNodePositions.size() > 0)
	{
        DiagramDependencyGenes genes;
	GraphRect rect = getNodeRect(0);
	genes.initialize(*this, static_cast<size_t>(rect.size.y));
	genes.updatePositionsInDrawer();
	}
#endif
    }

void PortionDrawer::updateGraph(PortionGraph const &graph)
    {
    mGraph = &graph;

    updateNodePositions();
    }

void PortionDrawer::drawGraph()
    {
    if(mDrawer && mGraph)
	{
	// This must be set for svg
	mDrawer->setDiagramSize(getDrawingSize());

        drawNodes();
	mDrawer->groupShapes(true, Color(0,0,0), Color(245,245,255));
	drawConnections();
	mDrawer->groupShapes(false, 0, 0);

	mDrawer->groupText(true);
        drawNodeText();
	mDrawer->groupText(false);
	}
    }

GraphSize PortionDrawer::getDrawingSize() const
    {
    GraphRect graphRect;
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
	{
	graphRect.unionRect(getNodeRect(i));
	}
    // Add some margin.
    graphRect.size.x += 5;
    graphRect.size.y += 5;
    return graphRect.size;
    }

void PortionDrawer::setPosition(size_t nodeIndex, GraphPoint startPoint, GraphPoint newPoint)
    {
    if(nodeIndex != NO_INDEX)
	{
	mNodePositions[nodeIndex].add(newPoint - startPoint);
	}
    }

void PortionDrawer::drawNodes()
    {
    mDrawer->groupShapes(true, Color(0,0,0), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
        {
	if(mGraph->getNodes()[i].getNodeType() == PNT_Attribute)
	    {
	    mDrawer->drawRect(getNodeRect(i));
	    }
	else
	    {
	    mDrawer->drawEllipse(getNodeRect(i));
	    }
        }
    mDrawer->groupShapes(false, 0, 0);
    mDrawer->groupShapes(true, Color(0,0,255), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
	{
	if(mGraph->getNodes()[i].getNodeType() == PNT_NonMemberVariable)
	    {
	    mDrawer->drawRect(getNodeRect(i));
	    }
	}
    mDrawer->groupShapes(false, 0, 0);
    }

void PortionDrawer::drawConnections()
    {
    size_t lastColorIndex = NO_INDEX;
    for(auto const &conn : mGraph->getConnections())
	{
	GraphRect suppRect = getNodeRect(conn.mSupplierNodeIndex);
	GraphRect consRect = getNodeRect(conn.mConsumerNodeIndex);
        GraphPoint suppPoint;
        GraphPoint consPoint;
        mDrawer->getConnectionPoints(consRect, suppRect, consPoint, suppPoint);
	size_t colorIndex = conn.mSupplierNodeIndex;
	if(colorIndex != lastColorIndex)
	    {
	    if(lastColorIndex != NO_INDEX)
		{
		mDrawer->groupShapes(false, 0, 0);
		}
	    Color lineColor = DistinctColors::getColor(colorIndex % DistinctColors::getNumColors());
	    mDrawer->groupShapes(true, lineColor, Color(245,245,255));
	    lastColorIndex = colorIndex;
	    }
        drawArrowDependency(consPoint, suppPoint);
	}
    if(lastColorIndex != NO_INDEX)
	{
	mDrawer->groupShapes(false, 0, 0);
	}
    }

void PortionDrawer::drawNodeText()
    {
    float textHeight = mDrawer->getTextExtentHeight("W");
    int pad = static_cast<int>(textHeight / 7.f * 2);
    int yTextOffset = static_cast<int>(textHeight) + pad;
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
	{
	GraphRect rect = getNodeRect(i);
	rect.start.x += pad;
	rect.start.y += yTextOffset;
	mDrawer->drawText(rect.start, mGraph->getNodes()[i].getName().getStr());
	}
    }

