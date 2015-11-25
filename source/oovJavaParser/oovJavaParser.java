// oovJavaParser.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

// This requires tools.jar from the JAVA SDK.

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import parser.*;
import model.ModelData;


public class oovJavaParser
    {
    public static void main(String[] args)
        {
        AnalysisParser parser = new AnalysisParser();
        if(args.length >= 3)
            {
            ArrayList<String> filenames = new ArrayList<String>();
            filenames.add(args[0]);
            for(int argi=3; argi<args.length; argi++)
                {
                filenames.add(args[argi]);
                }
            if(parser.parse(args[2], filenames.toArray(new String[filenames.size()])))
                {
                ModelWriter writer = new ModelWriter();
                String outFn = getOutputFileName(parser.getModel(), args[0],
                    args[1], args[2]);
                writer.write(parser.getModel(), outFn);
                writer.writeImportDependencies(parser.getModel(), args[0],
                    args[1], args[2]);
                }
            }
        else
            {
            System.err.println("oovCppParser args are: sourceFilePath sourceRootDir " +
                "outputProjectFilesDir [extraJavaFiles]...");
            }
        }


    static String getOutputFileName(ModelData model, String srcFileName,
        String srcRootDir, String outDir)
        {
	String moduleName = model.getModuleName();
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
