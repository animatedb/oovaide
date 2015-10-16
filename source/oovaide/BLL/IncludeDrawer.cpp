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

std::vector<int> IncludeDrawer::getColumnPositions(DiagramDrawer &drawer,
        std::vector<size_t> const &depths) const
    {
    // Add 1 so there is a spacing column one beyond the last column.
    std::vector<int> columnSpacing(depths.size() + 1);
    int pad = static_cast<int>(drawer.getTextExtentWidth("W"));
    // first get the width required for each column.
    for(size_t ni=0; ni<mNodePositions.size(); ni++)
        {
        int &columnWidth = columnSpacing[depths[ni]];
        int nodeWidth = static_cast<int>(getNodeRect(drawer, ni).size.x + pad * 5);
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

void IncludeDrawer::updateNodePositions(DiagramDrawer &drawer)
    {
    if(mGraph)
        {
        if(mGraph->getNodes().size() != mNodePositions.size())
            {
            mNodePositions.resize(mGraph->getNodes().size());
            }
        std::vector<size_t> depths = getCallDepths();
        std::vector<int> columnPositions = getColumnPositions(drawer, depths);
        int yOffset = 0;
        const int margin = 20;
        int pad = static_cast<int>(drawer.getTextExtentHeight("W"));
        for(size_t i=0; i<mNodePositions.size(); i++)
            {
            GraphRect rect = getNodeRect(drawer, i);
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
        GraphRect rect = getNodeRect(drawer, 0);
        genes.initialize(*this, static_cast<size_t>(rect.size.y));
        genes.updatePositionsInDrawer();
        }
    }

void IncludeDrawer::updateGraph(DiagramDrawer &drawer, IncludeGraph const &graph)
    {
    mGraph = &graph;

    updateNodePositions(drawer);
    }

void IncludeDrawer::drawGraph(DiagramDrawer &drawer)
    {
    if(mGraph)
        {
        // This must be set for svg
        drawer.setDiagramSize(getDrawingSize(drawer));

        drawNodes(drawer);
        drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
        drawConnections(drawer);
        drawer.groupShapes(false, 0, 0);

        drawer.groupText(true, false);
        drawNodeText(drawer);
        drawer.groupText(false, false);
        }
    }

GraphSize IncludeDrawer::getDrawingSize(DiagramDrawer &drawer) const
    {
    GraphRect graphRect;
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
        {
        graphRect.unionRect(getNodeRect(drawer, i));
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

void IncludeDrawer::drawNodes(DiagramDrawer &drawer)
    {
    drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
        {
        drawer.drawRect(getNodeRect(drawer, i));
        }
    drawer.groupShapes(false, 0, 0);
    drawer.groupShapes(true, Color(0,0,255), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
        {
        drawer.drawRect(getNodeRect(drawer, i));
        }
    drawer.groupShapes(false, 0, 0);
    }

void IncludeDrawer::drawConnections(DiagramDrawer &drawer)
    {
    size_t lastColorIndex = NO_INDEX;
    for(auto const &conn : mGraph->getConnections())
        {
        GraphRect suppRect = getNodeRect(drawer, conn.mSupplierNodeIndex);
        GraphRect consRect = getNodeRect(drawer, conn.mConsumerNodeIndex);
        GraphPoint suppPoint;
        GraphPoint consPoint;
        drawer.getConnectionPoints(consRect, suppRect, consPoint, suppPoint);
        size_t colorIndex = conn.mSupplierNodeIndex;
        if(colorIndex != lastColorIndex)
            {
            if(lastColorIndex != NO_INDEX)
                {
                drawer.groupShapes(false, 0, 0);
                }
            Color lineColor = DistinctColors::getColor(colorIndex % DistinctColors::getNumColors());
            drawer.groupShapes(true, lineColor, Color(245,245,255));
            lastColorIndex = colorIndex;
            }
        drawArrowDependency(drawer, consPoint, suppPoint);
        }
    if(lastColorIndex != NO_INDEX)
        {
        drawer.groupShapes(false, 0, 0);
        }
    }

void IncludeDrawer::drawNodeText(DiagramDrawer &drawer)
    {
    float textHeight = drawer.getTextExtentHeight("W");
    int pad = static_cast<int>(textHeight / 7.f * 2);
    int yTextOffset = static_cast<int>(textHeight) + pad;
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
        {
        GraphRect rect = getNodeRect(drawer, i);
        rect.start.x += pad;
        rect.start.y += yTextOffset;
        drawer.drawText(rect.start, mGraph->getNodes()[i].getName().getStr());
        }
    }

