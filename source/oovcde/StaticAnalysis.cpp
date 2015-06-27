/*
 * StaticAnalysis.cpp
 *
 *  Created on: Nov 24, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "StaticAnalysis.h"
#include "FilePath.h"
#include "Project.h"


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
            "           the same class that refer to the attribute.</xsl:text>\n"
            "        <table border=\"1\">\n"
            "          <tr>\n"
            "            <th>Class Name</th>\n"
            "            <th>Attribute Name</th>\n"
            "            <th>Use Count</th>\n"
            "          </tr>\n"
            "          <xsl:apply-templates select=\"MemberAttrUseReport/Attr\">\n"
            "       <xsl:sort select=\"UseCount\" data-type=\"number\" />\n"
            "     </xsl:apply-templates>\n"
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
                        if(oper->getStatements().checkAttrUsed(attr->getName()))
                            usageCount++;
                        }
                    static const char *item =
                        "  <Attr>\n"
                        "    <ClassName>%s</ClassName>\n"
                        "    <AttrName>%s</AttrName>\n"
                        "    <UseCount>%d</UseCount>\n"
                        "  </Attr>\n";
                    fprintf(useFile.getFp(), item,
                        classifier->getName().makeXml().c_str(), attr->getName().c_str(), usageCount);
                    }
                }
            }

        static const char *footer = "</MemberAttrUseReport>\n";
        fprintf(useFile.getFp(), "%s", footer);
        }
    return useFile.isOpen();
    }


bool createProjectStats(ModelData const &modelData, std::string &displayStr)
    {
    unsigned numClasses = 0;
    unsigned numFiles = modelData.mModules.size();
    unsigned numOps = 0;
    unsigned numAttrs = 0;
    unsigned maxOpsPerClass = 0;
    unsigned maxAttrsPerClass = 0;
    std::string maxOpsStr;
    std::string maxAttrStr;
    for(auto const &type : modelData.mTypes)
        {
        ModelClassifier const *classifier = type.get()->getClass();
        if(classifier && classifier->getName().length() > 0)
            {
            numClasses++;
            unsigned ops = classifier->getOperations().size();
            if(ops > maxOpsPerClass)
                {
                maxOpsStr = classifier->getName();
                maxOpsPerClass = ops;
                }
            numOps += ops;
            unsigned attrs = classifier->getAttributes().size();
            if(attrs > maxAttrsPerClass)
                {
                maxAttrStr = classifier->getName();
                maxAttrsPerClass = attrs;
                }
            numAttrs += attrs;
            }
        }
    OovString str;
    str += "Number of files: ";
    str.appendInt(numFiles);
    str += "\nNumber of classes: ";
    str.appendInt(numClasses);
    str += "\nNumber of operations: ";
    str.appendInt(numOps);
    str += "\nNumber of attributes: ";
    str.appendInt(numAttrs);
    str += "\n";
    if(numFiles > 0)
        {
        str += "\nAverage classes per file: ";
        str.appendFloat(static_cast<float>(numClasses)/numFiles);
        }
    if(numClasses > 0)
        {
        str += "\nAverage operations per class: ";
        str.appendFloat(static_cast<float>(numOps)/numClasses);
        str += "\nAverage attributes per class: ";
        str.appendFloat(static_cast<float>(numAttrs)/numClasses);
        str += "\n";
        str += "\nMax operations per class: ";
        str.appendFloat(maxOpsPerClass);
        str += ' ' + maxOpsStr;
        str += "\nMax attributes per class: ";
        str.appendFloat(maxAttrsPerClass);
        str += ' ' + maxAttrStr;
        }
    displayStr = str;
    return true;
    }

static void createLineStatsStyleTransform(const std::string &fullPath)
    {
    static const char *text =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n"
            "<xsl:output method=\"html\" />\n"
            "\n"
            "  <xsl:key name=\"ModulesByModuleDir\" match=\"Module\" use=\"ModuleDir\" />\n\n"

            "  <xsl:template match=\"/\">\n"
            "    <html>\n"
            "      <head>\n"
            "        <title>\n"
            "          <xsl:text>Line Statistics Report</xsl:text>\n"
            "        </title>\n"
            "      </head>\n"
            "      <body>\n"

            "  <h1>\n"
            "    <xsl:text>Line Statistics</xsl:text>\n"
            "  </h1>\n"
            "  <xsl:text>Note that code and comments can be on the same line,\n"
            "     and will be counted in both counts.</xsl:text>\n"
// Output the total project stats

            "    <p/><xsl:text>Total code lines:</xsl:text>\n"
            "      <xsl:call-template name=\"SumCodeLines\" /><br/>\n"

            "    <xsl:text>\n"
            "      Total comment lines: \n"
            "    </xsl:text>\n"
            "      <xsl:call-template name=\"SumCommentLines\" /><p/>\n"
// Output the directory lines table
            "  <h2>\n"
            "    <xsl:text>Directory Lines</xsl:text>\n"
            "  </h2>\n"
            "  <table border=\"1\">\n"
            "    <tr>\n"
            "      <th>Module Dir</th>\n"
            "      <th>Code Lines</th>\n"
            "      <th>Comment Lines</th>\n"
            "      <th>Module Lines</th>\n"
            "    </tr>\n"
            "  <xsl:for-each select=\"LineStatisticsReport/Module[count(. | key(\'ModulesByModuleDir\', ModuleDir)[1]) = 1]\">\n"
            "      <xsl:sort select=\"sum(key(\'ModulesByModuleDir\', ModuleDir)/ModuleLines)\"\n"
            "      data-type=\"number\" order=\"ascending\" />\n"
            "      <tr><td>\n"
            "      <xsl:value-of select=\"ModuleDir\" />\n"
            "      </td><td>\n"
            "      <xsl:value-of select=\"sum(key(\'ModulesByModuleDir\', ModuleDir)/CodeLines)\" />\n"
            "      </td><td>\n"
            "      <xsl:value-of select=\"sum(key(\'ModulesByModuleDir\', ModuleDir)/CommentLines)\" />\n"
            "      </td><td>\n"
            "      <xsl:value-of select=\"sum(key(\'ModulesByModuleDir\', ModuleDir)/ModuleLines)\" />\n"
            "      </td></tr>\n"
            "  </xsl:for-each>\n"
            "  </table>\n"


// Output the individual modules table
            "  <h2>\n"
            "    <xsl:text>Module Lines</xsl:text>\n"
            "  </h2>\n"
            "    <table border=\"1\">\n"
            "      <tr>\n"
            "        <th>Module Name</th>\n"
            "        <th>Module Dir</th>\n"
            "        <th>Code Lines</th>\n"
            "        <th>Comment Lines</th>\n"
            "        <th>Module Lines</th>\n"
            "      </tr>\n"
            "      <xsl:apply-templates select=\"LineStatisticsReport/Module\">\n"
            "        <xsl:sort select=\"ModuleLines\" data-type=\"number\" />\n"
            "      </xsl:apply-templates>\n"
            "    </table>\n"

            "      </body>\n"
            "    </html>\n"
            "  </xsl:template>\n"
            "\n"

// Template function SumCodeLines
            "  <xsl:template name=\"SumCodeLines\">\n"
            "    <xsl:value-of select='sum(//CodeLines)'/>\n"
            "  </xsl:template>\n"

            "  <xsl:template name=\"SumCommentLines\">\n"
            "    <xsl:value-of select='sum(//CommentLines)'/>\n"
            "  </xsl:template>\n"

// The Module template match
            "  <xsl:template match=\"Module\">\n"
            "    <tr>\n"
            "      <td>\n"
            "        <xsl:value-of select=\"ModuleName\" />\n"
            "      </td>\n"
            "      <td>\n"
            "        <xsl:value-of select=\"ModuleDir\" />\n"
            "      </td>\n"
            "      <td>\n"
            "        <xsl:value-of select=\"CodeLines\" />\n"
            "      </td>\n"
            "      <td>\n"
            "        <xsl:value-of select=\"CommentLines\" />\n"
            "      </td>\n"
            "      <td>\n"
            "        <xsl:value-of select=\"ModuleLines\" />\n"
            "      </td>\n"
            "    </tr>\n"
            "  </xsl:template>\n"

            "</xsl:stylesheet>\n";
    File transformFile(fullPath, "w");
    fprintf(transformFile.getFp(), "%s", text);
    }

static OovString getRelativeFileName(OovString const &fullFn)
    {
    FilePath srcDir(Project::getSourceRootDirectory(), FP_Dir);
    return Project::getSrcRootDirRelativeSrcFileName(fullFn, srcDir);
    }

bool createLineStatsFile(ModelData const &modelData, std::string &fn)
    {
    FilePath fp(Project::getProjectDirectory(), FP_Dir);
    fp.appendDir("output");
    fp.appendFile("LineStatistics");

    FileEnsurePathExists(fp);
    createLineStatsStyleTransform(fp + ".xslt");

    fp.appendFile(".xml");
    fn = fp;
    File useFile(fp, "w");
    if(useFile.isOpen())
        {
        static const char *header =
                "<?xml version=\"1.0\"?>\n"
                "<?xml-stylesheet type=\"text/xsl\" href=\"LineStatistics.xslt\"?>\n"
                "<LineStatisticsReport>\n";
        fprintf(useFile.getFp(), "%s", header);
        for(auto const &module : modelData.mModules)
            {
            static const char *item =
                "  <Module>\n"
                "    <ModuleName>%s</ModuleName>\n"
                "    <ModuleDir>%s</ModuleDir>\n"
                "    <CodeLines>%d</CodeLines>\n"
                "    <CommentLines>%d</CommentLines>\n"
                "    <ModuleLines>%d</ModuleLines>\n"
                "  </Module>\n";
            FilePath modName(getRelativeFileName(module->getName()), FP_File);
            fprintf(useFile.getFp(), item,
                modName.getNameExt().makeXml().c_str(),
                modName.getDrivePath().makeXml().c_str(),
                module.get()->mLineStats.mNumCodeLines,
                module.get()->mLineStats.mNumCommentLines,
                module.get()->mLineStats.mNumModuleLines);
            }

        static const char *footer = "</LineStatisticsReport>\n";
        fprintf(useFile.getFp(), "%s", footer);
        }
    return useFile.isOpen();
    }
