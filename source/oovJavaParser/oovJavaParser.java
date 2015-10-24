// oovJavaParser.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

// Compile:	javac oovJavaParser.java Parser.java
//	set CLASSPATH if needed: C:\Program Files\Java\jdk1.8.0_51\lib\tools.jar
// Make jar:	jar cfm oovJavaParser.jar Manifest.txt *.class
// Run:		java -jar oovJavaParser.jar

import java.io.File;
import java.io.IOException;

import parser.*;


public class oovJavaParser
    {
    public static void main(String[] args)
        {
        Parser parser = new Parser();
        if(args.length >= 3)
            {
            if(parser.parse(args[0], args[2]))
                {
                ModelWriter writer = new ModelWriter();
                writer.write(getOutputFileName(args[0], args[1], args[2]));
                }
            }
        else
            {
            System.err.println("oovCppParser args are: sourceFilePath sourceRootDir " +
                "outputProjectFilesDir [javaArgs]...");
            }
        }


    static String getOutputFileName(String srcFileName, String srcRootDir, String outDir)
        {
	String moduleName = Parser.model.getModuleName();
        String outFn = getAbsPath(outDir);
        String srcFn = getAbsPath(srcFileName);
        String srcDir = getAbsPath(srcRootDir);
        if(srcFn.length() > srcDir.length())
            {
            srcFn = srcFn.substring(srcDir.length()+1);
            // All relative paths (.) should have been removed with getCanonicalPath,
            // so only the extension has a period.
            srcFn = srcFn.replace(".", "_d");
            srcFn += ".xmi";
            }
        while(srcFn.indexOf('/') != -1)
            {
            srcFn = srcFn.replace("/", "_s");
            }
        while(srcFn.indexOf('\\') != -1)
            {
            srcFn = srcFn.replace("\\", "_s");
            }
	outFn += '/' + srcFn;
        return outFn;
        }

    /// @todo - duplicate code
    static String getAbsPath(String path)
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
