/*
 * PortionDrawer.cpp
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "PortionDrawer.h"
#include "PortionGenes.h"


bool PortionDrawer::fillDepths(size_t nodeIndex, std::vector<size_t> &depths) const
    {
    size_t depth = 0;
    bool resolved = true;
    for(size_t i=0; i<mGraph->getConnections().size(); i++)
	{
	PortionConnection const &conn = mGraph->getConnections()[i];
	if(nodeIndex == conn.mConsumerNodeIndex)
	    {
	    size_t supIndex = conn.mSupplierNodeIndex;
		if(mGraph->getNodes()[supIndex].getNodeType() == PNT_Operation)
		    {
		    size_t opDepth = depths[supIndex];
		    if(opDepth > 0)
			{
			if(opDepth > depth)
			    {
			    depth = opDepth;
			    }
			}
		    else
			{
			resolved = false;
			break;
			}
		    }
		}
	    }
    if(resolved)
	{
	depths[nodeIndex] = depth + 1;
	}
    return resolved;
    }

std::vector<size_t> PortionDrawer::getCallDepths() const
    {
    std::vector<size_t> depths(mGraph->getNodes().size());
    bool resolved = false;
    /// @todo - need max nesting depth to limit loop time
    for(int limi=0; limi<100 && !resolved; limi++)
	{
	for(size_t ni=0; ni<mGraph->getNodes().size(); ni++)
	    {
	    if(depths[ni] == 0 &&
		    mGraph->getNodes()[ni].getNodeType() == PNT_Operation)
		{
		if(!fillDepths(ni, depths))
		    {
		    resolved = false;
		    }
		}
	    }
	}
    return depths;
    }

std::vector<int> PortionDrawer::getColumnPositions(std::vector<size_t> const &depths) const
    {
    std::vector<int> columnSpacing(depths.size());
    // first get the width required for each column.
    for(size_t ni=0; ni<mNodePositions.size(); ni++)
	{
	int &columnWidth = columnSpacing[depths[ni]];
	int nodeWidth = static_cast<int>(getNodeRect(ni).size.x * 1.5);
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
	PortionGenes genes;
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

GraphRect PortionDrawer::getNodeRect(size_t nodeIndex) const
    {
    GraphPoint nodePos = mNodePositions[nodeIndex];
    OovString name = mGraph->getNodes()[nodeIndex].getName();
    int textWidth = static_cast<int>(mDrawer->getTextExtentWidth(name));
    float textHeight = mDrawer->getTextExtentHeight(name);
    /// @todo - fix pad here and in drawNodeText
    int pad2 = static_cast<int>((textHeight / 7.f) * 4);
    return GraphRect(nodePos.x, nodePos.y, textWidth + pad2,
        static_cast<int>(textHeight) + pad2);
    }

size_t PortionDrawer::getNodeIndex(GraphPoint p)
    {
    size_t nodeIndex = NO_INDEX;
    if(mGraph)
	{
	for(size_t i=0; i<mGraph->getNodes().size(); i++)
	    {
	    GraphRect rect = getNodeRect(i);
	    if(rect.isPointIn(p))
		{
		nodeIndex = i;
		break;
		}
	    }
	}
    return nodeIndex;
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
#if(NonMemberVariables)
    mDrawer->groupShapes(true, Color(0,0,255), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
	{
	if(mGraph->getNodes()[i].getNodeType() == PNT_NonMemberVariable)
	    {
	    mDrawer->drawRect(getNodeRect(i));
	    }
	}
    mDrawer->groupShapes(false, 0, 0);
#endif
    }

void PortionDrawer::drawConnections()
    {
    size_t lastColorIndex = NO_INDEX;
    for(auto const &conn : mGraph->getConnections())
	{
	GraphRect suppRect = getNodeRect(conn.mSupplierNodeIndex);
	GraphRect consRect = getNodeRect(conn.mConsumerNodeIndex);
	GraphPoint suppPoint = suppRect.getCenter();
	GraphPoint consPoint = consRect.getCenter();
	if(abs(suppPoint.x - consPoint.x) > abs(suppPoint.y - consPoint.y))
	    {
	    // Horizontal distance is larger, so place connection at ends
	    if(suppPoint.x > consPoint.x)
		{
		consPoint.x += consRect.size.x/2;
		suppPoint.x -= suppRect.size.x/2;
		}
	    else
		{
		consPoint.x -= consRect.size.x/2;
		suppPoint.x += suppRect.size.x/2;
		}
	    }
	else
	    {
	    // Place connection at top or bottom
	    if(suppPoint.y > consPoint.y)
		{
		consPoint.y += consRect.size.y/2;
		suppPoint.y -= suppRect.size.y/2;
		}
	    else
		{
		consPoint.y -= consRect.size.y/2;
		suppPoint.y += suppRect.size.y/2;
		}
	    }
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
	mDrawer->drawLine(suppPoint, consPoint);
	DiagramArrow arrow(suppPoint, consPoint, 10);
	mDrawer->drawLine(suppPoint, suppPoint + arrow.getLeftArrowPosition());
	mDrawer->drawLine(suppPoint, suppPoint + arrow.getRightArrowPosition());
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

