/*
 * ComponentDiagram.cpp
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "ComponentDiagram.h"
#include "ComponentDrawer.h"

void ComponentDiagram::initialize(IncDirDependencyMapReader const &includeMap)
    {
    mComponentGraph.setGraphDataSource(includeMap);
    }

void ComponentDiagram::updateGraph(ComponentDrawOptions const &options,
        DiagramDrawer &nullDrawer)
    {
    mComponentGraph.updateGraph(options);
    updatePositionsInGraph(nullDrawer);
    }

struct NodeVectors
    {
    NodeVectors():
        nodesSizeX(0)
        {}
    void add(ComponentNode *node, int sizeX, int pad)
        {
        nodeVector.push_back(node);
        nodesSizeX += sizeX + pad;
        }
    std::vector<ComponentNode*> nodeVector;
    int nodesSizeX;
    };

void ComponentDiagram::updatePositionsInGraph(DiagramDrawer &nullDrawer)
    {
    ComponentDrawer drawer(*this, nullDrawer);
    int pad = nullDrawer.getPad(1) * 2;

    enum NodeVectorsIndex { NVI_ExtPackage, NVI_Lib, NVI_Exec, NVI_NumVecs };
    NodeVectors nodeVectors[NVI_NumVecs];
    int nodeSpacingY = 0;
    for(auto &node : mComponentGraph.getNodes())
        {
        GraphSize size = drawer.drawNode(node);
        node.setSize(size);
        if(nodeSpacingY == 0)
            nodeSpacingY = size.y * 2;
        if(node.getComponentNodeType() == ComponentNode::CNT_ExternalPackage)
            {
            nodeVectors[NVI_ExtPackage].add(&node, size.x, pad);
            }
        else
            {
            if(node.getComponentType() == CT_StaticLib)
                {
                nodeVectors[NVI_Lib].add(&node, size.x, pad);
                }
            else
                {
                nodeVectors[NVI_Exec].add(&node, size.x, pad);
                }
            }
        }
    int biggestX = 0;
    for(auto const &vec : nodeVectors)
        {
        if(vec.nodesSizeX > biggestX)
            biggestX = vec.nodesSizeX;
        }
    for(size_t veci=0; veci<sizeof(nodeVectors)/sizeof(nodeVectors[0]); veci++)
        {
        int yPos = static_cast<int>(veci) * nodeSpacingY;
        int xPos = (biggestX - static_cast<int>(nodeVectors[veci].nodesSizeX)) / 2;
        for(auto const &node : nodeVectors[veci].nodeVector)
            {
            node->setPos(GraphPoint(xPos, yPos));
            xPos += node->getRect().size.x + pad;
            }
        }
    }

void ComponentDiagram::drawDiagram(DiagramDrawer &diagDrawer)
    {
    ComponentDrawer drawer(*this, diagDrawer);
    drawer.drawDiagram(mComponentGraph);
    }
