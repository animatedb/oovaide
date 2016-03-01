/*
 * OperationDrawer.cpp
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "OperationDrawer.h"


GraphSize OperationDrawer::getDrawingSize(DiagramDrawer &drawer,
    OperationGraph &graph, const OperationDrawOptions &options)
    {
    return drawOrSizeDiagram(drawer, graph, options, false);
    }

GraphSize OperationDrawer::drawDiagram(DiagramDrawer &drawer,
        OperationGraph &graph, const OperationDrawOptions &options)
    {
    drawer.setDiagramSize(getDrawingSize(drawer, graph, options));
    drawer.setCurrentDrawingFontSize(mDiagram.getDiagramBaseFontSize());
    return drawOrSizeDiagram(drawer, graph, options, true);
    }

GraphSize OperationDrawer::drawOrSizeDiagram(DiagramDrawer &drawer,
        OperationGraph &graph, const OperationDrawOptions &options, bool draw)
    {
    mCharHeight = static_cast<int>(drawer.getTextExtentHeight("W"));
    int pad = mCharHeight / 3;
    if(pad < 1)
        pad = 1;
    mPad = pad;

    GraphPoint startpos(mCharHeight, mCharHeight);
    GraphPoint pos = startpos;
    GraphSize size;
    GraphSize diagramSize;
    int maxy = 0;
    std::vector<int> classEndY;
    for(size_t i=0; i<graph.mNodes.size(); i++)
        {
        OperationNode &node = graph.getNode(i);
        node.setPosition(pos);
        size = drawNode(drawer, node, options, draw);
        size_t condDepth = graph.getNestDepth(i);
        size.x += static_cast<int>(condDepth) * mPad;
        node.setSize(size);
        pos.x += size.x + mCharHeight;
        classEndY.push_back(startpos.y + size.y);
        if(size.y > maxy)
            maxy = size.y;
        }
    diagramSize.x = pos.x;

    pos = startpos;
    pos.y = startpos.y + maxy;  // space between class rect and operations
    std::set<const OperationDefinition*> drawnOperations;
    for(const auto &oper : graph.mOperations)
        {
        if(drawnOperations.find(oper.get()) == drawnOperations.end())
            {
            size = drawOperation(drawer, pos, *oper, graph, options,
                    drawnOperations, draw);
            pos.y += size.y;
            }
        }
    if(draw)
        {
        drawLifeLines(drawer, graph.mNodes, classEndY, pos.y);
        }

    diagramSize.y = pos.y + mCharHeight;
    return diagramSize;
    }

void OperationDrawer::drawLifeLines(DiagramDrawer &drawer,
        std::vector<std::unique_ptr<OperationNode>> const &nodes,
        std::vector<int> const &classEndY, int endy)
    {
    drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
    endy += mCharHeight;
    for(size_t i=0; i<nodes.size(); i++)
        {
        int x = nodes[i]->getLifelinePosX();
        drawer.drawLine(GraphPoint(x, classEndY[i]), GraphPoint(x, endy));
        }
    drawer.groupShapes(false, Color(0,0,0), Color(245,245,255));
    }

GraphSize OperationDrawer::drawNode(DiagramDrawer &drawer, const OperationNode &node,
        const OperationDrawOptions & /*options*/, bool draw)
    {
    GraphPoint startpos = node.getPosition();
    int rectx = 0;
    int recty = 0;
    OovStringVec strs;
    std::vector<GraphPoint> positions;

    OovString const typeName = node.getName();
    strs.push_back(typeName);

    splitStrings(strs, 30, 40);
    for(auto const &str : strs)
        {
        recty += mCharHeight + (mPad * 2);
        positions.push_back(GraphPoint(startpos.x+mPad, startpos.y + recty - mPad));
        int curx = static_cast<int>(drawer.getTextExtentWidth(str)) + mPad*2;
        if(curx > rectx)
            rectx = curx;
        }
/*
    if(node.getNodeType() == NT_Class)
        {
        const ModelType *type = OperationClass::getClass(&node)->getType();
        const ModelClassifier *classifier = ModelClassifier::getClass(type);
        if(classifier)
            {
            }
        }
*/
    if(draw)
        {
        if(node.getNodeType() == NT_Class)
            {
            drawer.groupShapes(true, Color(0,0,0), Color(245,255,245));
            }
        else
            {
            drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
            }
        drawer.drawRect(GraphRect(startpos.x, startpos.y, rectx, recty));
        drawer.groupShapes(false, Color(0,0,0), Color(245,245,255));

        drawer.groupText(true, false);
        for(size_t i=0; i<strs.size(); i++)
            {
            drawer.drawText(positions[i], strs[i]);
            }
        drawer.groupText(false, false);
        }
    return GraphSize(rectx, recty);
    }

// This only stores points for the right side of the blocks.
void BlockPolygon::incDepth(int y)
    {
    push_back(GraphPoint(mCenterLineX + mDepth*mPad, y));
    push_back(GraphPoint(mCenterLineX + ++mDepth*mPad, y));
    }

// This only stores points for the right side of the blocks.
void BlockPolygon::decDepth(int y)
    {
    push_back(GraphPoint(mCenterLineX + mDepth*mPad, y));
    push_back(GraphPoint(mCenterLineX + --mDepth*mPad, y));
    }

// This replicates the right side block to the left side, from bottom to top.
void BlockPolygon::finishBlock()
    {
    int rightSize = static_cast<int>(size());
    for(int i=rightSize-1; i>=0; i--)
        {
        push_back(GraphPoint(mCenterLineX - (at(static_cast<size_t>(i)).x -
            mCenterLineX), at(static_cast<size_t>(i)).y));
        }
    }
/*
void BlockPolygon::startChildBlock(int depth, int y)
    {
    push_back(GraphPoint(mCenterLineX + depth*mPad, y));
    push_back(GraphPoint(mCenterLineX, y));
    }

void BlockPolygon::endChildBlock(int depth, int y)
    {
    push_back(GraphPoint(mCenterLineX, y));
    push_back(GraphPoint(mCenterLineX + depth*mPad, y));
    }
*/

static void drawConnection(DiagramDrawer &drawer, GraphPoint source,
        GraphPoint target, bool isConst, int arrowLen)
    {
    drawer.drawLine(source, target, isConst);
    if(arrowLen > 0)
        {
        if(source.x > target.x)
            arrowLen = -arrowLen;
        drawer.drawLine(GraphPoint(target.x-arrowLen, target.y-arrowLen),
                GraphPoint(target.x, target.y));
        drawer.drawLine(GraphPoint(target.x-arrowLen, target.y+arrowLen),
                GraphPoint(target.x, target.y));
        }
    }

void OperationDrawer::drawCall(DiagramDrawer &drawer,
    const OperationGraph &graph, size_t sourceIndex, OperationCall *call,
    std::vector<DrawString> &drawStrings, int condOffset, int &y,
    const OperationDrawOptions &options,
    std::set<const OperationDefinition*> &drawnOperations, bool draw, int callDepth)
    {
    size_t targetIndex = graph.getNodeIndex(call->getDestNode());
    int arrowLen = mCharHeight * 7 / 10;
    int lineY = y + mCharHeight + mPad*2;
    OperationNode const &node = graph.getNode(sourceIndex);
    int sourcex = node.getLifelinePosX();
    sourcex += condOffset;
    const OperationNode &targetNode = graph.getNode(targetIndex);
    int targetx = targetNode.getLifelinePosX();
    if(targetIndex == NO_INDEX)
        {
        // Handle [else]
//                  int len = mCharHeight*3;
//                  mDrawer.drawLine(GraphPoint(sourcex, lineY),
//                          GraphPoint(sourcex+len, lineY), call->isConst());
        }
    else if(targetIndex != sourceIndex)
        {
        if(draw)
            {
            drawConnection(drawer, GraphPoint(sourcex, lineY),
                GraphPoint(targetx, lineY), call->isConst(), arrowLen);
            }
        }
    else
        {
        // Draw line back to same class
        int len = mCharHeight*3;
        int height = 3;
        if(draw)
            {
            drawer.drawLine(GraphPoint(sourcex, lineY),
                GraphPoint(sourcex+len, lineY), call->isConst());
            drawer.drawLine(GraphPoint(sourcex+len, lineY),
                GraphPoint(sourcex+len, lineY+height));
            drawer.drawLine(GraphPoint(sourcex, lineY+height),
                GraphPoint(sourcex+len, lineY+height), call->isConst());
            }
        y += height;
        }
    int textX = (sourceIndex < targetIndex) ? node.getLifelinePosX() :
            targetNode.getLifelinePosX();
    GraphPoint callPos(textX+condOffset+mPad, y+mCharHeight);
    drawStrings.push_back(DrawString(callPos, call->getName()));
    call->setRect(GraphPoint(callPos.x, callPos.y-mCharHeight),
            GraphSize(mCharHeight*50, mCharHeight+mPad));
    y += mCharHeight*2;

    // Draw child definition.
    OperationDefinition *childDef = graph.getOperDefinition(*call);
    if(childDef)
        {
        BlockPolygon &poly = mLifelinePolygons[sourceIndex];
        //                    poly.startChildBlock(condDepth, y);
        if(drawnOperations.find(childDef) == drawnOperations.end())
            {
            poly.incDepth(y);
int posX = targetNode.getLifelinePosX();
            GraphSize childSize = drawOperationNoText(drawer,
                GraphPoint(posX/*pos.x*/, y), *childDef, graph, options,
                drawnOperations, drawStrings, draw, callDepth+1);
            y += childSize.y + mPad * 2;
            poly.decDepth(y);
            }
        else
            {
            // This draws a rectangle to signify that it is defined
            // elsewhere in the diagram.
            GraphRect rect(targetx + mPad*2, callPos.y + mPad*2,
                mCharHeight+mPad, mCharHeight+mPad);
            if(draw)
                {
                drawer.drawRect(rect);
                }
            y += mCharHeight+mPad;
            }
//                    poly.endChildBlock(condDepth, y);
        }
    }

GraphSize OperationDrawer::drawOperationNoText(DiagramDrawer &drawer, GraphPoint pos,
        OperationDefinition &operDef, const OperationGraph &graph,
        const OperationDrawOptions &options,
        std::set<const OperationDefinition*> &drawnOperations,
        std::vector<DrawString> &drawStrings, bool draw, int callDepth)
    {
    GraphPoint startpos = pos;
    // Add space between bottom of class and operation name
    int starty = startpos.y+(mPad*2);
    int y=starty;
    size_t sourceIndex = graph.getNodeIndex(operDef.getDestNode());
    std::vector<int> condStartPosY;

    drawnOperations.insert(&operDef);
    OperationNode const &node = graph.getNode(sourceIndex);
    if(node.getNodeType() == NT_Class)
        {
        if(callDepth == 0)
            {
            // Reinit all polys to initial conditions.
            mLifelinePolygons.clear();
            mLifelinePolygons.resize(graph.getNodes().size());
            }
        BlockPolygon &poly = mLifelinePolygons[sourceIndex];
        poly.setup(node.getLifelinePosX(), mPad);

        if(!graph.isOperCalled(operDef))
            {
            // Show starting operation
            operDef.setRect(GraphPoint(node.getPosition().x, y),
                    GraphSize(mCharHeight*50, mCharHeight+mPad));
            y += mCharHeight;
            drawStrings.push_back(DrawString(GraphPoint(
                    node.getPosition().x, y), operDef.getName()));
            // Add space between operation name and called operations.
            y += (mPad * 2);
            int lineY = y + mPad*2;
            if(draw)
                {
                int arrowLen = mCharHeight * 7 / 10;
                drawConnection(drawer, GraphPoint(node.getPosition().x, lineY),
                    GraphPoint(node.getLifelinePosX(), lineY),
                    operDef.isConst(), arrowLen);
                }
            }
        for(const auto &stmt : operDef.getStatements())
            {
            int condOffset = poly.getDepth() * mPad;
            switch(stmt->getStatementType())
                {
                case ST_Call:
                    {
                    OperationCall *call = stmt->getCall();
                    drawCall(drawer, graph, sourceIndex, call, drawStrings,
                        condOffset, y, options, drawnOperations, draw, callDepth);
                    }
                    break;

                case ST_VarRef:
                    {
                    OperationVarRef *ref = stmt->getVarRef();

                    size_t targetIndex = graph.getNodeIndex(ref->getDestNode());
                    int lineY = y + mCharHeight + mPad*2;
                    int sourcex = node.getLifelinePosX();
                    sourcex += condOffset;
                    const OperationNode &targetNode = graph.getNode(targetIndex);
                    int targetx = targetNode.getLifelinePosX();
                    if(targetIndex == NO_INDEX)
                        {
                        int len = mCharHeight*3;
                        if(draw)
                            {
                            drawer.drawLine(GraphPoint(sourcex, lineY),
                                GraphPoint(sourcex+len, lineY), ref->isConst());
                            }
                        }
                    else
                        {
                        if(draw)
                            {
                            drawConnection(drawer, GraphPoint(sourcex, lineY),
                                GraphPoint(targetx, lineY), ref->isConst(), 0);
                            }
                        }
                    int textX = (sourceIndex < targetIndex) ? node.getLifelinePosX() :
                            targetNode.getLifelinePosX();
                    GraphPoint callPos(textX+condOffset+mPad, y+mCharHeight);
                    ref->setRect(GraphPoint(callPos.x, callPos.y-mCharHeight),
                            GraphSize(mCharHeight*50, mCharHeight+mPad));
                    y += mCharHeight*2;
                    }
                    break;

                case ST_OpenNest:
                    {
                    GraphPoint lifePos(node.getLifelinePosX()+condOffset+
                            mPad, y+mCharHeight);
                    const OperationNestStart *cond = stmt->getNestStart();
                    if(draw)
                        {
                        drawStrings.push_back(DrawString(lifePos, cond->getExpr()));
                        }
                    condStartPosY.push_back(y);
                    y += mCharHeight*2;

                    poly.incDepth(y);
                    }
                    break;

                case ST_CloseNest:
                    {
                    poly.decDepth(y);
                    }
                    break;
                }
            }
        if(callDepth == 0)
            {
            for(auto &poly : mLifelinePolygons)
                {
                poly.finishBlock();
                if(draw)
                    {
                    drawer.drawPoly(poly, Color(245,245,255));
                    }
                }
            }
        }

    return GraphSize(0, y - startpos.y);
    }

GraphSize OperationDrawer::drawOperation(DiagramDrawer &drawer, GraphPoint pos,
        OperationDefinition &operDef, const OperationGraph &graph,
        const OperationDrawOptions &options,
        std::set<const OperationDefinition*> &drawnOperations, bool draw)
    {
    std::vector<DrawString> drawStrings;

    if(draw)
        {
        drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
        }
    GraphSize size = drawOperationNoText(drawer, pos, operDef, graph, options,
            drawnOperations, drawStrings, draw);
    if(draw)
        {
        drawer.groupShapes(false, Color(0,0,0), Color(245,245,255));

        drawer.groupText(true, false);
        for(size_t i=0; i<drawStrings.size(); i++)
            {
            drawer.drawText(drawStrings[i].pos, drawStrings[i].str);
            }
        drawer.groupText(false, false);
        }
    return size;
    }
