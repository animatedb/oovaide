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
#include <vector>


struct OperationDrawOptions
    {
    OperationDrawOptions()
        {}
    };

enum NodeTypes { NT_Class, NT_Variable };

// These are not really graph nodes. These are the blocks at the top of the
// operation diagram that contain classes, or variable names.
class OperationNode
    {
    public:
        OperationNode(NodeTypes nt):
            mNodeType(nt)
            {}
        virtual ~OperationNode()
            {}
        virtual OovString getName() const=0;
        /// Positions are only valid after updateNodeSizes is called.
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
        NodeTypes getNodeType() const
            { return mNodeType; }
        /// Returns nullptr if this is not a class
        class OperationClass *getClass();
        class OperationClass const *getClass() const;
        /// Returns nullptr if this is not a variable
        class OperationVariable *getVariable();
        class OperationVariable const *getVariable() const;

    private:
        GraphRect rect;
        NodeTypes mNodeType;
    };

/// This has information about the size and position of a class
/// along the top of the sequence diagram.
class OperationClass:public OperationNode
    {
    public:
        OperationClass(const ModelType *type):
            OperationNode(NT_Class), mType(type)
            {}
        virtual OovString getName() const
            { return mType->getName(); }
        const ModelType *getType() const
            { return mType; }

    private:
        const ModelType *mType;
    };

class OperationVariable:public OperationNode
    {
    public:
        OperationVariable(ModelClassifier const *cls, OovStringRef varName):
            OperationNode(NT_Variable), mOwnerClass(cls), mAttrName(varName)
            {}
        ModelClassifier const *getOwnerClass() const
            { return mOwnerClass; }
        OovString getAttrName() const
            { return mAttrName; }
        static OovString getName(ModelClassifier const *ownerClass, OovStringRef attrName)
            {
            OovString name = ownerClass->getName();
            name += "::";
            name += attrName;
            return name;
            }
        virtual OovString getName() const
            {
            return getName(mOwnerClass, mAttrName);
            }

    private:
        ModelClassifier const *mOwnerClass;
        OovString mAttrName;
    };

/// A statement either defines a call, or a conditional(nest) start or end.
class OperationStatement
    {
    public:
        OperationStatement(eModelStatementTypes type):
            mStatementType(type)
            {}
        /// All of the classes derived from operation statement are put into a
        /// collection, so they require a virtual destructor to delete the
        /// correctly inserted type.
        virtual ~OperationStatement();
        eModelStatementTypes getStatementType() const
            { return mStatementType; }
        // Returns non-const, so that the rect can be modified.
        class OperationCall *getCall();
        class OperationVarRef *getVarRef();
        const class OperationNestStart *getNestStart() const;
        const class OperationNestEnd *getNestEnd() const;
    private:
        eModelStatementTypes mStatementType;
    };

class OperationNestStart:public OperationStatement
    {
    public:
        OperationNestStart(char const * const expr):
            OperationStatement(ST_OpenNest), mExpression(expr)
            {}
        virtual ~OperationNestStart();
        OovString const &getExpr() const
            { return mExpression; }
    private:
        OovString mExpression;
    };

class OperationNestEnd:public OperationStatement
    {
    public:
        OperationNestEnd():
            OperationStatement(ST_CloseNest)
            {}
        virtual ~OperationNestEnd();
    };

/// A call is a call to a method in a class.
class OperationCall:public OperationStatement
    {
    public:
        /// destNode is the class of the operation that is called.
        OperationCall(OperationNode const *destNode, const ModelOperation &operation):
            OperationStatement(ST_Call), mDestNode(destNode),
            mOperation(operation)
            {}
        virtual ~OperationCall();
        OovString const getName() const
            { return mOperation.getName(); }
        OovString getOverloadFuncName() const
            { return mOperation.getOverloadFuncName(); }
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
        OperationNode const *getDestNode() const
            { return mDestNode; }

    private:
        OperationNode const *mDestNode;
        const ModelOperation &mOperation;
        GraphRect rect;
    };

class OperationVarRef:public OperationStatement
    {
    public:
        /// destNode is the class of the operation that is referred to.
        OperationVarRef(OperationNode const *destNode):
            OperationStatement(ST_VarRef), mDestNode(destNode)
            {}
        virtual ~OperationVarRef();
        bool isConst() const
            { return false; }
        void setRect(const GraphPoint &pos, const GraphSize &size)
            {
            rect.start = pos;
            rect.size = size;
            }
        void getRect(GraphRect &r) const
            { r = rect; }
        OperationNode const *getDestNode() const
            { return mDestNode; }

    private:
        OperationNode const *mDestNode;
        GraphRect rect;
    };

/// An operation definition is a method in a class, that defines all of the
/// functions that are called in other classes.
class OperationDefinition:public OperationCall
    {
    public:
        OperationDefinition(OperationNode const *destNode,
            const ModelOperation &operation):
            OperationCall(destNode, operation)
            {}
        virtual ~OperationDefinition();
        void removeStatements()
            {
            mStatements.clear();
            }
        void addStatement(std::unique_ptr<OperationStatement> stmt)
            {
            mStatements.push_back(std::move(stmt));
            }
        void addNestStart(OovStringRef const exprName)
            {
            /// @todo - use make_unique when supported.
            mStatements.push_back(std::unique_ptr<OperationStatement>(
                    new OperationNestStart(exprName)));
            }
        void addNestEnd()
            {
            mStatements.push_back(std::unique_ptr<OperationStatement>(
                    new OperationNestEnd()));
            }
        size_t getNestDepth() const;
        const std::vector<std::unique_ptr<OperationStatement>> &getStatements() const
            { return mStatements; }
        const OperationCall *getCall(int x, int y) const;
        bool isCalled(const OperationCall &opcall) const;
        bool isClassReferred(OperationNode const *destNode) const;

    private:
        std::vector<std::unique_ptr<OperationStatement>> mStatements;
        static const size_t NO_INDEX = static_cast<size_t>(-1);

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
        enum eAddOperationTypes
            {
            AO_AddCallers=0x01, AO_AddCalled=0x02,
            AO_All=0xFF
            };
        /// Adds operations to a graph.
        /// @param operClass Used to look up related classes.
        /// @param oper The operation to be added to the graph.
        /// @param addType Defines the relationship types to look for.
        /// @param maxDepth Recurses to the specified depth. 1 adds operation with no relations.
        /// @todo - addType isn't used.
        void addRelatedOperations(const ModelClassifier &operClass,
            const ModelOperation &oper, eAddOperationTypes /*addType*/, int maxDepth);
        void clearGraph()
            {
            mOperations.clear();
            mNodes.clear();
            }
        /// Clears the graph and adds related operations. See the addRelatedOperations
        /// function for more description.
        /// @todo - options and nodeDepth are not used.
        void clearGraphAndAddOperation(const ModelData &model,
                const OperationDrawOptions &options, char const * const className,
                char const * const operName, bool isConst, int nodeDepth);
        const std::vector<std::unique_ptr<OperationNode>> &getNodes() const
            { return mNodes; }
        const OperationNode &getNode(size_t index) const
            { return *mNodes[index]; }
        OperationNode &getNode(size_t index)
            { return *mNodes[index]; }
        int getNodeIndex(OperationNode const *node) const;
//      OperationClass *getNode(int x, int y);
        // Can only remove leaf operations?
//      void removeOperation(const ModelOperation *oper);
        size_t getNestDepth(size_t classIndex);
        OperationNode const *getNode(int x, int y) const;
        OperationClass const *getClass(int x, int y) const;
        void removeNode(const OperationNode *node);
        const OperationCall *getOperation(int x, int y) const;
        OovStringRef getNodeName(const OperationCall &opcall) const;
        void addOperDefinition(const OperationCall &call);
        void addOperCallers(const ModelData &model, const OperationCall &call);
        // Adds references from all of the classes in the operation graph.
        void addVariableReferencesFromGraphNodes(OperationNode const *node);
        void removeOperDefinition(const OperationCall &opcall);
        bool isOperCalled(const OperationCall &opcall) const;
        OperationDefinition *getOperDefinition(const OperationCall &opcall) const;
        bool isOperDefined(const OperationCall &opcall)
            { return(getOperDefinition(opcall) != NULL); }
        bool isModified() const
            { return mModified; }

    private:
        std::vector<std::unique_ptr<OperationNode>> mNodes;
        std::vector<std::unique_ptr<OperationDefinition>> mOperations;
        GraphSize mPad;
        bool mModified;
        static const size_t NO_INDEX = static_cast<size_t>(-1);

        void addDefinition(OperationNode const *destNode, ModelOperation const &oper);
        void addOperCallers(ModelStatements const &stmts, ModelClassifier const &srcCls,
                ModelOperation const &oper, OperationCall const &callee);
        enum eGetClass { GC_AddClasses, FT_OnlyGetClasses };
        size_t addOrGetClass(ModelClassifier const *cls, eGetClass gc);
        size_t addOrGetVariable(ModelClassifier const *cls, OovStringRef varName,
            eGetClass gc);
        void fillDefinition(ModelStatements const &stmt, OperationDefinition &opDef,
                eGetClass ft);
        void removeUnusedClasses();
        void removeOperation(size_t index);
        void removeNode(size_t index);
    };



#endif /* OPERATIONGRAPH_H_ */
