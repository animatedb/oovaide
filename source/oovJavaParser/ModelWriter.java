// ModelWriter.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

import java.io.PrintWriter;
import java.io.IOException;
import java.util.HashMap;
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
    public void write(String outfileName)
        {
        Integer typeIndex = 10;		// Reserve some for the module or whatever.

        mTypeIndices = new HashMap<ModelType, Integer>();
        for(ModelType type : Parser.model)
            {
            mTypeIndices.put(type, typeIndex++);
            }

        try
            {
            XmlFile file = new XmlFile(outfileName);
            file.println("  <Module id=\"1\" module=\"" +
                Parser.model.getModuleName().replace('\\', '/') + "\" />");
            for(ModelType type : Parser.model)
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
                    line += "{" + stmt.getName();
                    break;

                case ST_CloseNest:
                    line += "}";
                    break;

                case ST_Call:
                    line += "c=" + stmt.getName() + "@" + getTypeIndexStr(
                        stmt.getClassType());
                    break;

                case ST_VarRef:
                    line += "v=" + stmt.getName() + "@" + getTypeIndexStr(
                        stmt.getClassType()) + "@" + getTypeIndexStr(stmt.getVarType()) + "@t";
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

    HashMap<ModelType, Integer> mTypeIndices;
    };
