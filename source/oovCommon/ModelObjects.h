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
#include <string>
#include <list>
#include <vector>
#include <memory>

#define UNDEFINED_ID -1

class ModelType;


// The basic structure is that a class contains attributes and operations.
// A class can also inherit from another class, which must already be declared.
enum ObjectType
{
    // Class or struct
    otClass,

    // Generalization/inheritance
    otAssociation,

    // Class data member, same as data type, but includes reference/const and
    // visibility.
    otAttribute,

    // Function definition in a class with arguments, statements, etc.
    otOperation,

    // Function parameters, same as data type, but includes reference/const?
    otOperParam,

    // Call to a function
    otOperCall,

    // Conditional statement
    otCondStatement,

    // Variable declarations in a function body
    otBodyVarDecl,

    // This is used for types not defined in the current file.  This might be class
    // names defined elsewhere, or simple data types.
    // For encoding func params, type such as "unsigned int", "MyObject", etc.
    otDatatype,

    // This defines the source module that some entity came from.
    otModule,
};

class Visibility
    {
  public:
    enum VisType {
        Public,
        Protected,
        Private,
    };
    Visibility(VisType v=Private)
        { vis = v; }
    void setVis(VisType v)
	{ vis = v; }
    VisType getVis() const
	{ return vis; }
    char const * const asStr() const;
    char const * const asUmlStr() const;
  private:
    VisType vis;
};

class ModelObject
{
public:
    ModelObject(const std::string &name, ObjectType type):
        mName(name), mObjectType(type), mModelId(UNDEFINED_ID)
        {}
    const std::string &getName() const
        { return mName; }
    void setName(char const * const name)
	{ mName = name; }
    ObjectType getObjectType() const
        { return mObjectType; }
    void setModelId(int id)
	{ mModelId = id; }
    int getModelId() const
	{ return mModelId; }
    static const class ModelClassifier *getClass(const ModelObject *type);
    static class ModelClassifier *getClass(ModelObject *type);
private:
    std::string mName;
    ObjectType mObjectType;
    int mModelId;
};


// A declarator can be a function return, function argument, or attribute.
// Basically anything that refers to a type.
class ModelDeclarator:public ModelObject
{
public:
    explicit ModelDeclarator(const std::string &name, ObjectType type, const ModelType *declType):
        ModelObject(name, type), mDeclType(declType),  mDeclTypeModelId(0), mStatic(false),
        mConst(false), mRefer(false)
        {}
    void setStatic(bool isStatic)
        { mStatic = isStatic; }
    void setConst(bool isConst)
        { mConst = isConst; }
    void setRefer(bool isRefer)
        { mRefer = isRefer; }
    const ModelType *getDeclType() const
        { return mDeclType; }
    const class ModelClassifier *getDeclClassType() const;
    void setDeclType(const ModelType *type)
        { mDeclType = type; }
    void setDeclTypeModelId(int id)
	{ mDeclTypeModelId = id; }
    int getDeclTypeModelId() const
	{ return mDeclTypeModelId; }
    bool isConst() const
        { return mConst; }
    bool isRefer() const
        { return mRefer; }
    private:
        const ModelType *mDeclType;
        int mDeclTypeModelId;
        bool mStatic;
        bool mConst;
        bool mRefer;
};


class ModelFuncParam:public ModelDeclarator
{
public:
    ModelFuncParam(const std::string &name, const ModelType *type):
	ModelDeclarator(name, otOperParam, type)
        {}
};


// This is a record(class/struct) data member.
class ModelAttribute:public ModelDeclarator
{
public:
    explicit ModelAttribute(const std::string &name, const ModelType *attrType,
	    Visibility access):
        ModelDeclarator(name, otAttribute, attrType), mAccess(access)
        {}
    void setAccess(Visibility access)
        { mAccess = access; }
    Visibility getAccess() const
        { return mAccess; }
    private:
        Visibility mAccess;
};

class ModelBodyVarDecl:public ModelDeclarator
{
public:
    ModelBodyVarDecl(const std::string &name, const ModelType *type):
	ModelDeclarator(name, otBodyVarDecl, type)
        {}
};

// Derived types are conditionals or function calls.
class ModelStatement:public ModelObject
    {
    public:
	ModelStatement(const std::string &name, ObjectType type):
	    ModelObject(name, type)
	    {}
    };

class ModelOperationCall:public ModelStatement
    {
    public:
	// The type is the class that owns the operation.
	ModelOperationCall(const std::string &name, const ModelType *modType):
	    ModelStatement(name, otOperCall), mDecl("", otOperCall, modType)
	    {}
	ModelDeclarator &getDecl()
	    { return mDecl; }
	const ModelDeclarator &getDecl() const
	    { return mDecl; }
    private:
	ModelDeclarator mDecl;
    };

/// This holds recursive statements.  The statements can be operation calls
/// or conditionals with nested statements.
/// The name contains the conditional operation string or will be empty.
/// For a function, this will not have a name for the top level compound
/// statements.  Then for any conditionals, all function call statements
/// that are children of the conditional are stored.
/// This container owns all pointers (statements) given to it.
class ModelCondStatements:public ModelStatement
    {
    public:
	ModelCondStatements(const std::string &name):
	    ModelStatement(name, otCondStatement)
	    {}
	void addStatement(std::unique_ptr<ModelStatement> stmt)
	    {
	    mStatements.push_back(std::move(stmt));
	    }
	const std::vector<std::unique_ptr<ModelStatement>> &getStatements() const
	    { return mStatements; }
    private:
	std::vector<std::unique_ptr<ModelStatement>> mStatements;
    };

/// This container owns all pointers (parameters, etc.) given to it, except for
/// types used to define parameters/variables.
class ModelOperation:public ModelObject
{
public:
    ModelOperation(const std::string &name, Visibility access,
	    bool isConst):
        ModelObject(name, otOperation), mCondStatements(""), mAccess(access),
        mIsConst(isConst), mModule(nullptr), mLineNum(0)
        {}
    ~ModelOperation();
    // Returns a pointer to the added parameter so that it can be modified.
    ModelFuncParam *addMethodParameter(const std::string &name, const ModelType *type,
        bool isConst);
    void addMethodParameter(ModelFuncParam *param)
	{
        mParameters.push_back(param);
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
    ModelBodyVarDecl *addBodyVarDeclarator(const std::string &name, const ModelType *type,
        bool isConst, bool isRef);
    void addBodyVarDeclarator(ModelBodyVarDecl *var)
	{
        mBodyVarDeclarators.push_back(var);
        }
    bool isDefinition() const;
    const std::vector<ModelFuncParam*> getParams() const
	{ return mParameters; }
    const std::vector<ModelBodyVarDecl*> getBodyVarDeclarators() const
	{ return mBodyVarDeclarators; }
    ModelCondStatements &getCondStatements()
	{ return mCondStatements; }
    const ModelCondStatements &getCondStatements() const
	{ return mCondStatements; }
    void setAccess(Visibility access)
        { mAccess = access; }
    Visibility getAccess() const
        { return mAccess; }
    void setConst(bool isConst)
	{ mIsConst = isConst; }
    bool isConst() const
	{ return mIsConst; }
    void setModule(const class ModelModule *module)
	{ mModule = module; }
    const class ModelModule *getModule() const
	{ return mModule; }
    void setLineNum(int lineNum)
	{ mLineNum = lineNum; }
    int getLineNum() const
	{ return mLineNum; }

private:
    std::vector<ModelFuncParam*> mParameters;
    // This could be expanded to contain ref/ptr/const
//    ModelDeclarator* mDefinitionDeclarator;	// return type
    std::vector<ModelBodyVarDecl*> mBodyVarDeclarators;
    ModelCondStatements mCondStatements;
    Visibility mAccess;
    bool mIsConst;
    const class ModelModule *mModule;
    int mLineNum;
};


class ModelType:public ModelObject
    {
    public:
	explicit ModelType(const std::string &name, ObjectType type=otDatatype):
	    ModelObject(name, type)
	    {}
	bool isTemplateType() const;
    };

/// This owns all pointers except for the modules, and other types.
class ModelClassifier:public ModelType
{
public:
    explicit ModelClassifier(const std::string &name):
        ModelType(name, otClass), mOutput(O_NoOutput), mModule(nullptr),
        mLineNum(0)
        {}
    ~ModelClassifier();
    void clearOperations();
    void clearAttributes();
    ModelAttribute *addAttribute(const std::string &name, ModelType *attrType,
	    Visibility scope);
    void addAttribute(ModelAttribute *attr)
	{
        mAttributes.push_back(attr);
	}
    ModelOperation *addOperation(const std::string &name, Visibility access,
	    bool isConst);
    void removeOperation(ModelOperation *oper);
    ModelOperation *findMatchingOperation(ModelOperation const * const oper);
    int findMatchingOperationIndex(ModelOperation const * const oper);
    void addOperation(ModelOperation *oper)
        {
        mOperations.push_back(oper);
        }
    void replaceOperation(ModelOperation const * const operToReplace,
	    ModelOperation * const newOper)
	{
	int index = findMatchingOperationIndex(operToReplace);
	if(index != -1)
	    mOperations[index] = newOper;
	}
    // These allow removing the pointer from the collections so that they
    // won't be destructed later.
    void takeAttribute(const ModelAttribute *attr);
    void takeOperation(const ModelOperation *oper);
    /// @todo - this doesn't work for overloaded functions.
    const ModelOperation *getOperation(const std::string &name, bool isConst) const;
    int getOperationIndex(const std::string &name, bool isConst) const;
    ModelOperation *getOperation(const std::string &name, bool isConst)
	{
	return const_cast<ModelOperation*>(
		static_cast<const ModelClassifier*>(this)->getOperation(name, isConst));
	}
    /// @todo - This isn't really correct.
    const ModelOperation *getOperationAnyConst(const std::string &name, bool isConst) const;
    const std::vector<ModelAttribute*> getAttributes() const
	{ return mAttributes; }
    const std::vector<ModelOperation*> getOperations() const
	{ return mOperations; }
    void setModule(const class ModelModule *module)
	{ mModule = module; }
    const class ModelModule *getModule() const
	{ return mModule; }
    void setLineNum(int lineNum)
	{ mLineNum = lineNum; }
    int getLineNum() const
	{ return mLineNum; }
    bool isDefinition() const;
    // These are used for the ModelWriter for XMI files.
    enum ModelOutputs
	{
	O_NoOutput=0x0,
	O_DefineOperations=0x01,
	O_DefineAttributes=0x02,
	O_DefineClass=0x03	// Define attributes and operations
	};
    void setOutput(enum ModelOutputs out)
	{ mOutput = static_cast<ModelOutputs>(mOutput | out); }
    enum ModelOutputs getOutput() const
	{ return mOutput; }

private:
    std::vector<ModelAttribute*> mAttributes;
    std::vector<ModelOperation*> mOperations;
    enum ModelOutputs mOutput;
    const class ModelModule *mModule;
    int mLineNum;
};


class ModelAssociation:public ModelObject
{
public:
    ModelAssociation(const ModelClassifier *child,
        const ModelClassifier *parent, Visibility access):
        ModelObject("", otAssociation),
        mChildModelId(UNDEFINED_ID), mParentModelId(UNDEFINED_ID),
        mChild(child), mParent(parent), mAccess(access)
        {}
    void setChildModelId(int id)
	{ mChildModelId = id; }
    void setParentModelId(int id)
	{ mParentModelId = id; }
    int getChildModelId()
	{ return mChildModelId; }
    int getParentModelId()
	{ return mParentModelId; }
    const ModelClassifier *getChild() const
        { return mChild; }
    const ModelClassifier *getParent() const
        { return mParent; }
    void setAccess(Visibility vis)
	{ mAccess = vis; }
    const Visibility getAccess() const
        { return mAccess; }
    void setChildClass(const ModelClassifier *cl)
	{ mChild = cl; }
    void setParentClass(const ModelClassifier *cl)
	{ mParent = cl; }

private:
    int mChildModelId;
    int mParentModelId;
    const ModelClassifier *mChild;
    const ModelClassifier *mParent;
    Visibility mAccess;
};

class ModelModule:public ModelObject
    {
    public:
	ModelModule():
	    ModelObject("", otModule)
	    {}
	void setModulePath(const std::string &str)
	    { mModulePath = str; }
	const std::string &getModulePath() const
	    { return mModulePath; }
    private:
	std::string mModulePath;
    };

class ModelDeclClass
    {
    public:
	ModelDeclClass(const ModelDeclarator *d, const ModelClassifier *c):
	    decl(d), cl(c)
	    {}
	const ModelDeclarator *decl;
	const ModelClassifier *cl;
    };

typedef std::vector<ModelDeclClass> ConstModelDeclClassVector;
typedef std::vector<const ModelClassifier*> ConstModelClassifierVector;

class ModelData
    {
    public:
	std::vector<ModelType*> mTypes;			// Some of these (otClasses) are Nodes
	std::vector<ModelAssociation*> mAssociations;	// Edges
	std::vector<ModelModule*> mModules;

	void clear();
	/// Use the model ids from the file to resolve references.  This should
	/// be done for every loaded file since ID's are specific for each file.
        void resolveModelIds();
        void resolveStatements(ModelStatement *stmt);
        void resolveDecl(ModelDeclarator &decl);

        void addType(ModelType *type);
        ModelModule const * const findModuleById(int id);

        // otype is only used if a type is created.
	ModelType *createOrGetTypeRef(char const * const typeName, ObjectType otype);
	ModelType *createTypeRef(char const * const typeName, ObjectType otype);
	const ModelType *getTypeRef(char const * const typeName) const;
	ModelType *findType(char const * const name);
	const ModelType *findType(char const * const name) const;
	/// For all pointers to the old type, sets to the new type, then
	/// deletes the old type.
	void replaceType(ModelType *existingType, ModelClassifier *newType);
	void replaceStatementType(ModelStatement *stmts, ModelType *existingType,
		ModelClassifier *newType);
	void deleteType(ModelType *existingType);
	/// Takes the attributes from the source type, and moves them to the dest type.
	void takeAttributes(ModelClassifier *sourceType, ModelClassifier *destType);
	void getRelatedTemplateClasses(const ModelType &type,
		ConstModelClassifierVector &classes) const;
	void getRelatedFuncParamClasses(const ModelClassifier &type,
		ConstModelDeclClassVector &declclasses) const;
	void getRelatedBodyVarClasses(const ModelClassifier &type,
		ConstModelDeclClassVector &declclasses) const;
    private:
	ModelObject *createObject(ObjectType type, const std::string &id);
	std::string getBaseType(char const * const fullStr) const;
	const ModelClassifier *findClassByModelId(int id) const;
	const ModelType *findTypeByModelId(int id) const;
	void dumpTypes();
    };

#endif
