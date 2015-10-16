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
        /// operClassIndex is the class of the operation that is called.
        OperationCall(size_t operClassIndex, const ModelOperation &operation):
            OperationStatement(ST_Call), mOperClassIndex(operClassIndex),
            mOperation(operation)
            {}
        virtual ~OperationCall();
        size_t getOperClassIndex() const
            { return mOperClassIndex; }
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
        void decrementClassIndex()
            { mOperClassIndex--; }

    private:
        size_t mOperClassIndex;
        const ModelOperation &mOperation;
        GraphRect rect;
    };

class OperationVarRef:public OperationStatement
    {
    public:
        /// operClassIndex is the class of the operation that is referred to.
        OperationVarRef(size_t operClassIndex, const ModelAttribute &attr):
            OperationStatement(ST_VarRef), mOperClassIndex(operClassIndex),
            mAttribute(attr)
            {}
        virtual ~OperationVarRef();
        size_t getOperClassIndex() const
            { return mOperClassIndex; }
        OovString const &getName() const
            { return mAttribute.getName(); }
        bool isConst() const
            { return false; }
        void setRect(const GraphPoint &pos, const GraphSize &size)
            {
            rect.start = pos;
            rect.size = size;
            }
        void getRect(GraphRect &r) const
            { r = rect; }
        const ModelAttribute &getAttribute() const
            { return mAttribute; }
        bool compareAttribute(const OperationVarRef &ref) const;
        void decrementClassIndex()
            { mOperClassIndex--; }

    private:
        size_t mOperClassIndex;
        const ModelAttribute &mAttribute;
        GraphRect rect;
    };

/// An operation definition is a method in a class, that defines all of the
/// functions that are called in other classes.
class OperationDefinition:public OperationCall
    {
    public:
        OperationDefinition(size_t classIndex, const ModelOperation &operation):
            OperationCall(classIndex, operation)
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
        bool isClassReferred(size_t classIndex) const;

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
            for(const auto &oper : mOperations)
                { delete oper; }
            mOperations.clear();
            mOpClasses.clear();
            }
        /// Clears the graph and adds related operations. See the addRelatedOperations
        /// function for more description.
        /// @todo - options and nodeDepth are not used.
        void clearGraphAndAddOperation(const ModelData &model,
                const OperationDrawOptions &options, char const * const className,
                char const * const operName, bool isConst, int nodeDepth);
        const std::vector<OperationClass> &getClasses() const
            { return mOpClasses; }
        const OperationClass &getClass(size_t index) const
            { return mOpClasses[index]; }
//      OperationClass *getNode(int x, int y);
        // Can only remove leaf operations?
//      void removeOperation(const ModelOperation *oper);
        size_t getNestDepth(size_t classIndex);
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
        static const size_t NO_INDEX = static_cast<size_t>(-1);

        void addDefinition(size_t classIndex, const ModelOperation &oper);
        void addOperCallers(const ModelStatements &stmts, const ModelClassifier &srcCls,
                const ModelOperation &oper, const OperationCall &callee);
        enum eGetClass { GC_AddClasses, FT_OnlyGetClasses };
        size_t addOrGetClass(const ModelClassifier *cls, eGetClass gc);
        void fillDefinition(const ModelStatements &stmt, OperationDefinition &opDef,
                eGetClass ft);
        void removeUnusedClasses();
        void removeOperation(size_t index);
        void removeClass(size_t index);
    };



#endif /* OPERATIONGRAPH_H_ */
