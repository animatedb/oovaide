/*
 * OperationGraph.h
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPERATIONGRAPH_H_
#define OPERATIONGRAPH_H_


#include "ModelObjects.h"
#include "Graph.h"
#include <gtk/gtk.h>	// For GtkWidget and cairo_t
#include <vector>

struct OperationDrawOptions
    {
    OperationDrawOptions():
	showConst(true)
	{}

    bool showConst;
    };

/// This has information about the size and position of a class
/// along the top of the sequence diagram.
class OperationClass
    {
    public:
	OperationClass(const ModelType *type):
	    mType(type)
	    {}
	const ModelType *getType() const
	    { return mType; }
	/// Positions are only valid after positionNodes is called.
	void setPosition(const GraphPoint &pos)
	    { rect.start = pos; }
	void setSize(const GraphSize &size)
	    { rect.size = size; }
	void getRect(GraphRect &r) const
	    { r = rect; }
	GraphPoint getPosition() const
	    { return rect.start; }
	int getLifelinePosX() const
	    { return rect.start.x + rect.size.x/2; }

    private:
	const ModelType *mType;
	GraphRect rect;
    };

enum OpStatementType { os_CondStart, os_CondEnd, os_Call };

/// A statement either defines a call, or a conditional start or end.
class OperationStatement
    {
    public:
	OperationStatement(OpStatementType type):
	    mStatementType(type)
	    {}
	OpStatementType getStatementType() const
	    { return mStatementType; }
	// Returns non-const, so that the rect can be modified.
	class OperationCall *getCall();
	const class OperationConditionStart *getCondStart() const;
	const class OperationConditionEnd *getCondEnd() const;
    private:
	OpStatementType mStatementType;
    };

class OperationConditionStart:public OperationStatement
    {
    public:
	OperationConditionStart(char const * const expr):
	    OperationStatement(os_CondStart), mExpression(expr)
	    {}
	char const * const getExpr() const
	    { return mExpression.c_str(); }
    private:
	std::string mExpression;
    };

class OperationConditionEnd:public OperationStatement
    {
    public:
	OperationConditionEnd():
	    OperationStatement(os_CondEnd)
	    {}
    };

/// A call is a call to a method in a class.
class OperationCall:public OperationStatement
    {
    public:
	/// operClassIndex is the class of the operation that is called.
	OperationCall(int operClassIndex, const ModelOperation &operation):
	    OperationStatement(os_Call), mOperClassIndex(operClassIndex),
	    mOperation(operation)
	    {}
	int getOperClassIndex() const
	    { return mOperClassIndex; }
	char const * const getName() const
	    { return mOperation.getName().c_str(); }
	bool isConst() const
	    { return mOperation.isConst(); }
	void setRect(const GraphPoint &pos, const GraphSize &size)
	    {
	    rect.start = pos;
	    rect.size = size;
	    }
	void getRect(GraphRect &r) const
	    { r = rect; }
	const ModelOperation &getOperation() const
	    { return mOperation; }
	bool compareOperation(const OperationCall &call) const;
	void decrementClassIndex()
	    { mOperClassIndex--; }

    private:
	int mOperClassIndex;
	const ModelOperation &mOperation;
	GraphRect rect;
    };

/// An operation definition is a method in a class, that defines all of the
/// functions that are called in other classes.
class OperationDefinition:public OperationCall
    {
    public:
	OperationDefinition(int classIndex, const ModelOperation &operation):
	    OperationCall(classIndex, operation)
	    {}
	~OperationDefinition()
	    {
	    removeStatements();
	    }
	void removeStatements()
	    {
	    for(size_t i=0; i<mStatements.size(); i++)
		{ delete mStatements[i]; }
	    mStatements.clear();
	    }
	void addCall(OperationCall *call)
	    {
	    mStatements.push_back(call);
	    }
	void addCondStart(char const * const exprName)
	    { mStatements.push_back(new OperationConditionStart(exprName)); }
	void addCondEnd()
	    { mStatements.push_back(new OperationConditionEnd()); }
	int getCondDepth() const;
	const std::vector<OperationStatement*> &getStatements() const
	    { return mStatements; }
	const OperationCall *getCall(int x, int y) const;
	bool isCalled(const OperationCall &opcall) const;
	bool isClassReferred(int classIndex) const;

    private:
	std::vector<OperationStatement*> mStatements;
	// Not defined to prevent copy.
	OperationDefinition(const OperationDefinition &def);
	void operator=(const OperationDefinition &def);
    };

/// The graph contains all classes and operations that will be displayed.
class OperationGraph
    {
    public:
	friend class OperationDrawer;
	OperationGraph():
	    mModified(false)
	    {}
	~OperationGraph()
	    {
	    clearGraph();
	    }
	void addOperation(const ModelOperation *oper);
	enum eAddOperationTypes
	    {
	    AO_AddCallers=0x01, AO_AddCalled=0x02,
	    AO_All=0xFF,
	    };
	/// Adds operations to a graph.
	/// @param model Used to look up related classes.
	/// @param oper The operation to be added to the graph.
	/// @param addType Defines the relationship types to look for.
	/// @param maxDepth Recurses to the specified depth. 1 adds operation with no relations.
	void addRelatedOperations(/*const ModelData &model, const OperationDrawOptions &options,*/
		const ModelClassifier &operClass, const ModelOperation &oper,
		eAddOperationTypes addType, int maxDepth);
	void clearGraph()
	    {
	    for(const auto &oper : mOperations)
		{ delete oper; }
	    mOperations.clear();
	    mOpClasses.clear();
	    }
	/// Clears the graph and adds related operations. See the addRelatedOperations
	/// function for more description.
	void clearGraphAndAddOperation(const ModelData &model,
		const OperationDrawOptions &options, char const * const className,
		char const * const operName, bool isConst, int nodeDepth);
	const std::vector<OperationClass> &getClasses() const
	    { return mOpClasses; }
	const OperationClass &getClass(int index) const
	    { return mOpClasses[index]; }
//	OperationClass *getNode(int x, int y);
	// Can only remove leaf operations?
//	void removeOperation(const ModelOperation *oper);
	int getCondDepth(int classIndex);
	const OperationClass *getNode(int x, int y) const;
	void removeNode(const OperationClass *classNode);
	const OperationCall *getOperation(int x, int y) const;
	std::string getClassName(const OperationCall &opcall) const;
	void addOperDefinition(const OperationCall &call);
	void addOperCallers(const ModelData &model, const OperationCall &call);
	void removeOperDefinition(const OperationCall &opcall);
	bool isOperCalled(const OperationCall &opcall) const;
	OperationDefinition *getOperDefinition(const OperationCall &opcall) const;
	bool isOperDefined(const OperationCall &opcall)
	    { return(getOperDefinition(opcall) != NULL); }
	bool isModified() const
	    { return mModified; }

    private:
	std::vector<OperationClass> mOpClasses;
	std::vector<OperationDefinition*> mOperations;
	GraphSize mPad;
	bool mModified;

	void addDefinition(int classIndex, const ModelOperation &oper);
	void addOperCallers(const ModelStatement *stmt, const ModelClassifier &srcCls,
		const ModelOperation &oper, const OperationCall &callee);
	enum eGetClass { GC_AddClasses, FT_OnlyGetClasses };
	int addOrGetClass(const ModelClassifier *cls, eGetClass gc);
	void fillDefinition(const ModelStatement *stmt, OperationDefinition &opDef,
		eGetClass ft);
	void removeUnusedClasses();
	void removeOperation(int index);
	void removeClass(int index);
    };



#endif /* OPERATIONGRAPH_H_ */
