// ParserTreeVisitor.java
// Created on: Oct 20, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package parser;

import com.sun.source.util.Trees;
import com.sun.source.tree.*;
import com.sun.source.util.*;	// For TreePathScanner, JavacTask
import java.util.ArrayList;
import java.util.List;
import javax.lang.model.element.Element;
import javax.lang.model.element.ElementKind;
import javax.lang.model.element.TypeElement;
import javax.lang.model.type.TypeMirror;
import javax.lang.model.type.DeclaredType;
import javax.lang.model.type.TypeKind;
import com.sun.tools.javac.code.Symbol;
import com.sun.tools.javac.code.Symbol.TypeSymbol;
import com.sun.tools.javac.code.Symbol.ClassSymbol;
import com.sun.tools.javac.code.Symbol.VarSymbol;
import com.sun.tools.javac.tree.TreeInfo;
import com.sun.tools.javac.tree.JCTree.*;
import com.sun.tools.javac.tree.JCTree;

import model.*;


// Class names are resolved at run-time. See the following for notes.
// http://stackoverflow.com/questions/9812152/java-compile-time-class-resolution
// http://docs.oracle.com/javase/7/docs/technotes/tools/findingclasses.html
// http://stackoverflow.com/questions/8345220/java-difference-between-class-forname-and-classloader-loadclass

class ParserTreeVisitor extends TreePathScanner<Object, Trees>
    {
    ModelType currentClass;
    ModelMethod currentMethod;
    ModelData model;

    public ParserTreeVisitor(ModelData mod, String packageName, String analysisDir)
        {
        super();
        model = mod;
        model.setPackage(packageName);
        }
/*
    @Override
    public Object visitImport(ImportTree importTree, Trees trees)
        {
        Parser.model.addImport(importTree.toString());
        return super.visitImport(importTree, trees);
        }
*/
    @Override
    public Object visitClass(ClassTree classTree, Trees trees)
        {
        ModelType type = addOrFindType(classTree, trees);
        if(type != null)
            {
            type.setModule();
            currentClass = type;

            addTypeRelations(type, classTree, trees);
            addMemberVariables(type, classTree, trees);
            }
// At the moment, this displays for enums.
//        else
//            { System.out.println("Unable to find type " + classTree.getSimpleName()); }

        return super.visitClass(classTree, trees);
        }

    @Override
    public Object visitMethod(MethodTree methodTree, Trees trees)
        {
        if(currentClass != null)
            {
            ModelMethod method = new ModelMethod();
            currentMethod = method;
            method.setMethodName(methodTree.getName().toString());
            currentClass.addMethod(method);

            for(VariableTree varTree : methodTree.getParameters())
                {
                addTypeRef(method.getParameters(), varTree.getType(), trees,
                    varTree.getName().toString());
                }
            }
        return super.visitMethod(methodTree, trees);
        }

    @Override
    public Object visitIf(IfTree ifTree, Trees trees)
        {
        addOpenCondStatement("[" + ifTree.getCondition().toString() + "]");
        Object obj = super.visitIf(ifTree, trees);
        addCloseStatement();
        return obj;
        }

    @Override
    public Object visitForLoop(ForLoopTree forTree, Trees trees)
        {
        addOpenCondStatement("*[" + forTree.getCondition().toString() + "]");
        Object obj = super.visitForLoop(forTree, trees);
        addCloseStatement();
        return obj;
        }

    @Override
    public Object visitEnhancedForLoop(EnhancedForLoopTree forTree, Trees trees)
        {
//System.out.println("exp " + forTree.getExpression() + " st " + forTree.getStatement() + " v " + forTree.getVariable());
        addOpenCondStatement("*[" + forTree.getVariable() + " : " + forTree.getExpression() + "]");
        Object obj = super.visitEnhancedForLoop(forTree, trees);
        addCloseStatement();
        return obj;
        }

    @Override
    public Object visitWhileLoop(WhileLoopTree whileTree, Trees trees)
        {
        addOpenCondStatement("*[" + whileTree.getCondition().toString() + "]");
        Object obj = super.visitWhileLoop(whileTree, trees);
        addCloseStatement();
        return obj;
        }

    @Override
    public Object visitMethodInvocation(MethodInvocationTree callTree, Trees trees)
        {
        ModelStatement statement = new ModelStatement(
            ModelStatement.StatementType.ST_Call);
        // callName contains the full name such as:  classTree.getSimpleName().toString()
        String callName = callTree.getMethodSelect().toString();
        String methodName = callName;
	int lastDot = methodName.lastIndexOf('.');
        if(lastDot != -1)
            { methodName = methodName.substring(lastDot+1); }

	ModelType type = addOrFindType(callTree, trees);
        if(type != null)
            {
            statement.setClassType(type);
            statement.setName(methodName);
            currentMethod.getStatements().addStatement(statement);
            }
        return super.visitMethodInvocation(callTree, trees);
        }

// This is for the definition of a variable.
//    @Override
//    public Object visitVariable(VariableTree varTree, Trees trees)
//        {
//        ModelType type = addOrFindType(varTree, trees);
//        return super.visitVariable(varTree, trees);
//        }

/*
    // An expression statement does not contain a MEMBER_SELECT, but sometimes
    // contains a METHOD_INVOCATION.
    @Override
    public Object visitExpressionStatement(ExpressionStatementTree expTree, Trees trees)
        {
        ModelType type = addOrFindType(expTree, trees);
        return super.visitExpressionStatement(expTree, trees);
        }

    @Override
    public Object visitAssignment(AssignmentTree assTree, Trees trees)
        {
        addVarRefStatement(assTree, trees, assTree.getVariable().toString());
        return super.visitAssignment(assTree, trees);
        }

    @Override
    public Object visitMemberSelect(MemberSelectTree membTree, Trees trees)
        {
        return super.visitMemberSelect(membTree, trees);
        }
*/

    @Override
    public Object visitIdentifier(IdentifierTree identTree, Trees trees)
        {
        addVarRefStatement(identTree, trees, identTree.getName().toString());
        return super.visitIdentifier(identTree, trees);
        }


//////////////


    public void addVarRefStatement(Tree tree, Trees trees, String varName)
        {
        if(currentMethod != null && currentMethod.getStatements() != null)
            {
            ModelType varType = addOrFindType(tree, trees);
            if(varType != null)
                {
                ModelType classType = findOrAddEnclosingClassForVariable(tree, trees);
                if(classType != null)
                    {
                    ModelStatement statement = new ModelStatement(
                        ModelStatement.StatementType.ST_VarRef);

                    statement.setName(varName);
                    statement.setClassType(classType);
                    statement.setVarType(varType);
                    currentMethod.getStatements().addStatement(statement);
                    }
                }
            }
        }

    public void addOpenCondStatement(String condText)
        {
        ModelStatement statement = new ModelStatement(
            ModelStatement.StatementType.ST_OpenNest);
        statement.setName(condText);
        currentMethod.getStatements().addStatement(statement);
        }

    public void addCloseStatement()
        {
        ModelStatement statement = new ModelStatement(
            ModelStatement.StatementType.ST_CloseNest);
        currentMethod.getStatements().addStatement(statement);
        }

    void addTypeRelations(ModelType type, ClassTree classTree, Trees trees)
        {
	// Used classTree.getExtendsClause().getKind() to find the
	// type of the tree, use getClass() to find class.
	// classTree.getExtendsClause() returns a ParameterizedTypeTree.
	// parameterizedTypeTree.getType() returns an IdentifierTree.

        if(classTree.getExtendsClause() != null)
            {
            Tree parent = classTree.getExtendsClause();
            addTypeRel(type, parent, trees, ModelTypeRelation.RelationType.RT_Extends);
            }

        for(int i=0; i<classTree.getImplementsClause().size(); i++)
            {
            Tree parent = classTree.getImplementsClause().get(i);
            addTypeRel(type, parent, trees, ModelTypeRelation.RelationType.RT_Implements);
            }
        }

    void addMemberVariables(ModelType type, ClassTree classTree, Trees trees)
        {
        List<? extends Tree> memberVars = classTree.getMembers();
        if(memberVars.size() > 0)
            {
            for(int i=0; i<memberVars.size(); i++)
                {
                if(memberVars.get(i).getKind() == Tree.Kind.VARIABLE)
                    {
                    VariableTree varTree = (VariableTree)memberVars.get(i);
                    String parentName = varTree.getType().toString();

                    addTypeRef(type.getMemberVars(), varTree.getType(), trees,
                        varTree.getName().toString());
                    }
                }
            }
        }

    /// @param consumerType The type that the relationship will be added to.
    void addTypeRel(ModelType consumerType, Tree supplierTree, Trees trees,
        ModelTypeRelation.RelationType relType)
        {
        ModelType supplierType = addOrFindType(supplierTree, trees);
        if(supplierType != null)
            {
            ModelTypeRelation rel = new ModelTypeRelation();
            rel.setRelationType(ModelTypeRelation.RelationType.RT_Extends);
            rel.setType(supplierType);
            consumerType.addRelation(rel);
            }
        }

    void addTypeRef(ModelTypeRefs typeRefs, Tree supplierTree, Trees trees,
        String supplierVarName)
        {
        ModelType supplierType = addOrFindType(supplierTree, trees);
        if(supplierType != null)
            {
            ModelTypeRef ref = new ModelTypeRef();
            ref.setType(supplierType);
            ref.setName(supplierVarName);
            typeRefs.addTypeRef(ref);
            }
        }

    Scope safeGetScope(Trees trees, TreePath path)
        {
        Scope scope = null;
        try
            {
            // This crashes deep inside for some types of files.
            scope = trees.getScope(path);
            }
        catch(NullPointerException e)
            {
            }
        return scope;
        }

    // The tree passed in ((JCTree)tree).getClass() should return JCIdent.
    ModelType findOrAddEnclosingClassForVariable(Tree tree, Trees trees)
        {
        ModelType type = null;
        Scope scope = safeGetScope(trees, getCurrentPath());
        if(scope != null)
            {
            TypeElement cl = scope.getEnclosingClass();
            if(cl != null)
                {
                Element elem = TreeInfo.symbol((JCTree)tree);
                switch(elem.getKind())
                    {
                    case PARAMETER:
                    case LOCAL_VARIABLE:
                    case FIELD:
                        VarSymbol varSym = ((VarSymbol)elem);
                        type = addOrFindTypeUsingName(varSym.enclClass().toString());
                        break;
                    }
                }
            }
        return type;
        }

    // http://stackoverflow.com/questions/4881389/how-do-i-find-the-type-declaration-
    //      of-an-identifier-using-the-java-tree-compiler
    // From http://stackoverflow.com/questions/1066555/discover-the-class-of-a-
    //      methodinvocation-in-the-annotation-processor-for-java
    // http://www.docjar.com/docs/api/com/sun/tools/javac/tree/TreeInfo.html
    // https://bitbucket.org/jglick/qualifiedpublic/src/f2d33fd97c83/src/qualifiedpublic/
    //       PublicProcessor.java?fileviewer=file-view-default
    // http://stackoverflow.com/questions/2124201/how-do-i-get-the-type-of-the-expression-
    //       in-a-memberselecttree-from-a-javac-plugi

    // The trick to diagnosis to print any tree using ((JCTree)expTree).getClass()
    // http://www.docjar.com/docs/api/com/sun/tools/javac/tree/package-index.html
    String getTypeName(Tree tree, Trees trees)
        {
        String fullName = null;

//System.out.println(tree.getKind());
/*
        // This will find the last/lowest identifer.
        while(tree.getKind() == Tree.Kind.MEMBER_SELECT)
            {
            // This returns a METHOD_INVOCATION, IDENTIFIER, MEMBER_SELECT,
            // or NEW_CLASS.
            tree = ((MemberSelectTree)tree).getExpression();
//System.out.println("  " + tree.getKind());
            }
*/
        switch(tree.getKind())
            {
            case PARAMETERIZED_TYPE:
                {
                Element sym = TreeInfo.symbol((JCTree)tree);
                fullName = ((ClassSymbol)sym).getQualifiedName().toString();
                }
                break;

            case METHOD_INVOCATION:
                {
                ExpressionTree expTree = ((MethodInvocationTree)tree).getMethodSelect();
                Element method = TreeInfo.symbol((JCTree)expTree);
                TypeElement invokedClass = (TypeElement)method.getEnclosingElement();
                fullName = invokedClass.toString();
                }
                break;

            // Return null fullName for NEW_CLASS, since it is part of a MethodInvocationTree.
            case NEW_CLASS:
                break;

            case CLASS:
                fullName = "";
                String pkgName = model.getPackage();
                if(pkgName.length() > 0)
                    {
                    fullName = pkgName + ".";
                    }
                fullName += ((ClassTree)tree).getSimpleName();
                break;

            case IDENTIFIER:
                fullName = getIdentifierTypeName(tree, trees);
                break;

            // This might be used to determine modification of a variable, but
            // member selects would have to be checked also.
            // An expression statement can be something like: var1 = var2;
            // or:  var1.func(var2.func().func());
//            case EXPRESSION_STATEMENT:
//                {
//                ExpressionTree expTree = ((ExpressionStatementTree)tree).getExpression();
//                }
//                break;
//
//            // Only return names for identifiers (not method invocations).
//            case MEMBER_SELECT:
//                {
//                ExpressionTree expTree = ((MemberSelectTree)tree).getExpression();
//                if(expTree.getKind() == Tree.Kind.IDENTIFIER)
//                    {
//                    Element sym = TreeInfo.symbol((JCTree)expTree);
//                    JCIdent ident = (JCIdent)expTree;
//                    fullName = sym.toString();
//System.out.println("sel " + ident);
//                    }
//                }
//                break;
//
//            // AssignmentTree.getVariable returns INDENTIFIER on the left side of the assignment.
//            // AssignmentTree.getExpression returns the right side.
//            case ASSIGNMENT:
//                {
//                ExpressionTree varTree = ((AssignmentTree)tree).getVariable();
//                fullName = getTypeNameFromIdentifier(varTree, trees);
//                }
//                break;

            default:
//System.out.println("   " + tree);
                break;
            }

//if(fullName == null)
//   { System.out.println("No type name: " + tree.getKind() + " " + tree); }
//if(fullName != null && fullName.compareTo("nullModelWriter") == 0)
//   { System.out.println("Bad type name: " + tree.getKind() + " " + fullName); }

        return fullName;
        }

    // Returns null if the type does not need to be created.
    ModelType addOrFindType(Tree tree, Trees trees)
        {
        ModelType type = null;
        String fullName = getTypeName(tree, trees);
        if(fullName != null)
            {
            type = addOrFindTypeUsingName(fullName);
            }
//if(fullName == null)
//  { System.out.println("No type: " + tree.getKind() + " " + tree); }
        return type;
        }

    ModelType addOrFindTypeUsingName(String fullName)
        {
        ModelType type = model.findType(fullName);
        if(type == null)
            {
            type = new ModelType();
            type.setTypeName(fullName);
            model.addType(type);
            }
       return type;
       }

    String getIdentifierTypeName(Tree tree, Trees trees)
        {
        Element elem = TreeInfo.symbol((JCTree)tree);
        String fullName = null;
        switch(elem.getKind())           // ElementKind.*
            {
            case CLASS:
            case INTERFACE:
            case ENUM:
                fullName = ((ClassSymbol)elem).getQualifiedName().toString();
                break;

            case PACKAGE:
            case METHOD:
            case ENUM_CONSTANT:
            case ANNOTATION_TYPE:
                break;

            case LOCAL_VARIABLE:
                fullName = ((VarSymbol)elem).getQualifiedName().toString();
                break;

            default:
                fullName = lookupTypeNameFromIdentifier(tree, trees);
                if(fullName == null)
                    {
                    fullName = "BAD" + tree;
//System.out.println(" " + elem.getKind() + " " + fullName);
                    }
//debugDumpTreePath();
            }
        return fullName;
        }

    String lookupTypeNameFromIdentifier(Tree tree, Trees trees)
        {
        String fullName = null;
        Scope scope = safeGetScope(trees, getCurrentPath());
        if(scope != null)
            {
            TypeElement cl = scope.getEnclosingClass();

            for(Element elem : scope.getLocalElements())
                {
                if(elem.toString().compareTo(tree.toString()) == 0)
                    {
                    fullName = ((VarSymbol)elem).asType().toString();
                    break;
                    }
                }
            if(fullName == null)
                {
                for(Element elem : cl.getEnclosedElements())
                    {
                    if(elem.toString().compareTo(tree.toString()) == 0)
                        {
                        fullName = ((VarSymbol)elem).asType().toString();
//System.out.println(fullName);
                        }
                    }
                }
            }
//System.out.println(tree.getClass() + " " + tree + " " + fullName);
        return fullName;
        }




// Dumps from leaf back to root
// Different types for an identifier (Only showing 2 leaf most nodes):
//   IDENTIFIER PARAMETERIZED_TYPE	parameter
//   IDENTIFIER METHOD_INVOCATION       method call
//   IDENTIFIER ASSIGNMENT		assignment
//   METHOD CLASS                       WHAT?
//   IDENTIFIER ANNOTATION
//   IDENTIFIER METHOD
//   IDENTIFIER VARIABLE
//   IDENTIFIER MEMBER_SELECT
//   CLASS COMPILATION_UNIT             ???
/*
    void debugDumpTreePath()
        {
        String str = "";
        for(Tree tr : getCurrentPath())
            {
            str += tr.getKind() + " ";
            }
        System.out.println(str);
        }
*/
    }

