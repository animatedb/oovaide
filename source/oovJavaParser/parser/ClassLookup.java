// ClassLookup.java
// Created on: Oct 20, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.



/// THIS FILE IS NOT USED ANYMORE.
/*
package parser;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.Path;
import java.nio.charset.StandardCharsets;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

import model.ModelData;    // For import list and current package


// This file assumes that the package structure matches the directory
// structure.
// http://stackoverflow.com/questions/8395916/package-name-is-different-
//    than-the-folder-structure-but-still-java-code-compiles


// The compsource.txt file is created during project scanning, so it
// contains all project java files before the java files are parsed.
// Since the public class names in the java files must match the filename,
// the java file names can be used to search for the classes.
class ClassLookup
    {
    public ClassLookup(String projectDir)
        {
        mClassLookupMap = null;
        mFileLines = null;
        mProjectDir = projectDir;
        }


/// @todo - the classNames may not be needed. It looks like the typename already
///       has the package name included for certain classes.
    /// @param classNames The class names in the currently compiled java file.
    public String getFullClassName(String currentPackage,
        ArrayList<String> classNames, String typeName)
        {
        buildMapIfNeeded(mProjectDir);
        String fullName = null;
        if(typeName.indexOf('.') != -1) // || classNames.contains(typeName))
            {
//System.out.println("Using current file " + typeName);
//            fullName = currentPackage;
//            if(fullName.length() > 0)
//                { fullName += "."; }
//            fullName += typeName;
            fullName = typeName;
            }
        if(fullName == null)
            {
            fullName = getFullNameFromLookupMap(typeName);
            if(fullName != null)
                {
//System.out.println("Using comp sources file " + typeName);
                }
            }
        if(fullName == null)
            {
//System.out.println("Using global " + typeName);
            fullName = typeName;
            }
//System.out.println("full " + fullName);
        return fullName;
        }

    // The lookup map uses the imports to build the map, which must
    // be done after the imports are parsed. So this should be called
    // after the imports, but before looking up names.
    void buildMapIfNeeded(String projectDir)
        {
        if(mClassLookupMap == null)
            {
            mClassLookupMap = new HashMap<String, HashSet<String>>();
	    String compFn = "";
            try
                {
                compFn = projectDir + "/oovaide-tmp-compsources.txt";
                Path path = Paths.get(compFn);
                mFileLines = Files.readAllLines(path, StandardCharsets.UTF_8);
                createClassLookupMap();
                mFileLines.clear();
                }
            catch(IOException e)
                {
                System.out.println("Unable to open source info " + compFn);
                }
            }
        }

    /// Only adds entries that were in the import list and the current
    /// dir of the java file so that the size of the map is greatly reduced.
    void createClassLookupMap()
        {
        for(String line : mFileLines)
            {
            String javaPrefix = "Comp-java-";
            if(line.startsWith(javaPrefix))
                {
                int barPos = line.indexOf('|');
                if(barPos != -1)
                    {
                    String compSrcName = line.substring(javaPrefix.length(), barPos);
                    String compPackage = getPackageNameFromCompSourceName(compSrcName);
                    if(inImportsOrCurrentPackage(compPackage))
                        {
                        String filePathsStr = line.substring(barPos+1);
                        String[] filePaths = filePathsStr.split(";");
                        addClassLookupMapEntries(compPackage, filePaths);
                        }
                    }
                }
            }
        }

    /// The compSourceName is the name from the compsources.txt file
    /// after the Comp-java- prefix.
    String getPackageNameFromCompSourceName(String compSourceName)
        {
        String compPackage = compSourceName.replace('/', '.');
        // Remove the top level containing directory.
        int subDirPos = compPackage.indexOf('.');
        if(subDirPos != -1)
            { compPackage = compPackage.substring(subDirPos+1); }
        else
            { compPackage = ""; }
        return compPackage;
        }

    /// This does not check for class names out of packages. That
    /// filter should be applied later.
    boolean inImportsOrCurrentPackage(String compPackage)
        {
        boolean present = false;
        for(String importStr : Parser.model.getImports())
            {
            /// Remove the class name from the package string.
            String importPackage = importStr;
            int lastDot = importPackage.lastIndexOf('.');
            if(lastDot != -1)
                {
                importPackage = importPackage.substring(0, lastDot);
                }
            if(compPackage.compareTo(importPackage) == 0)
                {
                present = true;
                break;
                }
            }
        if(compPackage.compareTo(Parser.model.getPackage()) == 0)
            { present = true; }
        return(present);
        }

    void addClassLookupMapEntries(String compDir, String[] filePaths)
        {
        for(String path : filePaths)
            {
            String className = "";
            // Extract the class name from the filepath.
            int lastSlashPos = path.lastIndexOf('/');
            int extPos = path.lastIndexOf('.');
            if(lastSlashPos != -1 && extPos != -1)
                {
                className = path.substring(lastSlashPos+1, extPos);
                HashSet<String> dirs = mClassLookupMap.get(className);
                if(dirs == null)
                    {
                    dirs = new HashSet<String>();
                    }
                dirs.add(compDir);
//System.out.println("Adding(" + className + ") " + dirs);
                mClassLookupMap.put(className, dirs);
                }
            }
        }

    String getFullNameFromLookupMap(String simpleClassName)
        {
        String name = null;

        HashSet<String> dirs = mClassLookupMap.get(simpleClassName);
        if(dirs != null)
            {
            for(String dir : dirs)
                {
                name = dir + "." + simpleClassName;
                break;
                }
            }
        return name;
        }

    // First arg is class name, second is list of directories.
    HashMap<String, HashSet<String>> mClassLookupMap;
    List<String> mFileLines;
    String mProjectDir;
    }
*/
