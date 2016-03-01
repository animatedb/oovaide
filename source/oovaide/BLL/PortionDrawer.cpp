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

void PortionDrawer::updateNodePositions(DiagramDrawer &drawer)
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
#define PORTION_GENES 1
#if(PORTION_GENES)
    if(mNodePositions.size() > 0)
        {
        DiagramDependencyGenes genes;
        GraphRect rect = getNodeRect(drawer, 0);
        genes.initialize(*this, static_cast<size_t>(rect.size.y));
        genes.updatePositionsInDrawer();
        }
#endif
    }

void PortionDrawer::drawGraph(DiagramDrawer &drawer)
    {
    if(mGraph)
        {
        // This must be set for svg
        drawer.setDiagramSize(getDrawingSize(drawer));
        drawer.setCurrentDrawingFontSize(mDiagram.getDiagramBaseFontSize());

        drawNodes(drawer);
        drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
        drawConnections(drawer);
        drawer.groupShapes(false, 0, 0);

        std::vector<DrawString> drawStrings;
        std::vector<bool> virtOpers;
        getNodeText(drawer, drawStrings, virtOpers);

        drawer.groupText(true, false);
        drawNodeText(drawer, false, drawStrings, virtOpers);
        drawer.groupText(false, false);

        drawer.groupText(true, true);
        drawNodeText(drawer, true, drawStrings, virtOpers);
        drawer.groupText(false, false);
        }
    }

GraphSize PortionDrawer::getDrawingSize(DiagramDrawer &drawer) const
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

void PortionDrawer::setPosition(size_t nodeIndex, GraphPoint startPoint, GraphPoint newPoint)
    {
    if(nodeIndex != NO_INDEX)
        {
        mNodePositions[nodeIndex].add(newPoint - startPoint);
        }
    }

void PortionDrawer::setPosition(size_t nodeIndex, GraphPoint newPoint)
    {
    if(nodeIndex != NO_INDEX)
        {
        mNodePositions[nodeIndex] = newPoint;
        }
    }

void PortionDrawer::drawNodes(DiagramDrawer &drawer)
    {
    drawer.groupShapes(true, Color(255,0,0), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
        {
        if(std::find(mSelectedNodeIndices.begin(), mSelectedNodeIndices.end(),
                i) != mSelectedNodeIndices.end())
            {
            GraphRect rect = getNodeRect(drawer, i);
            rect.start.x--;
            rect.start.y--;
            rect.size.x+=2;
            rect.size.y+=2;
            drawer.drawRect(rect);
            }
        }
    drawer.groupShapes(false, 0, 0);

    drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
        {
        if(mGraph->getNodes()[i].getNodeType() == PNT_Attribute)
            {
            drawer.drawRect(getNodeRect(drawer, i));
            }
        else
            {
            drawer.drawEllipse(getNodeRect(drawer, i));
            }
        }
    drawer.groupShapes(false, 0, 0);
    drawer.groupShapes(true, Color(0,0,255), Color(245,245,255));
    for(size_t i=0; i<mNodePositions.size(); i++)
        {
        if(mGraph->getNodes()[i].getNodeType() == PNT_NonMemberVariable)
            {
            drawer.drawRect(getNodeRect(drawer, i));
            }
        }
    drawer.groupShapes(false, 0, 0);
    }

void PortionDrawer::drawConnections(DiagramDrawer &drawer)
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

void PortionDrawer::getNodeText(DiagramDrawer &drawer,
    std::vector<DrawString> &drawStrings, std::vector<bool> &virtOpers)
    {
    float textHeight = drawer.getTextExtentHeight("W");
    int pad = static_cast<int>(textHeight / 7.f * 2);
    int yTextOffset = static_cast<int>(textHeight) + pad;
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
        {
        GraphRect rect = getNodeRect(drawer, i);
        rect.start.x += pad;
        rect.start.y += yTextOffset;
        PortionNode const &node = mGraph->getNodes()[i];
        drawStrings.push_back(DrawString(rect.start, node.getDisplayName().getStr()));
        virtOpers.push_back(node.isVirtualOperation());
        }
    }

void PortionDrawer::drawNodeText(DiagramDrawer &drawer, bool drawVirts,
    std::vector<DrawString> const &drawStrings, std::vector<bool> const &virtOpers)
    {
    for(size_t i=0; i<drawStrings.size(); i++)
        {
        if(drawVirts == virtOpers[i])
            {
            drawer.drawText(drawStrings[i].pos, drawStrings[i].str);
            }
        }
    }

bool PortionDrawer::isSelected(size_t nodeIndex) const
    {
    return (std::find(mSelectedNodeIndices.begin(), mSelectedNodeIndices.end(),
        nodeIndex) != mSelectedNodeIndices.end());
    }

void PortionDrawer::setSingleSelection(size_t nodeIndex)
    {
    mSelectedNodeIndices.clear();
    mSelectedNodeIndices.push_back(nodeIndex);
    }

// If no nodes are contained in the rectangle, this clears the selection.
void PortionDrawer::setRectSelection(GraphPoint p1, GraphPoint p2)
    {
    mSelectedNodeIndices.clear();
    GraphRect selRect(p1, p2);
    for(size_t i=0; i<mNodePositions.size(); i++)
        {
        if(selRect.isPointIn(mNodePositions[i]))
            {
            mSelectedNodeIndices.push_back(i);
            }
        }
    }

void PortionDrawer::moveSelection(GraphPoint p)
    {
    for(auto const &nodeIndex : mSelectedNodeIndices)
        {
        mNodePositions[nodeIndex] = mNodePositions[nodeIndex] + p;
        }
    }

std::vector<int> PortionDrawer::getSelectedNodeIndicesSortedX() const
    {
    std::vector<int> sortedNodes;
    sortedNodes = mSelectedNodeIndices;
    std::sort(sortedNodes.begin(), sortedNodes.end(),
        [this](int n1, int n2)
        { return(mNodePositions[n1].x < mNodePositions[n2].x); });
    return sortedNodes;
    }

std::vector<int> PortionDrawer::getSelectedNodeIndicesSortedY() const
    {
    std::vector<int> sortedNodes;
    sortedNodes = mSelectedNodeIndices;
    std::sort(sortedNodes.begin(), sortedNodes.end(),
        [this](int n1, int n2)
        { return(mNodePositions[n1].y < mNodePositions[n2].y); });
    return sortedNodes;
    }

void PortionDrawer::alignTopSelection()
    {
    if(mSelectedNodeIndices.size() > 0)
        {
        std::vector<int> sortedSelectedNodes = getSelectedNodeIndicesSortedX();
        int y = mNodePositions[sortedSelectedNodes[0]].y;
        for(auto const &nodeIndex : mSelectedNodeIndices)
            {
            mNodePositions[nodeIndex].y = y;
            }
        }
    }

void PortionDrawer::alignLeftSelection()
    {
    if(mSelectedNodeIndices.size() > 0)
        {
        std::vector<int> sortedSelectedNodes = getSelectedNodeIndicesSortedY();
        int x = mNodePositions[sortedSelectedNodes[0]].x;
        for(auto const &nodeIndex : mSelectedNodeIndices)
            {
            mNodePositions[nodeIndex].x = x;
            }
        }
    }

void PortionDrawer::spaceEvenlyDownSelection()
    {
    if(mSelectedNodeIndices.size() > 2)
        {
        std::vector<int> sortedSelectedNodes = getSelectedNodeIndicesSortedY();
        int topY = mNodePositions[sortedSelectedNodes[0]].y;
        int botY = mNodePositions[sortedSelectedNodes[sortedSelectedNodes.size()-1]].y;
        float nodeDist = static_cast<float>(botY - topY) / (sortedSelectedNodes.size()-1);
        // No need to reposition first or last selected nodes
        for(size_t i=1; i<sortedSelectedNodes.size()-1; i++)
            {
            mNodePositions[sortedSelectedNodes[i]].y = static_cast<int>(i*nodeDist) + topY;
            }
        }
    }

