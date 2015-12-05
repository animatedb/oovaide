// ModelWriter.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

import java.io.PrintWriter;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.RandomAccessFile;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.Iterator;

import model.*;
import parser.*;

class XmlFile
    {
    public XmlFile(String fileName) throws IOException
        {
        mFile = new PrintWriter(fileName, "UTF-8");
        println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        println("<XMI xmi.version=\"1.2\" xmlns:UML=\"http://schema.omg.org/spec/UML/1.3\" >");
        println(" <XMI.content>");
        }
    public void close()
        {
        println(" </XMI.content>");
        println("</XMI>");
        mFile.close();
        }
    public void println(String str)
        { mFile.println(str); }

    PrintWriter mFile;
    }

public class ModelWriter
    {
    HashMap<ModelType, Integer> mTypeIndices;

    public void write(ModelData model, String outFileName)
        {
        Integer typeIndex = 10;		// Reserve some for the module or whatever.

        mTypeIndices = new HashMap<ModelType, Integer>();
        for(ModelType type : model)
            {
            mTypeIndices.put(type, typeIndex++);
            }

        try
            {
            XmlFile file = new XmlFile(outFileName);
            String moduleName = model.getModuleName().replace('\\', '/');
            file.println("  <Module id=\"1\" module=\"" + moduleName +
                "\" moduleLines=\"" + model.getModuleLines() +  "\" />");
            for(ModelType type : model)
                {
                writeClass(file, type);
                writeAssoc(file, type);
                }
            file.close();
            }
        catch(IOException e)
            {
            }

        }

    String outXml(String str)
        {
        String outStr = str.replace(">", "&gt;");;
        return outStr.replace("<", "&lt;");
        }
    void writeClass(XmlFile file, ModelType type)
        {
/// @todo - Need to get a more complete class name including packages.
///	    Needs to work for private classes also.

        String typeIndexStr = getTypeIndexStr(type);
        String ending = "/>";
	if(type.isDefined())
            {
            ending = ">";
            }
        String moduleStr = "";
        if(type.getModule())
            {
            moduleStr = "module=\"1\" line=\"1\" ";
            }
        file.println("  <Class id=\"" + typeIndexStr + "\" name=\"" +
            outXml(type.getTypeName()) + "\" " + moduleStr + ending);

	if(type.isDefined())
            {
            writeMemberVars(file, type);
            writeMethods(file, type);
            file.println("  </Class>");
            }
        }

    void writeMemberVars(XmlFile file, ModelType type)
        {
        for(ModelTypeRef typeRef : type.getMemberVars())
            {
            String typeIndexStr = getTypeIndexStr(typeRef.getType());
            file.println("   <Attr name=\"" + typeRef.getName() + "\" type=\"" +
                typeIndexStr + "\" />");
            }
        }

    void writeMethods(XmlFile file, ModelType type)
        {
        for(ModelMethod method : type)
            {
// @todo - add: ret="1121" 
            file.println("   <Oper name=\"" + outXml(method.getMethodName()) + 
                "\" const=\"f\" virt=\"f\" line=\"1\" retconst=\"f\" retref=\"f\" >");
            writeMethodParams(file, method);
            writeStatements(file, method);
            file.println("   </Oper>");
            }
        }

    void writeMethodParams(XmlFile file, ModelMethod method)
        {
        if(method.getParameters().getTypeRefs().size() > 0)
            {
            String line = "    <Parms list=\"";
            for(int i=0; i<method.getParameters().getTypeRefs().size(); i++)
                {
                ModelTypeRef param = method.getParameters().getTypeRefs().get(i);
                if(i!=0)
                    {
                    line += "#";
                    }
                String typeId = getTypeIndexStr(param.getType());
                line += param.getName() + "@" + typeId + "@f@f";
                }
            file.println(line + "\" />");
            }
        }

    void writeStatements(XmlFile file, ModelMethod method)
        {
        String line = "    <Statements list=\"";
        boolean first = true;
	for(ModelStatement stmt : method.getStatements())
            {
            if(first)
                { first = false; }
            else
                { line += "#"; }
            switch(stmt.getStatementType())
                {
                case ST_OpenNest:
                    line += "{" + translate(stmt.getName());
                    break;

                case ST_CloseNest:
                    line += "}";
                    break;

                case ST_Call:
                    line += "c=" + translate(stmt.getName()) + "@" + getTypeIndexStr(
                        stmt.getClassType());
                    break;

                case ST_VarRef:
                    line += "v=" + translate(stmt.getName()) + "@" + 
                        getTypeIndexStr(stmt.getClassType()) + "@" +
                        getTypeIndexStr(stmt.getVarType()) + "@t";
                    break;
                }
            }
        file.println(line + "\" />");
        }

    void writeAssoc(XmlFile file, ModelType type)
        {
        Integer genId = 100000;
        String childTypeId = getTypeIndexStr(type);
        for(ModelTypeRelation typeRel : type.getRelations())
            {
            String parentTypeId = getTypeIndexStr(typeRel.getType());
            file.println("  <Genrl id=\"" + genId.toString() + "\" child=\"" +
                childTypeId + "\" parent=\"" + parentTypeId + "\" />");
            genId++;
            }
        }

    String getTypeIndexStr(ModelType type)
        {

        String str = "";

        if(type != null)
 
            { str = mTypeIndices.get(type).toString(); }

        else
            { str = " null "; }
        return str;

        }

    static void ensureLastPathSep(String dir)
        {
        int lastCharIndex = dir.length()-1;
        if(dir.charAt(lastCharIndex) != '/' &&
            dir.charAt(lastCharIndex) != '\\')
            { dir += "/"; }
        }

    String getAnalysisIncDepsFilename(String outDir)
        {
        String incDepFn = outDir;
        ensureLastPathSep(incDepFn);
        incDepFn += "oovaide-incdeps.txt";
        return incDepFn;
        }

    // Format is:    fn | time ; time; incpath ; incname ;
    // where incpath and incname are repeated for each imported file.
    String getImportStr(ModelData model, String srcRootDir, String absSrcFn)
        {
        ensureLastPathSep(srcRootDir);
        String str = "";
        if(model.getImports().size() > 0)
            {
            str += absSrcFn;
            str += '|';
            int time = (int)(System.currentTimeMillis() / 1000L);
            str += Integer.toString(time);	// Last parser update time
            str += ';';
            str += Integer.toString(time);  // Last parser check time - NOT UPDATED!
            str += ';';
            for(String importStr : model.getImports())
                {
                importStr = importStr.replace("import ", "");
                importStr = importStr.trim();
                importStr = importStr.substring(0, importStr.length()-1);	// Remove the last semicolon.
                importStr = importStr.replace(".", "/");
                importStr = importStr.trim();

                int lastSep = importStr.lastIndexOf('/');
                if(lastSep != -1)
                    {
                    String dir = importStr.substring(0, lastSep);
                    dir += '/';
                    if(Files.isDirectory(Paths.get(srcRootDir + dir)))
                        {
                        dir = srcRootDir + dir;
                        }
                    str += dir;
                    str += ';';
                    str += importStr.substring(lastSep+1);
                    str += ".java;";
                    }
                }
            }
        return str;
        }

    void sleepMs(int ms)
        {
        try
            {
            Thread.sleep(ms);
            }
        catch(InterruptedException ex)
            {
            Thread.currentThread().interrupt();
            }
        }

    void writeImportDependencies(ModelData model, String absSrcFn,
        String srcRootDir, String outDir)
        {
        // Follow a similar technique as oovCppWriter/SharedFile::open
        String incDepFn = getAnalysisIncDepsFilename(outDir);
        RandomAccessFile file = null;
        for(int i=0; i<50; i++)
            {
            try
                {
                try
                    {
                    file = new RandomAccessFile(incDepFn, "rw");
                    break;
                    }
                catch(FileNotFoundException e)
                    {
                    file = new RandomAccessFile(incDepFn, "w");
                    break;
                    }
                }
            catch(IOException e)
                {
                }
            sleepMs(100);
            }
        if(file != null)
            {
            ArrayList<String> lines = new ArrayList<String>();
            boolean keepReading = true;
            while(keepReading)
                {
                try
                    {
                    String str = file.readLine();
                    if(str != null)
                        {
                        str.trim();
                        lines.add(str);
                        }
                    else
                        { keepReading = false; }
                    }
                catch(IOException e)
                    {
                    keepReading = false;
                    }
                }

            for(int i=0; i<lines.size(); i++)
                {
                String line = lines.get(i);
                int barIndex = line.indexOf('|');
                // If there are no imports, there is no bar.
                if(barIndex != -1)
                    {
                    if(line.substring(0, barIndex).compareTo(absSrcFn) == 0)
                        {
                        lines.remove(i);
                        break;
                        }
                    }
                }
            String importStr = getImportStr(model, srcRootDir, absSrcFn);
            if(importStr.length() > 0)
                {
                lines.add(importStr);
                }

            try
                {
                if(importStr.length() > 0)
                    {
                    file.seek(0);
                    for(String line : lines)
                        {
                        file.writeBytes(line);
                        file.writeBytes(System.getProperty("line.separator"));
                        }
                    file.setLength(file.getFilePointer());
                    }
                file.close();
                }
            catch(IOException e)
                {
                }
            }
        }

    String translate(String str)
        {
        String retStr = "";
        for(int i=0; i<str.length(); i++)
            {
            switch(str.charAt(i))
                {
                case '>':           retStr += "&gt;";       break;
                case '<':           retStr += "&lt;";       break;
                case '&':           retStr += "&amp;";      break;
                case '\'':          retStr += "&apos;";     break;
                case '\"':          retStr += "&quot;";     break;
                default:            retStr += str.charAt(i);    break;
                }
            }
        return retStr;
        }

    }
