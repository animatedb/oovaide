// Parser.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

// http://hg.openjdk.java.net/jdk7/jdk7/langtools/file/6855e5aa3348/test/tools/
//      javac/api/6557752/T6557752.java

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
import java.io.PrintWriter;
import java.io.FileNotFoundException;

import common.*;
import model.*;

class DupHashFile
    {
    public void open(String fileName)
        {
        try
            {
            mFile = new PrintWriter(fileName, "UTF-8");
            }
        catch(IOException e)
            {
            mFile = null;
            }
        }
    public void close()
        {
        if(mFile != null)
            { mFile.close(); }
        }
    public void append(String text, int lineNum)
        {
        String str = fixSpaces(text);
        if(mFile != null)
            {
            mFile.println("" + Integer.toHexString(makeHash(str)) + " " + lineNum);
            }
        }
    static String fixSpaces(String text)
        {
        String str = "";
        boolean outSpace = true;
        for(int i=0; i<text.length(); i++)
            {
            char c = text.charAt(i);
	    if(Character.isWhitespace(c))
                {
                if(outSpace)
                    {
                    str += " ";
                    }
                outSpace = false;
                }
            else
                {
                str += c;
                outSpace = true;
                }
            }
        return str;
        }
    static int makeHash(String str)
        {
        // djb2 hash function
        int hash = 5381;
        for(int i=0; i<str.length(); i++)
            {
            char c = str.charAt(i);
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
            }
        return hash;
        }

    PrintWriter mFile;
    };


public class AnalysisParser
    {
    /// @param dupHashFn Set null for no duplicates.
    /// @param srcFileNames First arg is file to analyze, additional files can be
    /// passed to the compiler.
    public boolean parse(String dupHashFn, String analysisDir, String...srcFileNames)
        {
        boolean success = false;
        JavaCompiler compiler = ToolProvider.getSystemJavaCompiler();
        StandardJavaFileManager fileManager = compiler.
            getStandardFileManager(null, null, null);
        Iterable<? extends javax.tools.JavaFileObject> javaFileObjects =
            fileManager.getJavaFileObjects(srcFileNames);

        JavacTask task = (JavacTask) compiler.getTask(null, fileManager, null, null,
            null, javaFileObjects);

	model = new ModelData();
	model.setModuleName(Common.getAbsPath(srcFileNames[0]));
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
                SourcePositions sourcePosition = trees.getSourcePositions();
//                long startPos = sourcePosition.getStartPosition(ast, ast);
                long endPos = sourcePosition.getEndPosition(ast, ast);
                LineMap lineMap = ast.getLineMap();
                model.setModuleLines((int)lineMap.getLineNumber(endPos));

                ArrayList<String> classNames = new ArrayList<String>();
//		new ParserClassTreeVisitor(classNames).scan(ast, trees);
                new ParserTreeVisitor(model, packageName, analysisDir).
                    scan(ast, trees);

                if(dupHashFn != null)
                    {
                    DupHashFile dupHashFile = new DupHashFile();
                    dupHashFile.open(dupHashFn);
                    String lineSep = System.getProperty("line.separator");

                    String[] lines = ast.getSourceFile().getCharContent(false).toString().split(lineSep);
                    int lineNum = 0;
                    for(String line : lines)
                        {
                        dupHashFile.append(line, ++lineNum);
                        }
// The Java ast inserts extra lines for things such as enums.
/*
                    String[] lines = ast.toString().split(lineSep);
                    int lineNum = 0;
                    for(String line : lines)
                        {
                        dupHashFile.append(line, ++lineNum);
                        }
*/
                    dupHashFile.close();
                    }
                }
            success = true;
            }
        catch(IOException e)
            {
            }
        return success;
        }

    public ModelData getModel()
        { return model; }

    ModelData model;
    }

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


