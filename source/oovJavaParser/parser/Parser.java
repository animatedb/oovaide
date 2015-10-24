// Parser.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

// http://hg.openjdk.java.net/jdk7/jdk7/langtools/file/6855e5aa3348/test/tools/javac/api/6557752/T6557752.java

package parser;


// The compiler stuff is contained in tools.jar. Tools.jar is only included
// in a jdk, not a jre. Tools.jar must be in CLASSPATH.
import com.sun.source.util.*;	// For TreePathScanner, JavacTask
import javax.tools.JavaCompiler;
import javax.tools.ToolProvider;
import javax.tools.StandardJavaFileManager;
import com.sun.source.tree.*;
import com.sun.source.util.Trees;
import java.io.IOException;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

import model.*;


/*
class ParserClassTreeVisitor extends TreePathScanner<Object, Trees>
    {
    ParserClassTreeVisitor(ArrayList<String> classNames)
        { mClassNames = classNames; }

    @Override
    public Object visitClass(ClassTree classTree, Trees trees)
        {
        mClassNames.add(classTree.getSimpleName().toString());
        return super.visitClass(classTree, trees);
        }

    ArrayList<String> mClassNames;
    }
*/


public class Parser
    {
    public static ModelData model;

    public boolean parse(String srcFileName, String analysisDir)
        {
        boolean success = false;
        JavaCompiler compiler = ToolProvider.getSystemJavaCompiler();
        StandardJavaFileManager fileManager = compiler.
            getStandardFileManager(null, null, null);
        Iterable<? extends javax.tools.JavaFileObject> javaFileObjects =
            fileManager.getJavaFileObjects(srcFileName);

        JavacTask task = (JavacTask) compiler.getTask(null, fileManager, null, null,
            null, javaFileObjects);

	model = new ModelData();
	model.setModuleName(getAbsPath(srcFileName));
        try
            {
            Iterable<? extends CompilationUnitTree> asts = task.parse();

            task.analyze();
            Trees trees = Trees.instance(task);

            // Since we are only parsing a single file, there will be only a
            // single ast.
            for (CompilationUnitTree ast : asts)
                {

                ExpressionTree et = ast.getPackageName();

                String packageName = "";

                if(et != null)

                    {
                    packageName = et.toString();
                    }
                ArrayList<String> classNames = new ArrayList<String>();
//		new ParserClassTreeVisitor(classNames).scan(ast, trees);
                new ParserTreeVisitor(packageName, /*classNames,*/ analysisDir).
                    scan(ast, trees);

                }
            success = true;
            }
        catch(IOException e)
            {
            }
        return success;
        }

    String getAbsPath(String path)
        {
        String outFn = path;
        try
            {
	    File outFile = new File(path);
	    outFn = outFile.getCanonicalPath();
            }
        catch(IOException e)
            {
            }
        return outFn;
        }
    }

