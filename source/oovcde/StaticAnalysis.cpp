/*
 * StaticAnalysis.cpp
 *
 *  Created on: Nov 24, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "StaticAnalysis.h"
#include "FilePath.h"
#include "Project.h"
#include "XmlWriter.h"


static void createMemberVarUsageStyleTransform(const std::string &fullPath)
    {
    using namespace XML;

    Writer xml;
    {
    XmlHeader header(xml);
    }
    {
    XslStyleSheet ss(xml);
        {
        XslOutputHtml out(xml);
        }
        {
            {
            XslTemplate tplroot(xml, "match=\"/\"");
                {
                Element html(xml, "html");
                    {
                    Element head(xml, "head");
                        {
                        Element title(xml, "title");
                            { XslText(xml, "Data Member Attribute Usage Report"); }
                        }
                    }
                    {
                    Element body(xml, "body");
                        {
                            {
                            Element headmem(xml, "h1");
                                { XslText(xml, "Data Member Usage"); }
                            }
                            XslText(xml, "See the output directory for"
                                " the text file output.");
                            XslText(xml, "The usage count is the count "
                                "of the number of methods in the same class "
                                "that refer to the attribute.");
                        Table tab(xml);
                            {
                                {
                                TableRow rowhead(xml);
                                    { TableHeader(xml, "Class Name"); }
                                    { TableHeader(xml, "Attribute Name"); }
                                    { TableHeader(xml, "Use Count"); }
                                }
                                {
                                XslApplyTemplates app(xml, "select=\"MemberAttrUseReport/Attr\"");
                                    {
                                    XslSort(xml, "select=\"UseCount\" data-type=\"number\"");
                                    }
                                }
                            }
                        }
                    }
                }
            }
            {
            XslTemplate tplattr(xml, "match=\"Attr\"");
                {
                    {
                    TableRow rowval(xml);
                        {
                        TableCol colclass(xml);
                            { XslValueOf(xml, "select=\"ClassName\""); }
                        }
                        {
                        TableCol colattr(xml);
                            { XslValueOf(xml, "select=\"AttrName\""); }
                        }
                        {
                        TableCol coluse(xml);
                            { XslValueOf(xml, "select=\"UseCount\""); }
                        }
                    }
                }
            }
        }
    }
/*
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
            "             <xsl:sort select=\"UseCount\" data-type=\"number\" />\n"
            "          </xsl:apply-templates>\n"
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
*/
    File transformFile(fullPath, "w");
    fprintf(transformFile.getFp(), "%s", xml.getStr());
    }

bool createMemberVarUsageStaticAnalysisFile(ModelData const &modelData, std::string &fn)
    {
    FilePath fp(Project::getProjectDirectory(), FP_Dir);
    fp.appendDir("output");
    fp.appendFile("MemberVarUsage");

    FileEnsurePathExists(fp);
    createMemberVarUsageStyleTransform(fp + ".xslt");

    fp.appendFile(".xml");
    fn = fp;
    File useFile(fp, "w");
    if(useFile.isOpen())
        {
        static const char *header =
                "<?xml version=\"1.0\"?>\n"
                "<?xml-stylesheet type=\"text/xsl\" href=\"MemberVarUsage.xslt\"?>\n"
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


static void createMethodUsageStyleTransform(const std::string &fullPath)
    {
    using namespace XML;

    Writer xml;
    {
    XmlHeader header(xml);
    }
    {
    XslStyleSheet ss(xml);
        {
        XslOutputHtml out(xml);
        }
        {
            {
            XslTemplate tplroot(xml, "match=\"/\"");
                {
                Element html(xml, "html");
                    {
                    Element head(xml, "head");
                        {
                        Element title(xml, "title");
                            { XslText(xml, "Method Usage Report"); }
                        }
                    }
                    {
                    Element body(xml, "body");
                        {
                            {
                            Element headmem(xml, "h1");
                                { XslText(xml, "Method Usage"); }
                            }
                            XslText(xml, "See the output directory for"
                                " the text file output.");
                            XslText(xml, "The usage count is the global "
                                "count of usage by any other method.");
                        Table tab(xml);
                            {
                                {
                                TableRow rowhead(xml);
                                    { TableHeader(xml, "Class Name"); }
                                    { TableHeader(xml, "Attribute Name"); }
                                    { TableHeader(xml, "Type"); }
                                    { TableHeader(xml, "Use Count"); }
                                }
                                {
                                XslApplyTemplates app(xml, "select=\"MethodUseReport/Oper\"");
                                    {
                                    XslSort(xml, "select=\"UseCount\" data-type=\"number\"");
                                    }
                                }
                            }
                        }
                    }
                }
            }
            {
            XslTemplate tplattr(xml, "match=\"Oper\"");
                {
                    {
                    TableRow rowval(xml);
                        {
                        TableCol colclass(xml);
                            { XslValueOf(xml, "select=\"ClassName\""); }
                        }
                        {
                        TableCol colattr(xml);
                            { XslValueOf(xml, "select=\"OperName\""); }
                        }
                        {
                        TableCol colattr(xml);
                            { XslValueOf(xml, "select=\"Type\""); }
                        }
                        {
                        TableCol coluse(xml);
                            { XslValueOf(xml, "select=\"UseCount\""); }
                        }
                    }
                }
            }
        }
    }
    File transformFile(fullPath, "w");
    fprintf(transformFile.getFp(), "%s", xml.getStr());
    }


/// Keeps counts of operations.
typedef std::map<class ModelOperation const *, int> ModelOperationCounts;

/// Appends the immediate operations that are called by this operation.
static void appendOperationCounts(ModelData const &model,
        ModelOperation const &srchOper, ModelOperationCounts &operCounts)
    {
/*
if(srchOper.getName().find("name of func") != std::string::npos)
    {
    }
*/
    for(auto const &stmt : srchOper.getStatements())
        {
        if(stmt.getStatementType() == ST_Call)
            {
            ModelType const *modelType = stmt.getClassDecl().getDeclType();
            ModelClassifier const *classifier = ModelType::getClass(modelType);
            if(classifier)
                {
                ModelOperation const *calledOper = classifier->getMatchingOperation(
                    stmt);
                auto const &it = operCounts.find(calledOper);
                if(it != operCounts.end())
                    {
                    it->second++;
                    }
                else
                    {
                    operCounts.insert(std::pair<ModelOperation const *, int>(
                            calledOper, 1));
                    }
                }
            }
        }
    }

bool createMethodUsageStaticAnalysisFile(GtkWindow *parentWindow,
        ModelData const &model, std::string &fn)
    {
    FilePath fp(Project::getProjectDirectory(), FP_Dir);
    fp.appendDir("output");
    fp.appendFile("MethodUsage");

    FileEnsurePathExists(fp);
    createMethodUsageStyleTransform(fp + ".xslt");

    fp.appendFile(".xml");
    fn = fp;
    File useFile(fp, "w");
    if(useFile.isOpen())
        {
        static const char *header =
                "<?xml version=\"1.0\"?>\n"
                "<?xml-stylesheet type=\"text/xsl\" href=\"MethodUsage.xslt\"?>\n"
                "<MethodUseReport>\n";
        fprintf(useFile.getFp(), "%s", header);

        TaskBusyDialog progressDlg;
        progressDlg.setParentWindow(parentWindow);
        // Find the counts of all operations.
        ModelOperationCounts operCounts;
        size_t totalTypes = model.mTypes.size();
        progressDlg.startTask("Searching Operations", totalTypes);
        bool keepGoing = true;
        time_t updateTime = 0;
        for(size_t i=0; i<totalTypes && keepGoing; i++)
            {
            time_t curTime;
            time(&curTime);
            if(updateTime != curTime)
                {
                keepGoing = progressDlg.updateProgressIteration(i, nullptr, true);
                updateTime = curTime;
                }
            ModelType const *modelType = model.mTypes[i].get();
            ModelClassifier const *classifier = ModelType::getClass(modelType);
            if(classifier)
                {
                for(auto const &oper : classifier->getOperations())
                    {
                    appendOperationCounts(model, *oper, operCounts);
                    }
                }
            }
        progressDlg.endTask();

        // Output the counts.
        for(auto const &type : model.mTypes)
            {
            ModelClassifier *classifier = ModelType::getClass(type.get());
            if(classifier)
                {
                for(auto const &oper : classifier->getOperations())
                    {
                    int usageCount = 0;
                    auto const &it = operCounts.find(oper.get());
                    if(it != operCounts.end())
                        {
                        usageCount = (*it).second;
                        }
                    OovString operTypeStr = (oper->isVirtual()) ? "virt" : "";
                    static const char *item =
                        "  <Oper>\n"
                        "    <ClassName>%s</ClassName>\n"
                        "    <OperName>%s</OperName>\n"
                        "    <Type>%s</Type>\n"
                        "    <UseCount>%d</UseCount>\n"
                        "  </Oper>\n";
                    fprintf(useFile.getFp(), item,
                        classifier->getName().makeXml().getStr(),
                        oper->getName().makeXml().getStr(), operTypeStr.getStr(),
                        usageCount);
                    }
                }
            }

        static const char *footer = "</MethodUseReport>\n";
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
