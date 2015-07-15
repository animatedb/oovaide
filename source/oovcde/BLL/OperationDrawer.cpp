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
    for(size_t i=0; i<graph.mOpClasses.size(); i++)
        {
        OperationClass &opClass = graph.mOpClasses[i];
        opClass.setPosition(pos);
        size = drawClass(drawer, opClass, options, draw);
        size_t condDepth = graph.getNestDepth(i);
        size.x += static_cast<int>(condDepth) * mPad;
        opClass.setSize(size);
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
        if(drawnOperations.find(oper) == drawnOperations.end())
            {
            size = drawOperation(drawer, pos, *oper, graph, options,
                    drawnOperations, draw);
            pos.y += size.y;
            }
        }
    if(draw)
        {
        drawLifeLines(drawer, graph.mOpClasses, classEndY, pos.y);
        }

    diagramSize.y = pos.y + mCharHeight;
    return diagramSize;
    }

void OperationDrawer::drawLifeLines(DiagramDrawer &drawer,
        const std::vector<OperationClass> &classes,
        std::vector<int> const &classEndY, int endy)
    {
    drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
    endy += mCharHeight;
    for(size_t i=0; i<classes.size(); i++)
        {
        const auto &cl = classes[i];
        int x = cl.getLifelinePosX();
        drawer.drawLine(GraphPoint(x, classEndY[i]), GraphPoint(x, endy));
        }
    drawer.groupShapes(false, Color(0,0,0), Color(245,245,255));
    }

GraphSize OperationDrawer::drawClass(DiagramDrawer &drawer, const OperationClass &node,
        const OperationDrawOptions & /*options*/, bool draw)
    {
    GraphPoint startpos = node.getPosition();
    const ModelType *type = node.getType();
    OovStringRef const typeName = type->getName();
    int rectx = 0;
    int recty = 0;
    const ModelClassifier *classifier = type->getClass();
    if(classifier)
        {
        if(draw)
            {
            drawer.groupText(true);
            }
        OovStringVec strs;
        std::vector<GraphPoint> positions;
        strs.push_back(typeName);
        splitStrings(strs, 30, 40);

        for(auto const &str : strs)
            {
            recty += mCharHeight + mPad;
            positions.push_back(GraphPoint(startpos.x+mPad, startpos.y + recty - mPad));
            int curx = static_cast<int>(drawer.getTextExtentWidth(str)) + mPad*2;
            if(curx > rectx)
                rectx = curx;
            }

        if(draw)
            {
            drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
            drawer.drawRect(GraphRect(startpos.x, startpos.y, rectx, recty));
            drawer.groupShapes(false, Color(0,0,0), Color(245,245,255));

            for(size_t i=0; i<strs.size(); i++)
                {
                drawer.drawText(positions[i], strs[i]);
                }
            drawer.groupText(false);
            }
        }
    return GraphSize(rectx, recty);
    }

class BlockPolygon:public OovPolygon
    {
    public:
        BlockPolygon(int centerLineX, int pad):
            mCenterLineX(centerLineX), mPad(pad)
            {}
        // This only stores points for the right side of the blocks.
        void changeBlockCondDepth(int oldDepth, int newDepth, int y)
            {
            push_back(GraphPoint(mCenterLineX + oldDepth*mPad, y));
            push_back(GraphPoint(mCenterLineX + newDepth*mPad, y));
            }
        // This replicates the right side block to the left side, from bottom to top.
        void finishBlock()
            {
            int rightSize = static_cast<int>(size());
            for(int i=rightSize-1; i>=0; i--)
                {
                push_back(GraphPoint(mCenterLineX - (at(static_cast<size_t>(i)).x -
                    mCenterLineX), at(static_cast<size_t>(i)).y));
                }
            }
        void startChildBlock(int depth, int y)
            {
            push_back(GraphPoint(mCenterLineX + depth*mPad, y));
            push_back(GraphPoint(mCenterLineX, y));
            }
        void endChildBlock(int depth, int y)
            {
            push_back(GraphPoint(mCenterLineX, y));
            push_back(GraphPoint(mCenterLineX + depth*mPad, y));
            }
    private:
        int mCenterLineX;
        int mPad;
    };

static void drawCall(DiagramDrawer &drawer, GraphPoint source,
        GraphPoint target, bool isConst, int arrowLen)
    {
    drawer.drawLine(source, target, isConst);
    // Draw arrow
    if(source.x > target.x)
        arrowLen = -arrowLen;
    drawer.drawLine(GraphPoint(target.x-arrowLen, target.y-arrowLen),
            GraphPoint(target.x, target.y));
    drawer.drawLine(GraphPoint(target.x-arrowLen, target.y+arrowLen),
            GraphPoint(target.x, target.y));
    }

GraphSize OperationDrawer::drawOperationNoText(DiagramDrawer &drawer, GraphPoint pos,
        OperationDefinition &operDef, const OperationGraph &graph,
        const OperationDrawOptions &options,
        std::set<const OperationDefinition*> &drawnOperations,
        std::vector<DrawString> &drawStrings, bool draw)
    {
    GraphPoint startpos = pos;
    int starty = startpos.y+mPad;
    int y=starty;
    size_t sourceIndex = operDef.getOperClassIndex();
    int arrowLen = mCharHeight * 7 / 10;
    int condDepth = 0;
    std::vector<int> condStartPosY;

    drawnOperations.insert(&operDef);
    const OperationClass &cls = graph.getClass(sourceIndex);
    BlockPolygon poly(cls.getLifelinePosX(), mPad);
    if(!graph.isOperCalled(operDef))
        {
        // Show starting operation
        operDef.setRect(GraphPoint(cls.getPosition().x, y),
                GraphSize(mCharHeight*50, mCharHeight+mPad));
        y += mCharHeight;
        drawStrings.push_back(DrawString(GraphPoint(
                cls.getPosition().x, y), operDef.getName()));
        int lineY = y + mPad*2;
        if(draw)
            {
            drawCall(drawer, GraphPoint(cls.getPosition().x, lineY),
                GraphPoint(cls.getLifelinePosX(), lineY),
                operDef.isConst(), arrowLen);
            }
        }
    for(const auto &stmt : operDef.getStatements())
        {
        int condOffset = condDepth * mPad;
        switch(stmt->getStatementType())
            {
            case ST_Call:
                {
                OperationCall *call = stmt->getCall();

                size_t targetIndex = call->getOperClassIndex();
                int lineY = y + mCharHeight + mPad*2;
                int sourcex = cls.getLifelinePosX();
                sourcex += condOffset;
                const OperationClass &targetCls = graph.getClass(targetIndex);
                int targetx = targetCls.getLifelinePosX();
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
                        drawCall(drawer, GraphPoint(sourcex, lineY),
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
                int textX = (sourceIndex < targetIndex) ? cls.getLifelinePosX() :
                        targetCls.getLifelinePosX();
                GraphPoint callPos(textX+condOffset+mPad, y+mCharHeight);
                drawStrings.push_back(DrawString(callPos, call->getName()));
                call->setRect(GraphPoint(callPos.x, callPos.y-mCharHeight),
                        GraphSize(mCharHeight*50, mCharHeight+mPad));
                y += mCharHeight*2;

                // Draw child definition.
                OperationDefinition *childDef = graph.getOperDefinition(*call);
                if(childDef)
                    {
                    poly.startChildBlock(condDepth, y);
                    if(drawnOperations.find(childDef) == drawnOperations.end())
                        {
                        GraphSize childSize = drawOperationNoText(drawer,
                            GraphPoint(pos.x, y), *childDef, graph, options,
                            drawnOperations, drawStrings, draw);
                        y += childSize.y + mPad * 2;
                        }
                    else
                        {
                        GraphRect rect(targetx + mPad*2, callPos.y + mPad*2,
                            mCharHeight+mPad, mCharHeight+mPad);
                        if(draw)
                            {
                            drawer.drawRect(rect);
                            }
                        y += mCharHeight+mPad;
                        }
                    poly.endChildBlock(condDepth, y);
                    }
                }
                break;

            case ST_VarRef:
                {
                OperationVarRef *ref = stmt->getVarRef();

                size_t targetIndex = ref->getOperClassIndex();
                int lineY = y + mCharHeight + mPad*2;
                int sourcex = cls.getLifelinePosX();
                sourcex += condOffset;
                const OperationClass &targetCls = graph.getClass(targetIndex);
                int targetx = targetCls.getLifelinePosX();
                if(targetIndex == NO_INDEX)
                    {
                    int len = mCharHeight*3;
                    if(draw)
                        {
                        drawer.drawLine(GraphPoint(sourcex, lineY),
                            GraphPoint(sourcex+len, lineY), ref->isConst());
                        }
                    }
                else if(targetIndex != sourceIndex)
                    {
                    if(draw)
                        {
                        drawCall(drawer, GraphPoint(sourcex, lineY),
                            GraphPoint(targetx, lineY), ref->isConst(), arrowLen);
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
                            GraphPoint(sourcex+len, lineY), ref->isConst());
                        drawer.drawLine(GraphPoint(sourcex+len, lineY),
                            GraphPoint(sourcex+len, lineY+height));
                        drawer.drawLine(GraphPoint(sourcex, lineY+height),
                            GraphPoint(sourcex+len, lineY+height), ref->isConst());
                        }
                    y += height;
                    }
                int textX = (sourceIndex < targetIndex) ? cls.getLifelinePosX() :
                        targetCls.getLifelinePosX();
                GraphPoint callPos(textX+condOffset+mPad, y+mCharHeight);
                if(draw)
                    {
                    drawStrings.push_back(DrawString(callPos, ref->getName()));
                    }
                ref->setRect(GraphPoint(callPos.x, callPos.y-mCharHeight),
                        GraphSize(mCharHeight*50, mCharHeight+mPad));
                y += mCharHeight*2;
                }
                break;

            case ST_OpenNest:
                {
                GraphPoint lifePos(cls.getLifelinePosX()+condOffset+
                        mPad, y+mCharHeight);
                const OperationNestStart *cond = stmt->getNestStart();
                if(draw)
                    {
                    drawStrings.push_back(DrawString(lifePos, cond->getExpr()));
                    }
                condStartPosY.push_back(y);
                y += mCharHeight*2;

                poly.changeBlockCondDepth(condDepth, condDepth+1, y);
                condDepth++;
                }
                break;

            case ST_CloseNest:
                {
                poly.changeBlockCondDepth(condDepth, condDepth-1, y);
                condDepth--;
                }
                break;
            }
        }
    poly.finishBlock();
    if(draw)
        {
        drawer.drawPoly(poly, Color(245,245,255));
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

        drawer.groupText(true);
        for(size_t i=0; i<drawStrings.size(); i++)
            {
            drawer.drawText(drawStrings[i].pos, drawStrings[i].str);
            }
        drawer.groupText(false);
        }
    return size;
    }
