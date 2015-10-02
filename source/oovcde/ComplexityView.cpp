/*
 * ComplexityView.cpp
 *
 *  Created on: Jun 17, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "Complexity.h"
#include "ComplexityView.h"
#include "FilePath.h"
#include "Project.h"

static void createStyleTransform(const std::string &fullPath)
    {
    static const char *text =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n"
            "<xsl:output method=\"html\" />\n"
            "\n"
            "<xsl:key name=\"OperationsByClassName\" match=\"Oper\" use=\"ClassName\" />\n\n"

            "  <xsl:template match=\"/\">\n"
            "    <html>\n"
            "      <head>\n"
            "        <title>\n"
            "          <xsl:text>Complexity Report</xsl:text>\n"
            "        </title>\n"
            "      </head>\n"
            "      <body>\n\n"

// Main Header
            "    <h1>\n"
            "      <xsl:text>Complexity</xsl:text>\n"
            "    </h1>\n"
            "    <xsl:text>This document contains tables for class and operation complexity.</xsl:text><br/>\n"
            "    See <xsl:element name=\"a\">\n"
            "        <xsl:attribute name=\"href\">http://oovcde.sourceforge.net/articles/Complexity.html</xsl:attribute>\n"
            "        <xsl:text>http://oovcde.sourceforge.net/articles/Complexity.html</xsl:text>\n"
            "        </xsl:element> for more information.\n\n"

// Class table
            "  <h2>\n"
            "    <xsl:text>Class Complexity</xsl:text>\n"
            "  </h2>\n"
            "  <table border=\"1\">\n"
            "    <tr>\n"
            "      <th>Class Name</th>\n"
            "      <th>McCabe<br/>Complexity</th>\n"
            "      <th>Oovcde<br/>Complexity</th>\n"
            "    </tr>\n"
            "  <xsl:for-each select=\"ComplexityReport/Oper[count(. | key(\'OperationsByClassName\', ClassName)[1]) = 1]\">\n"
            "    <xsl:sort select=\"sum(key(\'OperationsByClassName\', ClassName)/Oovcde)\"\n"
            "        data-type=\"number\" order=\"descending\" />\n"
            "    <tr><td>\n"
            "      <xsl:value-of select=\"ClassName\" />\n"
            "    </td><td>\n"
            "      <xsl:value-of select=\"sum(key(\'OperationsByClassName\', ClassName)/McCabe)\" />\n"
            "    </td><td>\n"
            "      <xsl:value-of select=\"sum(key(\'OperationsByClassName\', ClassName)/Oovcde)\" />\n"
            "    </td></tr>\n"
            "  </xsl:for-each>\n"
            "  </table>\n\n"

// Operation table
            "  <h2>\n"
            "    <xsl:text>Operation Complexity</xsl:text>\n"
            "  </h2>\n"
            "    <table border=\"1\">\n"
            "      <tr>\n"
            "      <th>Class Name</th>\n"
            "      <th>Operation Name</th>\n"
            "      <th>McCabe<br/>Complexity</th>\n"
            "      <th>Oovcde<br/>Complexity</th>\n"
            "      </tr>\n"
            "      <xsl:apply-templates select=\"ComplexityReport/Oper\">\n"
            "    <xsl:sort select=\"Oovcde\" data-type=\"number\" order=\"descending\" />\n"
            "  </xsl:apply-templates>\n"
            "  </table>\n"

            "      </body>\n"
            "    </html>\n"
            "  </xsl:template>\n"
            "\n"
            "  <xsl:template match=\"Oper\">\n"
            "    <tr>\n"
            "      <td>\n"
            "        <xsl:value-of select=\"ClassName\" />\n"
            "      </td>\n"
            "      <td>\n"
            "        <xsl:value-of select=\"OperName\" />\n"
            "      </td>\n"
            "      <td>\n"
            "        <xsl:value-of select=\"McCabe\" />\n"
            "      </td>\n"
            "      <td>\n"
            "        <xsl:value-of select=\"Oovcde\" />\n"
            "      </td>\n"
            "    </tr>\n"
            "  </xsl:template>\n"
            "</xsl:stylesheet>\n";
    File transformFile(fullPath, "w");
    fprintf(transformFile.getFp(), "%s", text);
    }

bool createComplexityFile(ModelData const &modelData, std::string &fn)
    {
    FilePath fp(Project::getProjectDirectory(), FP_Dir);
    fp.appendDir("output");
    fp.appendFile("Complexity");

    FileEnsurePathExists(fp);
    FilePath fpxsl(fp, FP_File);
    fpxsl.appendExtension("xslt");
    createStyleTransform(fpxsl);

    fp.appendFile(".xml");
    fn = fp;
    File compFile(fp, "w");
    if(compFile.isOpen())
        {
        static const char *header =
                "<?xml version=\"1.0\"?>\n"
                "<?xml-stylesheet type=\"text/xsl\" href=\"Complexity.xslt\"?>\n"
                "<ComplexityReport>\n";
        fprintf(compFile.getFp(), "%s", header);
        for(auto const &type : modelData.mTypes)
            {
            ModelClassifier *classifier = type->getClass();
            if(classifier)
                {
                for(auto const &oper : classifier->getOperations())
                    {
                    int mcCabe = calcMcCabeComplexity(oper->getStatements());
                    int oov = calcOovComplexity(*oper);

                    static const char *item =
                        "  <Oper>\n"
                        "    <ClassName>%s</ClassName>\n"
                        "    <OperName>%s</OperName>\n"
                        "    <McCabe>%d</McCabe>\n"
                        "    <Oovcde>%d</Oovcde>\n"
                        "  </Oper>\n";
                    fprintf(compFile.getFp(), item, classifier->getName().makeXml().c_str(),
                            oper->getName().makeXml().c_str(), mcCabe, oov);
                    }
                }
            }
        fprintf(compFile.getFp(), "%s", "</ComplexityReport>\n");
        }
    return(compFile.isOpen());
    }


