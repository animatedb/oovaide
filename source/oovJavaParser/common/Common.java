// oovJavaParser.java
// Created on: Nov 25, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package common;

import java.io.File;
import java.io.IOException;

public class Common
    {
    public static String getAbsPath(String path)
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

    public static String getOutputFileName(String srcFileName,
        String srcRootDir, String outDir, String extension)
        {
        String outFn = getAbsPath(outDir);
        String srcFn = getAbsPath(srcFileName);
        String srcDir = getAbsPath(srcRootDir);
        if(srcFn.length() > srcDir.length())
            {
            srcFn = srcFn.substring(srcDir.length()+1);
            // All relative paths (.) should have been removed with getCanonicalPath,
            // so only the extension has a period.
            srcFn = srcFn.replace(".", "_d");
            srcFn += "." + extension;
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

    public static String getNormalizedPath(String srcFn)
        {
        while(srcFn.indexOf('\\') != -1)
            {
            srcFn = srcFn.replace("\\", "_s");
            }
        return srcFn;
        }

    public static void ensurePathExists(String path)
        {
	File dir = new File(path);
        if(!dir.exists())
            {
            dir.mkdir();
            }
        }
    }
