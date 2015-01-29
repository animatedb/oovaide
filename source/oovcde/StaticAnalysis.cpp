/*
 * StaticAnalysis.cpp
 *
 *  Created on: Nov 24, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "StaticAnalysis.h"
#include "FilePath.h"
#include "Project.h"

static bool checkAttrUsed(std::string const &attrName, ModelStatements const &statements)
    {
    bool used = false;
    for(auto const &stmt : statements)
	{
	eModelStatementTypes stateType = stmt.getStatementType();
	if(stateType == ST_VarRef)
	    {
	    if(stmt.getName() == attrName)
		{
		used = true;
		break;
		}
	    }
	else if(stateType == ST_Call)
	    {
	    if(stmt.getAttrName() == attrName)
		{
		used = true;
		break;
		}
	    }
	}
    return used;
    }

static void createStyleTransform(const std::string &fullPath)
    {
    static const char *text =
	    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	    "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n"
	    "<xsl:output method=\"html\" />\n"
	    "\n"
	    "  <xsl:template match=\"/\">\n"
	    "    <html>\n"
	    "      <head>\n"
	    "        <title>\n"
	    "          <xsl:text>Member Attribute Usage Report</xsl:text>\n"
	    "        </title>\n"
	    "      </head>\n"
	    "      <body>\n"
	    "        <h1>\n"
	    "          <xsl:text>Member Usage</xsl:text>\n"
	    "        </h1>\n"
	    "        <xsl:text>The usage count is the count of the number of methods in"
	    "		the same class that refer to the attribute.</xsl:text>\n"
	    "        <table border=\"1\">\n"
	    "          <tr>\n"
	    "            <th>Class Name</th>\n"
	    "            <th>Attribute Name</th>\n"
	    "            <th>Use Count</th>\n"
	    "          </tr>\n"
	    "          <xsl:apply-templates select=\"MemberAttrUseReport/Attr\">\n"
	    "	    <xsl:sort select=\"UseCount\" data-type=\"number\" />\n"
	    "	  </xsl:apply-templates>\n"
	    "        </table>\n"
	    "      </body>\n"
	    "    </html>\n"
	    "  </xsl:template>\n"
	    "\n"
	    "  <xsl:template match=\"Attr\">\n"
	    "    <tr>\n"
	    "      <td>\n"
	    "        <xsl:value-of select=\"ClassName\" />\n"
	    "      </td>\n"
	    "      <td>\n"
	    "        <xsl:value-of select=\"AttrName\" />\n"
	    "      </td>\n"
	    "      <td>\n"
	    "        <xsl:value-of select=\"UseCount\" />\n"
	    "      </td>\n"
	    "    </tr>\n"
	    "  </xsl:template>\n"
	    "</xsl:stylesheet>\n";
    File transformFile(fullPath, "w");
    fprintf(transformFile.getFp(), "%s", text);
    }

bool createStaticAnalysisFile(ModelData const &modelData, std::string &fn)
    {
    FilePath fp(Project::getProjectDirectory(), FP_Dir);
    fp.appendDir("output");
    fp.appendFile("StaticAnalysis");

    FileEnsurePathExists(fp);
    createStyleTransform(fp + ".xslt");

    fp.appendFile(".xml");
    fn = fp;
    File useFile(fp, "w");
    if(useFile.isOpen())
	{
	static const char *header =
		"<?xml version=\"1.0\"?>\n"
		"<?xml-stylesheet type=\"text/xsl\" href=\"StaticAnalysis.xslt\"?>\n"
		"<MemberAttrUseReport>\n";
	fprintf(useFile.getFp(), "%s", header);
	for(auto const &type : modelData.mTypes)
	    {
	    ModelClassifier *classifier = type->getClass();
	    if(classifier)
		{
		for(auto const &attr : classifier->getAttributes())
		    {
		    int usageCount = 0;
		    for(auto const &oper : classifier->getOperations())
			{
			if(checkAttrUsed(attr->getName(), oper->getStatements()))
			    usageCount++;
			}
		    static const char *item =
			"  <Attr>\n"
			"    <ClassName>%s</ClassName>\n"
			"    <AttrName>%s</AttrName>\n"
			"    <UseCount>%d</UseCount>\n"
			"  </Attr>\n";
		    fprintf(useFile.getFp(), item,
			classifier->getName().c_str(), attr->getName().c_str(), usageCount);
		    }
		}
	    }

	static const char *footer = "</MemberAttrUseReport>\n";
	fprintf(useFile.getFp(), "%s", footer);
	}
    return useFile.isOpen();
    }
