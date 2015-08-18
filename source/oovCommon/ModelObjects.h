/**
*   Copyright (C) 2013 by dcblaha
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*  @file      ModelObjects.h
*
*  @date      5/27/2009
*/

#ifndef MODEL_OBJECTS_H
#define MODEL_OBJECTS_H
#include <list>
#include <vector>
#include <memory>
#include "OovString.h"

#define UNDEFINED_ID -1

class ModelType;

// The basic structure is that the model contains types and associations.
// The types can be simple data types or records. The record type is called
// ModelClass and is used to hold classes, unions or structs.
//
// A class contains attributes and operations. The operations contain
// parameters and statements. A class can also inherit or reference
// other classes, which must already be declared at least as some kind of type.
//
// Many objects have names that can be resolved. The association connections
// is saved in files as a model ID that is mapped to each named object.

/// The visibility indicates the public, protected or private access.
class Visibility
    {
  public:
    enum VisType {
        Public,
        Protected,
        Private
    };
    Visibility(VisType v=Private)
        { vis = v; }
    /// Converts the UML string to the internal representation
    /// @param umlStr The string containing a UML character
    Visibility(OovStringRef const umlStr);
    /// Set the visibility.
    /// @param v The new visibility
    void setVis(VisType v)
        { vis = v; }
    /// Get the visibility.
    VisType getVis() const
        { return vis; }
    /// The UML encoding is represented as '+', '#', or '-'
    OovStringRef const asUmlStr() const;

  private:
    VisType vis;
};


/// This is the base type for most objects that represent the model.
/// It contains a name and ID. The ID is only used to resolve relations
/// between objects within a single XMI file.
class ModelObject
    {
    public:
        ModelObject(OovStringRef const name):
            mName(name), mModelId(UNDEFINED_ID)
            {}
        /// Get the name of the object
        const OovString &getName() const
            { return mName; }
        /// Set the name of the object
        /// @param name The new name
        void setName(OovStringRef const name)
            { mName = name; }
        /// Set the unique file reference identifier
        /// @param id the identifier
        void setModelId(int id)
            { mModelId = id; }
        /// Get the model id.
        int getModelId() const
            { return mModelId; }

    private:
        OovString mName;
        int mModelId;
    };

/// Defines a reference (use or relation) to a model type.  For example,
/// a class's data members can refer to other classes or simple data types,
/// and these relations can be const, pointers, or instantiations, etc.
/// A model type is defined elsewhere.
class ModelTypeRef
    {
    public:
        ModelTypeRef(ModelType  const *declType):
            mDeclType(declType), mDeclTypeModelId(0), mStatic(false),
            mConst(false), mRefer(false)
            {}
        /// Set the relation as static use of the model type
        /// @param isStatic Indicates whether the reference is static
        void setStatic(bool isStatic)
            { mStatic = isStatic; }
        /// Set the relation as const use of the model type
        /// @param isConst Indicates whether the reference is const
        void setConst(bool isConst)
            { mConst = isConst; }
        /// Set the relation as a pointer or reference use of the model type
        /// @param isRefer Indicates whether the relation is a pointer or reference.
        void setRefer(bool isRefer)
            { mRefer = isRefer; }
        /// Get the model type that this refers to.
        const ModelType *getDeclType() const
            { return mDeclType; }
        /// Set the model type that this refers to.
        void setDeclType(ModelType const *type)
            { mDeclType = type; }
        /// Set the model id during file loading so that the model type
        /// can be resolved
        /// @param id The declaration type model id
        void setDeclTypeModelId(int id)
            { mDeclTypeModelId = static_cast<unsigned int>(id); }
        /// Get the model id during file loading
        int getDeclTypeModelId() const
            { return mDeclTypeModelId; }
        /// Check if the relation is const
        bool isConst() const
            { return mConst; }
        /// Check if the relation is a reference or pointer
        bool isRefer() const
            { return mRefer; }
        /// Check if the model type is a match to this type.
        bool match(ModelTypeRef const &typeRef) const;

    private:
        ModelType const *mDeclType;
        unsigned int mDeclTypeModelId:29;
        unsigned int mStatic:1;
        unsigned int mConst:1;
        unsigned int mRefer:1;
    };

/// A C++ declarator is a type ref and a name.
/// A declarator can be a function return, function argument, attribute, etc.
class ModelDeclarator:public ModelObject, public ModelTypeRef
    {
    public:
        explicit ModelDeclarator(OovStringRef const name,
                ModelType const *declType):
            ModelObject(name), ModelTypeRef(declType)
            {}
        /// Get the classifier that this declarator is referring.
        const class ModelClassifier *getDeclClassType() const;
        /// Check if the delcarator matches
        /// Used for operation parameter matching
        /// @param decl the declarator to compare to
        bool match(ModelDeclarator const &decl) const
            { return ModelTypeRef::match(decl); }
    };


typedef ModelDeclarator ModelFuncParam;
typedef ModelDeclarator ModelBodyVarDecl;


/// This is a record(class/struct) data member.
class ModelAttribute:public ModelDeclarator
{
public:
    explicit ModelAttribute(OovStringRef const name, ModelType const *attrType,
            Visibility access):
        ModelDeclarator(name, attrType), mAccess(access)
        {}
    void setAccess(Visibility access)
        { mAccess = access; }
    Visibility getAccess() const
        { return mAccess; }
    private:
        Visibility mAccess;
};

enum eModelStatementTypes { ST_OpenNest, ST_CloseNest, ST_Call, ST_VarRef };


/// This represents some functionality in the code.
/// These aren't really statements. These are important things in a function
/// that must be displayed in sequence/operation diagrams.
class ModelStatement:private ModelObject
    {
    public:
        // Different types contain different parts of this class. This is done
        // to reduce memory in the ModelStatements vector.
        // OpenNest
        //      The name is the conditional statement and is optional.
        //      There is no decl
        // Call
        //      The name is the function name.
        //      The class decl points to the class type.
        // CloseNest
        //      There is no name or decl.
        // VarRef
        //      The name is the class's attribute name
        //      The class decl points to the class type.
        //      The var decl points to the variable type.
        ModelStatement(OovStringRef const name, eModelStatementTypes type):
            ModelObject(name), mStatementType(type), mClassDecl(nullptr),
            mVarDecl(nullptr), mVarAccessWrite(false)
            {}
        /// Get the conditional name, which is actually the conditional
        /// expression for an ST_OpenNest statement.
        OovString getCondName() const
            { return getName(); }
        /// Get the full name, which is the function name for an ST_Call
        /// statement.
        OovString getFullName() const
            { return getName(); }
        /// Get the function name for an ST_Call statement.
        OovString getFuncName() const;
        /// Get the attribute name for an ST_Call or ST_VarRef statement.
        OovString getAttrName() const;
        /// Check if the operations match.
        /// @todo - fix - should use const, etc.
        bool operMatch(OovStringRef calleeName) const
            { return (getFuncName().compare(calleeName) == 0); }
        /// Get the statement type.
        eModelStatementTypes getStatementType() const
            { return mStatementType; }
        /// Get the class declaration. These are only valid for call statements.
        ModelTypeRef &getClassDecl()
            { return mClassDecl; }
        /// Get the class declaration. These are only valid for call statements.
        const ModelTypeRef &getClassDecl() const
            { return mClassDecl; }
        /// Get the variable declaration. These are only valid for variable
        /// reference statements
        ModelTypeRef &getVarDecl()
            { return mVarDecl; }
        /// Get the variable declaration. These are only valid for variable
        /// reference statements
        const ModelTypeRef &getVarDecl() const
            { return mVarDecl; }
        /// Set whether the variable access is writeable
        void setVarAccessWrite(bool write)
            { mVarAccessWrite = write; }
        /// Get whether the variable access is writeable
        bool getVarAccessWrite() const
            { return mVarAccessWrite; }
        /// Just stick a symbol in the name to indicate it is
        /// a base class member reference.
        bool hasBaseClassMemberRef() const;
        static char const *getBaseClassMemberRefSep()
            { return "+:"; }
        static char const *getBaseClassMemberCallSep()
            { return "+:"; }

    private:
        eModelStatementTypes mStatementType;
        ModelTypeRef mClassDecl;
        // Only class member references are saved here.
        ModelTypeRef mVarDecl;
        unsigned int mVarAccessWrite;   // Indicates whether the var decl is written or read.
    };

/// This is a list of statements in a function.
class ModelStatements:public std::vector<ModelStatement>
    {
    public:
        ModelStatements()
            {}
        /// Add a statement to the list of statements of a function
        /// @param stmt The statement to add
        void addStatement(ModelStatement const &stmt)
            {
            push_back(stmt);
            }
        /// Check whether a variable is used by any of the statements.
        /// The variable could be used by a variable reference or call.
        /// @param attrName the variable to check
        bool checkAttrUsed(OovStringRef attrName) const;
    };


/// This represents an operation in the code.
/// This container owns all pointers (parameters, etc.) given to it, except for
/// types used to define parameters/variables.
class ModelOperation:public ModelObject
{
public:
    ModelOperation(OovStringRef const name, Visibility access,
            bool isConst, bool isVirtual):
        ModelObject(name), mAccess(access),
        mModule(nullptr), mLineNum(0), mReturnType(nullptr),
        mConst(isConst), mVirtual(isVirtual)
        {}
    /// Add a method parameter to the operation.
    /// Returns a pointer to the added parameter so that it can be modified.
    ModelFuncParam *addMethodParameter(const std::string &name, const ModelType *type,
        bool isConst);
    /// Add a method parameter to the operation.
    void addMethodParameter(std::unique_ptr<ModelFuncParam> param)
        {
        mParameters.push_back(std::move(param));
        }
    // The operation's definition has declarations.
/*    void addMethodDefinitionDeclarator(const std::string &name, const ModelType *type,
        bool isConst, bool isRef)
        {
        ModelDeclarator *decl = new ModelDeclarator(name, otDatatype, type);
        decl->setConst(isConst);
        decl->setRefer(isRef);
        mDefinitionDeclarators.push_back(decl);
        }
*/
    /// Add a body variable declarator to the operation.
    ModelBodyVarDecl *addBodyVarDeclarator(const std::string &name, const ModelType *type,
        bool isConst, bool isRef);
    /// Add a variable that is declared in the body of the operation.
    void addBodyVarDeclarator(std::unique_ptr<ModelBodyVarDecl> var)
        {
        mBodyVarDeclarators.push_back(std::move(var));
        }
    /// Check if the operation is a definition or declaration.
    /// @todo - the test simply checks the number of statements, so if a
    ///     function definition is empty, it will not be considered a definition.
    bool isDefinition() const;
    /// Get the parameters of the operation
    const std::vector<std::unique_ptr<ModelFuncParam>> &getParams() const
        { return mParameters; }
    /// Get the variables declared in the body of the operation
    const std::vector<std::unique_ptr<ModelBodyVarDecl>> &getBodyVarDeclarators() const
        { return mBodyVarDeclarators; }
    /// Get the return type of the operation
    ModelTypeRef &getReturnType()
        { return mReturnType; }
    /// Get the return type of the operation
    ModelTypeRef const &getReturnType() const
        { return mReturnType; }
    /// Get the statements of the operation
    ModelStatements &getStatements()
        { return mStatements; }
    /// Get the statements of the operation
    const ModelStatements &getStatements() const
        { return mStatements; }
    /// Set the visibility of the operation within the class
    void setAccess(Visibility access)
        { mAccess = access; }
    /// Get the visibility of the operation
    Visibility getAccess() const
        { return mAccess; }
    /// Set whether the operation is const
    void setConst(bool isConst)
        { mConst = isConst; }
    /// Check whether the operation is const
    bool isConst() const
        { return mConst; }
    /// Set whether the operation is virtual
    void setVirtual(bool isVirtual)
        { mVirtual = isVirtual; }
    /// Check whether the operation is virtual
    bool isVirtual() const
        { return mVirtual; }
    /// Set the module that the operation is defined in
    void setModule(const class ModelModule *module)
        { mModule = module; }
    /// Get the module that the operation is defined in
    const class ModelModule *getModule() const
        { return mModule; }
    /// Set the line number in the module that the operation is defined in
    void setLineNum(unsigned int lineNum)
        { mLineNum = lineNum; }
    /// Get the line number in the module that the operation is defined in
    unsigned int getLineNum() const
        { return mLineNum; }
    /// Check if the parameters match the parameters of this operation
    bool paramsMatch(std::vector<std::unique_ptr<ModelFuncParam>> const &params) const;

private:
    std::vector<std::unique_ptr<ModelFuncParam>> mParameters;
    std::vector<std::unique_ptr<ModelBodyVarDecl>> mBodyVarDeclarators;
    ModelStatements mStatements;
    Visibility mAccess;
    /// What module the operation is defined in (not declared)
    const class ModelModule *mModule;
    unsigned int mLineNum;
    ModelTypeRef mReturnType;
    unsigned int mConst:1;
    unsigned int mVirtual:1;
};


// A DT_Datatype (ModelType) is for simple non-class types. While the OovParser
// is parsing, it is also used for class types that are not in the currently
// compiled file. This is used for references to the type where the full type
// may not be known.  If the full type is later defined in the program, the
// type is upgraded to the DT_Class type if it is a record type (class or
// struct).  The Oovcde program must combine function definitions that could
// be defined in multiple files.
//
// A DT_Class (ModelClassifier) is used for records (class or struct) and for
// templates and typedefs.  The only reason that a typedef uses a DT_Class is
// because it contains the file and line location of the definition, and that
// simple DT_DataType types are not shown on diagrams as relations.
/// @todo - a template class should also have member functions/data
// A template may eventually use the attributes vector to keep relations
// between the template arguments and other classes.
// A typedef name is prepended with the UML <<typedef>> stereotype.  The Oovcde
// program does not draw this in a standard manner, since it draws it as
// being inherited from the original type, but the meaning seems pretty apparent.
enum eModelDataTypes { DT_DataType, DT_Class };

/// This stores info about simple (non-class) types, and is the base for all types.
/// See eModelDataTypes for more info.
class ModelType:public ModelObject
    {
    public:
        explicit ModelType(OovStringRef const name, eModelDataTypes type=DT_DataType):
            ModelObject(name), mDataType(type)
            {}
        static bool isTemplateDefType(OovString const &name);
        static char const *getTemplateStereotype()
            { return "<<template>>"; }
        static char const *getTypedefStereotype()
            { return "<<typedef>>"; }
        /// The type is determined by a UML stereotype in the name
        bool isTypedefType() const;
        /// The type is determined by a UML stereotype in the name
        bool isTemplateDefType() const;
        /// The type is determined by a UML stereotype in the name
        bool isTemplateUseType() const;
/*
        /// Is a typedef, or a template usage
        bool hasTypeArgs() const
            { return isTypedefType() || isTemplateUseType(); }
*/
        /// Get whether this is a class or simple data type
        eModelDataTypes getDataType() const
            { return mDataType; }
        /// @todo - remove this function
        const class ModelClassifier *getClass() const;
        /// @todo - remove this function
        class ModelClassifier *getClass();
        /// Get a class pointer. This returns null if the data type is not a class.
        static class ModelClassifier const *getClass(ModelType const *modelType);
        /// Get a class pointer. This returns null if the data type is not a class.
        static class ModelClassifier *getClass(ModelType *modelType);

    private:
        eModelDataTypes mDataType;
    };

/// This stores data for record definitions.
/// This owns all pointers except for the modules, and other types.
/// See eModelDataTypes for more info.
class ModelClassifier:public ModelType
{
public:
    explicit ModelClassifier(OovStringRef const name):
        ModelType(name, DT_Class), mModule(nullptr), mLineNum(0)
        {}
    /// Erase all of the operations
    void clearOperations();
    /// Erase all of the attributes
    void clearAttributes();
    /// Add an attribute to the class
    /// @param name The name of the attribute
    /// @param attrType The model type
    /// @param scope The visiblity of the attribute
    ModelAttribute *addAttribute(const std::string &name, ModelType const *attrType,
            Visibility scope);
    /// Add an attribute to the class
    /// @param attr The attribute to add
    void addAttribute(std::unique_ptr<ModelAttribute> &&attr)
        {
        mAttributes.push_back(std::move(attr));
        }
    /// Add an operation to the class
    /// @param name The name of the operation
    /// @param access The visiblity of the attribute
    /// @param isConst Indicates whether the operation is const.
    /// @param isVirtual Indicates whether the operation is virtual.
    ModelOperation *addOperation(const std::string &name, Visibility access,
            bool isConst, bool isVirtual);
    /// Remove an operation
    /// @param oper The operation to remove
    void removeOperation(ModelOperation *oper);
    /// Add an operation to the class
    /// @param oper The operation to add
    void addOperation(std::unique_ptr<ModelOperation> &&oper)
        {
        mOperations.push_back(std::move(oper));
        }
    /// Replace an operation.
    /// @param operToReplace The original operation that will be replaced
    /// @param newOper The new operation that will replace the original.
    void replaceOperation(ModelOperation const * const operToReplace,
            std::unique_ptr<ModelOperation> &&newOper);
    /// Erase an attribute.
    /// This allows removing the pointer from the collections so that they
    /// won't be destructed later.
    /// @param attr The attribute to be erased
    void eraseAttribute(const ModelAttribute *attr);
    /// Erase an operation.
    /// This allows removing the pointer from the collections so that they
    /// won't be destructed later.
    /// @param oper The operation to be erased
    void eraseOperation(const ModelOperation *oper);
    /// Get the index of an attribute
    /// @param name The name to get the index of
    size_t getAttributeIndex(const std::string &name) const;
    /// Get the attribute by name
    /// @param name The name of the attribute
    const ModelAttribute *getAttribute(const std::string &name) const;

    static const size_t NoIndex = static_cast<size_t>(-1);
    /// Find a matching operation and get the index. This finds by name,
    /// whether the operation is const, and the parameters decltypes, names, etc.
    /// @param op The operation to find
    size_t findExactMatchingOperationIndex(const ModelOperation &op) const;

    /// Find a matching operation and get the index. This finds by name,
    /// whether the operation is const, and the parameters decltypes, names, etc.
    /// @param op The operation to find
    const ModelOperation *findExactMatchingOperation(const ModelOperation &op) const;

    /// Do a poor job of getting an operation.
    /// @todo - this doesn't work for overloaded functions.
    const ModelOperation *getOperation(OovStringRef const name, bool isConst) const;

    /// Do a poor job of getting an operation.
    /// @todo - this doesn't work for overloaded functions.
    size_t getOperationIndex(OovStringRef const name, bool isConst) const;
    /// Do a poor job of getting an operation.
    /// @todo - this doesn't work for overloaded functions.
    ModelOperation *getOperation(OovStringRef const name, bool isConst)
        {
        return const_cast<ModelOperation*>(
                static_cast<const ModelClassifier*>(this)->getOperation(name, isConst));
        }
    /// Do a poor job of getting an operation.
    /// @todo - This isn't really correct.
    const ModelOperation *getOperationAnyConst(const std::string &name, bool isConst) const;

    /// Get the attributes of the class.
    std::vector<std::unique_ptr<ModelAttribute>> &getAttributes()
        { return mAttributes; }
    /// Get the attributes of the class.
    const std::vector<std::unique_ptr<ModelAttribute>> &getAttributes() const
        { return mAttributes; }
    /// Get the operations of the class.
    std::vector<std::unique_ptr<ModelOperation>> &getOperations()
        { return mOperations; }
    /// Get the operations of the class.
    const std::vector<std::unique_ptr<ModelOperation>> &getOperations() const
        { return mOperations; }
    /// Set the module of where the class is defined
    void setModule(const class ModelModule *module)
        { mModule = module; }
    /// Get the module of where the class is defined
    const class ModelModule *getModule() const
        { return mModule; }
    /// Set the line number of the module where the class is defined
    void setLineNum(unsigned int lineNum)
        { mLineNum = lineNum; }
    /// Get the line number of the module where the class is defined
    unsigned int getLineNum() const
        { return mLineNum; }
    /// Check if this is a class definition.
    /// @todo - This currently uses whether there are any operations or
    /// attributes to decide if this is a definition.
    bool isDefinition() const;

private:
    std::vector<std::unique_ptr<ModelAttribute>> mAttributes;
    std::vector<std::unique_ptr<ModelOperation>> mOperations;
    const class ModelModule *mModule;
    unsigned int mLineNum;
};

/// This is used for class inheritance
class ModelAssociation:public ModelObject
{
public:
    ModelAssociation(const ModelClassifier *child,
        const ModelClassifier *parent, Visibility access):
        ModelObject(""),
        mChildModelId(UNDEFINED_ID), mParentModelId(UNDEFINED_ID),
        mChild(child), mParent(parent), mAccess(access)
        {}
    /// Set the id of the child
    void setChildModelId(int id)
        { mChildModelId = id; }
    /// Set the id of the parent
    void setParentModelId(int id)
        { mParentModelId = id; }
    /// Get the id of the child
    int getChildModelId()
        { return mChildModelId; }
    /// Get the id of the parent
    int getParentModelId()
        { return mParentModelId; }
    /// Get the class of the child part of the relation
    const ModelClassifier *getChild() const
        { return mChild; }
    /// Get the class of the parent part of the relation
    const ModelClassifier *getParent() const
        { return mParent; }
    /// Set what type of visibility the child class has of the parent
    void setAccess(Visibility vis)
        { mAccess = vis; }
    /// Get the visibility
    const Visibility getAccess() const
        { return mAccess; }
    /// Set the class of the child part of the relation
    void setChildClass(const ModelClassifier *cl)
        { mChild = cl; }
    /// Set the class of the parent part of the relation
    void setParentClass(const ModelClassifier *cl)
        { mParent = cl; }

private:
    int mChildModelId;
    int mParentModelId;
    const ModelClassifier *mChild;
    const ModelClassifier *mParent;
    Visibility mAccess;
};

/// There can be code and comments on the same lines. There can also
/// be blank lines. So totalling these does not equal the number of
/// lines in the module.
class ModelModuleLineStats
    {
    public:
        ModelModuleLineStats(unsigned int code=0, unsigned int comments=0,
            unsigned int total=0):
            mNumCodeLines(code), mNumCommentLines(comments), mNumModuleLines(total)
            {}
        /// The number of lines that have some code on them.
        unsigned int mNumCodeLines;
        /// The number of lines that have a comment on them. If the comment
        /// includes a blank line, it is included in the count.
        unsigned int mNumCommentLines;
        /// The total number of lines in the module.
        unsigned int mNumModuleLines;
    };

/// This stores the module where an operation or class was defined.
class ModelModule:public ModelObject
    {
    public:
        ModelModule():
            ModelObject("")
            {}
        void setModulePath(OovStringRef const str)
            { setName(str); }
        const std::string &getModulePath() const
            { return getName(); }
        ModelModuleLineStats mLineStats;
    };

/// This is used for references to types in other classes.
/// The declarator is the type and the name of the reference to the type.
/// It is used for function parameters, and function body variable references
/// This is only used for temporary access, and is not stored in the model memory.
/// This only exists because call statements don't contain an explicit declarator,
/// and contain the name and the type separately.
class ModelDeclClass
    {
    public:
        ModelDeclClass(const ModelDeclarator *d):
            mDecl(*d)
            {}
        ModelDeclClass(OovStringRef const name,
                ModelType const *declType):
                mDecl(name, declType)
            {}
        ModelDeclarator const *getDecl() const
            { return &mDecl; }
        ModelClassifier const *getClass() const
            { return mDecl.getDeclClassType(); }
        bool operator<(const ModelDeclClass &rhs) const
            {
            return (mDecl.getName() < rhs.mDecl.getName() &&
                    mDecl.getDeclClassType() < rhs.mDecl.getDeclClassType());
            }
        bool operator==(const ModelDeclClass &rhs) const
            {
            return (mDecl.getName() == rhs.mDecl.getName() &&
                    mDecl.getDeclClassType() == rhs.mDecl.getDeclClassType());
            }

    private:
        const ModelDeclarator mDecl;
    };

// @todo - this could be a set to eliminate duplicates, but it doesn't work yet.
//typedef std::vector<ModelDeclClass> ConstModelDeclClasses;

class ConstModelDeclClasses:public std::vector<ModelDeclClass>
    {
    public:
        bool addUnique(ModelDeclClass const &decl);
    };

class ConstModelClassifierVector:public std::vector<ModelClassifier const *>
    {
    public:
        bool addUnique(ModelClassifier const *cl);
    };


/// Holds all data used to make class and sequence diagrams. This data is read
/// from the XMI files.
class ModelData
    {
    public:
        std::vector<std::unique_ptr<ModelType>> mTypes;                 // Some of these (otClasses) are Nodes
        std::vector<std::unique_ptr<ModelAssociation>> mAssociations;   // Edges
        std::vector<std::unique_ptr<ModelModule>> mModules;

        /// Erase all model information.  This includes types, associations,
        /// and modules.
        void clear();
        /// Use the model ids from the file to resolve references.  This should
        /// be done for every loaded file since ID's are specific for each file.
        void resolveModelIds();

        /// Go through the classes and operations and see if the type is
        /// referenced.
        /// @param type The type to check.
        bool isTypeReferencedByDefinedObjects(ModelType const &type) const;

        /// Add a type to the model.
        /// @param type The type to add.
        void addType(std::unique_ptr<ModelType> &&type);

        /// This finds the module using its ID.
        /// @param id The ID of the module to find.
        ModelModule const * findModuleById(int id);

        /// Either gets a type reference, or creates one if it does not exist.
        /// @param typeName The name of the type to find or create.
        /// @param otype This is only used if a type is created.
        ModelType *createOrGetTypeRef(OovStringRef const typeName, eModelDataTypes otype);

        /// Create a reference to a class or data type.
        /// @param typeName The name of the type.
        /// @param otype The type of data to create.
        ModelType *createTypeRef(OovStringRef const typeName, eModelDataTypes otype);

        /// Get a type. I am pretty sure that this is the same as findType.
        /// @param typeName The name of the type to retrieve.
        const ModelType *getTypeRef(OovStringRef const typeName) const;

        /// Find a type.
        /// @param typeName The name of the type to retrieve.
        ModelType *findType(OovStringRef const typeName);

        /// Find a type.
        /// @param typeName The name of the type to retrieve.
        const ModelType *findType(OovStringRef const typeName) const;

        /// Find a template definition type.  This discards the parameters
        /// to find the template class type.
        /// @param typeName The name of the type to retrieve.
        const ModelType *findTemplateType(OovStringRef const typeName) const;

        /// Replace a type.
        /// For all pointers to the old type, sets to the new type, then
        /// deletes the old type.
        /// @param existingType The original type.
        /// @param newType The new type.
        void replaceType(ModelType *existingType, ModelClassifier *newType);

        /// Takes the attributes from the source type, and moves them to the dest type.
        /// @param sourceType The type that the attributes will be taken from.
        /// @param destType Where the attributes will be copied to.
        void takeAttributes(ModelClassifier *sourceType, ModelClassifier *destType);

        /// Go through all types and find the ones that have identifiers between
        /// angle brackets in the name.  These will either be typedefs or templates.
        /// @param type The type to search for.
        /// @param classes The found matching classes.
        ///
        /// @todo - These relations are being found using name lookup at
        ///     graph generation instead of during xmi loading.  This is done
        ///     as a tradeoff of time/space, since the name has all of the
        ///     types that are related, and converting them to pointer
        ///     relations would take less space, but could require dynamically
        ///     creating the name.  Saving pointers and names just takes more space.
        void getRelatedTypeArgClasses(const ModelType &type,
                ConstModelClassifierVector &classes) const;

        /// Go through all operations of the class, and find all parameters
        /// of all operations. Similar to getRelatedFuncParamClasses
        /// except that the collection type is different.
        /// Originally, this was for params and return type. But the return type
        /// is usually also a declaration in the function.
        /// @param type The class to search
        /// @param classes The returned classes.
        void getRelatedFuncInterfaceClasses(const ModelClassifier &type,
                ConstModelClassifierVector &classes) const;

        /// See getRelatedFuncInterfaceClasses.
        void getRelatedFuncParamClasses(const ModelClassifier &type,
                ConstModelDeclClasses &classes) const;

        /// Get all of the body variables references for all of the operations
        /// in a class.
        /// @param type The class to search.
        /// @param declclasses The found classes.
        void getRelatedBodyVarClasses(const ModelClassifier &type,
                ConstModelDeclClasses &declclasses) const;

        /// Append all of the base classes of the specified class to the
        /// collection.
        /// This also gets bases of bases.
        /// @param type The type to get the base classes of.
        /// @param classes The returned classes.
        void addBaseClasses(ModelClassifier const &type,
                ConstModelClassifierVector &classes) const;

        /// Get the base type name by stripping all of the const, mutable,
        /// etc. keywords and punctuation.
        /// @param fullStr The starting string.
        static std::string getBaseType(OovStringRef const fullStr);

    private:
        ModelObject *createDataType(eModelDataTypes type, const std::string &id);
        void resolveStatements(class TypeIdMap const &typeMap, ModelStatements &stmt);
        void resolveDecl(class TypeIdMap const &typeMap, ModelTypeRef &decl);
        bool isTypeReferencedByStatements(ModelStatements const &stmts, ModelType const &type) const;
        void dumpTypes();
        /// Replace a statement
        void replaceStatementType(ModelStatements &stmts, ModelType *existingType,
                ModelClassifier *newType);
        /// Erase a type.
        void eraseType(ModelType *existingType);
    };

#endif
