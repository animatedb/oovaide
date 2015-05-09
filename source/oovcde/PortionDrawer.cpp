/*
 * PortionDrawer.cpp
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "PortionDrawer.h"
#include "PortionGenes.h"


bool PortionDrawer::attemptTofillDepth(size_t nodeIndex, std::vector<int> &depths) const
    {
    int depth = 0;
    bool resolved = true;
    for(size_t i=0; i<mGraph->getConnections().size(); i++)
	{
	PortionConnection const &conn = mGraph->getConnections()[i];
	if(nodeIndex == conn.mConsumerNodeIndex)
	    {
	    if(mGraph->getNodes()[conn.mSupplierNodeIndex].getNodeType() == PNT_Operation)
		{
		int opDepth = depths[conn.mSupplierNodeIndex];
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

void PortionDrawer::findCallDepths(std::vector<int> &depths) const
    {
    bool resolved = false;
    /// @todo - need max nesting depth to limit loop time
    for(int i=0; i<100 && !resolved; i++)
	{
	for(size_t i=0; i<mGraph->getNodes().size(); i++)
	    {
	    if(depths[i] == 0 &&
		    mGraph->getNodes()[i].getNodeType() == PNT_Operation)
		{
		if(!attemptTofillDepth(i, depths))
		    {
		    resolved = false;
		    }
		}
	    }
	}
    }

void PortionDrawer::updateNodePositions()
    {
    if(mDrawer && mGraph)
	{
	if(mGraph->getNodes().size() != mNodePositions.size())
	    {
	    mNodePositions.resize(mGraph->getNodes().size());
	    }
	std::vector<int> depths;
	depths.resize(mGraph->getNodes().size());
	findCallDepths(depths);

	const int margin = 20;
	int pad = mDrawer->getTextExtentHeight("W");
	int xColumnSpacing = 0;
	for(size_t i=0; i<mNodePositions.size(); i++)
	    {
	    if(getNodeRect(i).size.x > xColumnSpacing)
		{
		xColumnSpacing = getNodeRect(i).size.x;
		}
	    }
	xColumnSpacing = xColumnSpacing * 1.5;
	int yOffset = 0;
	for(size_t i=0; i<mNodePositions.size(); i++)
	    {
	    GraphRect rect = getNodeRect(i);
	    rect.start.y = yOffset;
	    yOffset += pad + rect.size.y;
	    rect.start.y += margin;
	    rect.start.x = margin + xColumnSpacing * depths[i];
	    mNodePositions[i] = rect.start;
	    }
	}
#define PORTION_GENES 1
#if(PORTION_GENES)
    if(mNodePositions.size() > 0)
	{
	PortionGenes genes;
	GraphRect rect = getNodeRect(0);
	genes.initialize(*this, rect.size.y);
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

GraphRect PortionDrawer::getNodeRect(int nodeIndex) const
    {
    GraphPoint nodePos = mNodePositions[nodeIndex];
    OovString name = mGraph->getNodes()[nodeIndex].getName();
    int textWidth = mDrawer->getTextExtentWidth(name);
    float textHeight = mDrawer->getTextExtentHeight(name);
    /// @todo - fix pad here and in drawNodeText
    int pad2 = (textHeight / 7.f) * 4;
    return GraphRect(nodePos.x, nodePos.y, textWidth + pad2, textHeight + pad2);
    }

size_t PortionDrawer::getNodeIndex(GraphPoint p)
    {
    size_t nodeIndex = -1;
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
    if(nodeIndex != -1)
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
    int lastColorIndex = -1;
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
	int colorIndex = conn.mSupplierNodeIndex;
	if(colorIndex != lastColorIndex)
	    {
	    if(lastColorIndex != -1)
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
    if(lastColorIndex != -1)
	{
	mDrawer->groupShapes(false, 0, 0);
	}
    }

void PortionDrawer::drawNodeText()
    {
    float textHeight = mDrawer->getTextExtentHeight("W");
    int pad = (textHeight / 7.f * 2);
    int yTextOffset = textHeight + pad;
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
	{
	GraphRect rect = getNodeRect(i);
	rect.start.x += pad;
	rect.start.y += yTextOffset;
	mDrawer->drawText(rect.start, mGraph->getNodes()[i].getName().getStr());
	}
    }

