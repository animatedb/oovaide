/*
 * IncludeDrawer.cpp
 * Created on: June 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "IncludeDrawer.h"


void IncludeDrawer::fillDepths(size_t nodeIndex, std::vector<size_t> &depths) const
    {
    // Use the depths container to figure out if the depth for some index
    // has already been calculated.  This prevents recursion.
    if(mGraph->getConnections().size() == 0)
        depths[nodeIndex] = 0;
    else
        {
        size_t maxDepth = 0;
        for(size_t i=0; i<mGraph->getConnections().size(); i++)
            {
            IncludeConnection const &conn = mGraph->getConnections()[i];
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

std::vector<size_t> IncludeDrawer::getCallDepths() const
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

std::vector<int> IncludeDrawer::getColumnPositions(std::vector<size_t> const &depths) const
    {
    // Add 1 so there is a spacing column one beyond the last column.
    std::vector<int> columnSpacing(depths.size() + 1);
    int pad = static_cast<int>(mDrawer->getTextExtentWidth("W"));
    // first get the width required for each column.
    for(size_t ni=0; ni<mNodePositions.size(); ni++)
	{
	int &columnWidth = columnSpacing[depths[ni]];
	int nodeWidth = static_cast<int>(getNodeRect(ni).size.x + pad * 5);
	if(nodeWidth > columnWidth)
	    {
	    columnWidth = nodeWidth;
	    }
	}
    // Now convert to positions.
    int pos = 0;
    for(size_t ci=0; ci<columnSpacing.size(); ci++)
	{
	int space = columnSpacing[ci];
	columnSpacing[ci] = pos;
	pos += space;
	}
    return columnSpacing;
    }

void IncludeDrawer::updateNodePositions()
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
    if(mNodePositions.size() > 0)
	{
        DiagramDependencyGenes genes;
	GraphRect rect = getNodeRect(0);
	genes.initialize(*this, static_cast<size_t>(rect.size.y));
	genes.updatePositionsInDrawer();
	}
    }

void IncludeDrawer::updateGraph(IncludeGraph const &graph)
    {
    mGraph = &graph;

    updateNodePositions();
    }

void IncludeDrawer::drawGraph()
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

GraphSize IncludeDrawer::getDrawingSize() const
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

void IncludeDrawer::setPosition(size_t nodeIndex, GraphPoint startPoint, GraphPoint newPoint)
    {
    if(nodeIndex != NO_INDEX)
	{
	mNodePositions[nodeIndex].add(newPoint - startPoint);
	}
    }

void IncludeDrawer::drawNodes()
    {
    mDrawer->groupShapes(true, Color(0,0,0), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
        {
        mDrawer->drawRect(getNodeRect(i));
        }
    mDrawer->groupShapes(false, 0, 0);
    mDrawer->groupShapes(true, Color(0,0,255), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
	{
        mDrawer->drawRect(getNodeRect(i));
	}
    mDrawer->groupShapes(false, 0, 0);
    }

void IncludeDrawer::drawConnections()
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

void IncludeDrawer::drawNodeText()
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

