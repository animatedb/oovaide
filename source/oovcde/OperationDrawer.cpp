/*
 * OperationDrawer.cpp
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "OperationDrawer.h"


void OperationDrawer::setDiagramSize(GraphSize size)
    {
    mDrawer.setDiagramSize(size);
    }

GraphSize OperationDrawer::drawDiagram(OperationGraph &graph,
	const OperationDrawOptions &options)
    {
    mCharHeight = mDrawer.getTextExtentHeight("W");
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
	size = drawClass(opClass, options);
	int condDepth = graph.getNestDepth(i);
	size.x += condDepth * mPad;
	opClass.setSize(size);
	pos.x += size.x + mCharHeight;
	classEndY.push_back(startpos.y + size.y);
	if(size.y > maxy)
	    maxy = size.y;
	}
    diagramSize.x = pos.x;

    pos = startpos;
    pos.y = startpos.y + maxy;	// space between class rect and operations
    std::set<const OperationDefinition*> drawnOperations;
    for(const auto &oper : graph.mOperations)
	{
	if(drawnOperations.find(oper) == drawnOperations.end())
	    {
	    size = drawOperation(pos, *oper, graph, options, drawnOperations);
	    pos.y += size.y;
	    }
	}
    drawLifeLines(graph.mOpClasses, classEndY, pos.y);

    diagramSize.y = pos.y + mCharHeight;
    return diagramSize;
    }

void OperationDrawer::drawLifeLines(const std::vector<OperationClass> &classes,
	std::vector<int> const &classEndY, int endy)
    {
    mDrawer.groupShapes(true, Color(245,245,255));
    endy += mCharHeight;
    for(size_t i=0; i<classes.size(); i++)
	{
	const auto &cl = classes[i];
	int x = cl.getLifelinePosX();
	mDrawer.drawLine(GraphPoint(x, classEndY[i]), GraphPoint(x, endy));
	}
    mDrawer.groupShapes(false, Color(245,245,255));
    }

GraphSize OperationDrawer::drawClass(const OperationClass &node,
	const OperationDrawOptions &options)
    {
    GraphPoint startpos = node.getPosition();
    const ModelType *type = node.getType();
    char const * const typeName = type->getName().c_str();
    int rectx = 0;
    int recty = 0;
    const ModelClassifier *classifier = type->getClass();
    if(classifier)
	{
	mDrawer.groupText(true);
	std::vector<std::string> strs;
	std::vector<GraphPoint> positions;
	strs.push_back(typeName);
	splitStrings(strs, 30);

	for(auto const &str : strs)
	    {
	    recty += mCharHeight + mPad;
	    positions.push_back(GraphPoint(startpos.x+mPad, startpos.y + recty - mPad));
	    int curx = mDrawer.getTextExtentWidth(str.c_str()) + mPad*2;
	    if(curx > rectx)
		rectx = curx;
	    }

	mDrawer.groupShapes(true, Color(245,245,255));
	mDrawer.drawRect(GraphRect(startpos.x, startpos.y, rectx, recty));
	mDrawer.groupShapes(false, Color(245,245,255));

	for(size_t i=0; i<strs.size(); i++)
	    {
	    mDrawer.drawText(positions[i], strs[i].c_str());
	    }
	mDrawer.groupText(false);
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
		push_back(GraphPoint(mCenterLineX - (at(i).x - mCenterLineX), at(i).y));
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

GraphSize OperationDrawer::drawOperationNoText(GraphPoint pos,
	OperationDefinition &operDef, const OperationGraph &graph,
	const OperationDrawOptions &options,
	std::set<const OperationDefinition*> &drawnOperations,
	std::vector<DrawString> &drawStrings)
    {
    GraphPoint startpos = pos;
    int starty = startpos.y+mPad;
    int y=starty;
    int sourceIndex = operDef.getOperClassIndex();
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
	drawCall(mDrawer, GraphPoint(cls.getPosition().x, lineY),
		GraphPoint(cls.getLifelinePosX(), lineY),
		operDef.isConst(), arrowLen);
	}
    for(const auto &stmt : operDef.getStatements())
	{
	int condOffset = condDepth * mPad;
	switch(stmt->getStatementType())
	    {
	    case ST_Call:
		{
		OperationCall *call = stmt->getCall();

		int targetIndex = call->getOperClassIndex();
		int lineY = y + mCharHeight + mPad*2;
		int sourcex = cls.getLifelinePosX();
		sourcex += condOffset;
		const OperationClass &targetCls = graph.getClass(targetIndex);
		int targetx = targetCls.getLifelinePosX();
		if(targetIndex == -1)
		    {
		    // Handle [else]
//		    int len = mCharHeight*3;
//		    mDrawer.drawLine(GraphPoint(sourcex, lineY),
//			    GraphPoint(sourcex+len, lineY), call->isConst());
		    }
		else if(targetIndex != sourceIndex)
		    {
		    drawCall(mDrawer, GraphPoint(sourcex, lineY),
			    GraphPoint(targetx, lineY), call->isConst(), arrowLen);
		    }
		else
		    {
		    // Draw line back to same class
		    int len = mCharHeight*3;
		    int height = 3;
		    mDrawer.drawLine(GraphPoint(sourcex, lineY),
			    GraphPoint(sourcex+len, lineY), call->isConst());
		    mDrawer.drawLine(GraphPoint(sourcex+len, lineY),
			    GraphPoint(sourcex+len, lineY+height));
		    mDrawer.drawLine(GraphPoint(sourcex, lineY+height),
			    GraphPoint(sourcex+len, lineY+height), call->isConst());
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
			GraphSize childSize = drawOperationNoText(GraphPoint(pos.x, y),
			    *childDef, graph, options, drawnOperations, drawStrings);
			y += childSize.y + mPad * 2;
			}
		    else
			{
			GraphRect rect(targetx + mPad*2, callPos.y + mPad*2, mCharHeight+mPad,
				mCharHeight+mPad);
			mDrawer.drawRect(rect);
			y += mCharHeight+mPad;
			}
		    poly.endChildBlock(condDepth, y);
		    }
		}
		break;

	    case ST_VarRef:
		{
		OperationVarRef *ref = stmt->getVarRef();

		int targetIndex = ref->getOperClassIndex();
		int lineY = y + mCharHeight + mPad*2;
		int sourcex = cls.getLifelinePosX();
		sourcex += condOffset;
		const OperationClass &targetCls = graph.getClass(targetIndex);
		int targetx = targetCls.getLifelinePosX();
		if(targetIndex == -1)
		    {
		    int len = mCharHeight*3;
		    mDrawer.drawLine(GraphPoint(sourcex, lineY),
			    GraphPoint(sourcex+len, lineY), ref->isConst());
		    }
		else if(targetIndex != sourceIndex)
		    {
		    drawCall(mDrawer, GraphPoint(sourcex, lineY),
			    GraphPoint(targetx, lineY), ref->isConst(), arrowLen);
		    }
		else
		    {
		    // Draw line back to same class
		    int len = mCharHeight*3;
		    int height = 3;
		    mDrawer.drawLine(GraphPoint(sourcex, lineY),
			    GraphPoint(sourcex+len, lineY), ref->isConst());
		    mDrawer.drawLine(GraphPoint(sourcex+len, lineY),
			    GraphPoint(sourcex+len, lineY+height));
		    mDrawer.drawLine(GraphPoint(sourcex, lineY+height),
			    GraphPoint(sourcex+len, lineY+height), ref->isConst());
		    y += height;
		    }
		int textX = (sourceIndex < targetIndex) ? cls.getLifelinePosX() :
			targetCls.getLifelinePosX();
		GraphPoint callPos(textX+condOffset+mPad, y+mCharHeight);
		drawStrings.push_back(DrawString(callPos, ref->getName()));
		ref->setRect(GraphPoint(callPos.x, callPos.y-mCharHeight),
			GraphSize(mCharHeight*50, mCharHeight+mPad));
		y += mCharHeight*2;
		}
		break;

	    case ST_OpenNest:
		{
		GraphPoint pos(cls.getLifelinePosX()+condOffset+
			mPad, y+mCharHeight);
		const OperationNestStart *cond = stmt->getNestStart();
		drawStrings.push_back(DrawString(pos, cond->getExpr()));
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
    mDrawer.drawPoly(poly, Color(245,245,255));

    return GraphSize(0, y - startpos.y);
    }

GraphSize OperationDrawer::drawOperation(GraphPoint pos,
	OperationDefinition &operDef, const OperationGraph &graph,
	const OperationDrawOptions &options,
	std::set<const OperationDefinition*> &drawnOperations)
    {
    std::vector<DrawString> drawStrings;

    mDrawer.groupShapes(true, Color(245,245,255));
    GraphSize size = drawOperationNoText(pos, operDef, graph, options,
	    drawnOperations, drawStrings);
    mDrawer.groupShapes(false, Color(245,245,255));

    mDrawer.groupText(true);
    for(size_t i=0; i<drawStrings.size(); i++)
	mDrawer.drawText(drawStrings[i].pos, drawStrings[i].str.c_str());
    mDrawer.groupText(false);
    return size;
    }
