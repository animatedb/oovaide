// oovJavaParser.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

// This requires tools.jar from the JAVA SDK.

import java.util.ArrayList;

import parser.*;
import common.*;
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
            boolean dups = false;
            for(int argi=3; argi<args.length; argi++)
                {
                if(args[argi].compareTo("-dups") == 0)
                    { dups = true; }
                else
                    { filenames.add(args[argi]); }
                }
            String dupHashFn = null;
            if(dups)
                {
                String dupsDir = Common.getNormalizedPath(args[2]);
                dupsDir = dupsDir.substring(0, dupsDir.lastIndexOf('/'));
                dupsDir = dupsDir.substring(0, dupsDir.lastIndexOf('/'));
                dupsDir += "/dups";
                dupHashFn = Common.getOutputFileName(args[0], args[1], dupsDir, "hsh");
                Common.ensurePathExists(dupsDir);
                }
            if(parser.parse(dupHashFn, args[2], filenames.toArray(new String[filenames.size()])))
                {
                JavaModelWriter writer = new JavaModelWriter();
                String outFn = Common.getOutputFileName(args[0], args[1], args[2], "xmi");
                writer.write(parser.getModel(), outFn);
                writer.writeImportDependencies(parser.getModel(), args[0],
                    args[1], args[2]);
                }
            }
        else
            {
            System.err.println("oovCppParser args are: sourceFilePath sourceRootDir " +
                "outputProjectFilesDir [-dups] [extraJavaFiles]...");
            }
        }
    }
