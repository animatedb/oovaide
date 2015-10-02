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
#include "OovError.h"


static bool createMemberVarUsageStyleTransform(const std::string &fullPath)
    {
    using namespace XML;

    XmlRoot root;
    XmlHeader header(&root);
    XslStyleSheet ss(&root);
        XslOutputHtml out(&ss);
        XslTemplate tplRoot(&ss, "match=\"/\"");
            Element html(&tplRoot, "html");
                Element head(&html, "head");
                    Element title(&head, "title");
                        XslText titleText(&title, "Data Member Attribute Usage Report");
                Element body(&html, "body");
                    Element headMem(&body, "h1");
                        XslText headText(&headMem, "Data Member Usage");
                    XslText outText(&body, "See the output directory for the text file "
                        "output. The class usage count is the count of the "
                        "number of methods in the same class that refer to the "
                        " attribute. See");
                    XslElement link(&body, "name=\"a\"");
                        OovStringRef linkStr = "http://oovcde.sourceforge.net/articles/DeadCode.html";
                        XslAttribute attr(&link, "name=\"href\"", linkStr);
                        XslText linkText(&link, linkStr);
                    XslText explRemText(&body, " for more information.");
                    Table table(&body);
                        TableRow rowHead(&table);
                            TableHeader rowClass(&rowHead, "Class Name");
                            TableHeader rowAttr(&rowHead, "Attribute Name");
                            TableHeader rowUseCount(&rowHead, "Class Use Count");
                            TableHeader rowAllCount(&rowHead, "All Use Count");
                        XslApplyTemplates app(&table, "select=\"MemberAttrUseReport/Attr\"");
                            XslSort sort(&app, "select=\"AllUseCount\" data-type=\"number\"");
        XslTemplate tplAttr(&ss, "match=\"Attr\"");
            TableRow rowVal(&tplAttr);
                TableCol colClass(&rowVal);
                    XslValueOf valClass(&colClass, "select=\"ClassName\"");
                TableCol colAttr(&rowVal);
                    XslValueOf valAttr(&colAttr, "select=\"AttrName\"");
                TableCol colUse(&rowVal);
                    XslValueOf valUseCount(&colUse, "select=\"UseCount\"");
                TableCol colAllUse(&rowVal);
                    XslValueOf valAllCount(&colAllUse, "select=\"AllUseCount\"");

    File transformFile(fullPath, "w");
    XmlWriter writer;
    root.writeElementAndChildren(writer);
    bool success = transformFile.putString(writer.getStr());
    if(!success)
        {
        OovString str = "Unable to write member transform: ";
        str += fullPath;
        OovError::report(ET_Error, str);
        }
    return success;
    }

/// Keeps counts of member attribute values.
typedef std::map<class ModelAttribute const *, int> ModelAttrCounts;

/// Appends the immediate attributes that are referenced by this operation.
static void appendAttrCounts(ModelData const &model,
        ModelOperation const &srchOper, ModelAttrCounts &attrCounts)
    {
    // This eliminates increasing the count for multiple statements so
    // that the count only increases once per operation.
    std::set<ModelAttribute const *> usedAttrs;
    for(auto const &stmt : srchOper.getStatements())
        {
        if(stmt.getStatementType() == ST_VarRef ||
                stmt.getStatementType() == ST_Call)
            {
            ModelType const *modelType = stmt.getClassDecl().getDeclType();
            ModelClassifier const *classifier = ModelType::getClass(modelType);
            if(classifier)
                {
                usedAttrs.insert(classifier->getAttribute(stmt.getAttrName()));
                }
            }
        }
    for(auto const &attr : usedAttrs)
        {
        auto const &it = attrCounts.find(attr);
        if(it != attrCounts.end())
            {
            it->second++;
            }
        else
            {
            attrCounts.insert(std::pair<ModelAttribute const *, int>(attr, 1));
            }
        }
    }

static void getAllAttrCounts(GtkWindow *parentWindow,
    ModelData const &model, ModelAttrCounts &attrCounts)
    {
    TaskBusyDialog progressDlg;
    progressDlg.setParentWindow(parentWindow);
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
                appendAttrCounts(model, *oper, attrCounts);
                }
            }
        }
    progressDlg.endTask();
    }

bool createMemberVarUsageStaticAnalysisFile(GtkWindow *parentWindow,
        ModelData const &modelData, std::string &fn)
    {
    FilePath fp = Project::getOutputDir();
    fp.appendFile("MemberVarUsage");

    bool success = createMemberVarUsageStyleTransform(fp + ".xslt");
    // Attempt to create the xml file anyway.
    success = false;

    fp.appendFile(".xml");
    fn = fp;
    File useFile(fp, "w");
    if(useFile.isOpen())
        {
        static const char *header =
                "<?xml version=\"1.0\"?>\n"
                "<?xml-stylesheet type=\"text/xsl\" href=\"MemberVarUsage.xslt\"?>\n"
                "<MemberAttrUseReport>\n";
        success = useFile.putString(header);

        // Find the counts of all attributes.
        ModelAttrCounts attrCounts;
        getAllAttrCounts(parentWindow, modelData, attrCounts);

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
                        if(oper->getStatements().checkAttrUsed(classifier,
                                attr->getName()))
                            {
                            usageCount++;
                            }
                        }
                    auto const &attrIter = attrCounts.find(attr.get());
                    int allUseCount = 0;
                    if(attrIter != attrCounts.end())
                        {
                        allUseCount = (*attrIter).second;
                        }
                    static const char *item =
                        "  <Attr>\n"
                        "    <ClassName>%s</ClassName>\n"
                        "    <AttrName>%s</AttrName>\n"
                        "    <UseCount>%d</UseCount>\n"
                        "    <AllUseCount>%d</AllUseCount>\n"
                        "  </Attr>\n";
                    /// @todo - add error checking
                    fprintf(useFile.getFp(), item,
                        classifier->getName().makeXml().c_str(),
                        attr->getName().c_str(), usageCount, allUseCount);
                    }
                }
            }
        if(success)
            {
            success = useFile.putString("</MemberAttrUseReport>\n");
            }
        }
    if(!success)
        {
        OovString str = "Unable to write member usage: ";
        str += fn;
        OovError::report(ET_Error, str);
        }
    return success;
    }


static void createMethodUsageStyleTransform(const std::string &fullPath)
    {
    using namespace XML;

    XmlRoot root;
    XmlHeader header(&root);
    XslStyleSheet ss(&root);
        XslOutputHtml out(&ss);
        XslTemplate tplRoot(&ss, "match=\"/\"");
            Element html(&tplRoot, "html");
                Element head(&html, "head");
                    Element title(&head, "title");
                        XslText titleText(&title, "Method Usage Report");
                Element body(&html, "body");
                    Element headMem(&body, "h1");
                        XslText headText(&headMem, "Method Usage");
                    XslText outText(&body, "See the output directory for the "
                        "text file output. The all usage count is the count of "
                         "usage by all other methods. See ");
                    XslElement link(&body, "name=\"a\"");
                        OovStringRef linkStr = "http://oovcde.sourceforge.net/articles/DeadCode.html";
                        XslAttribute attr(&link, "name=\"href\"", linkStr);
                        XslText linkText(&link, "http://oovcde.sourceforge.net/articles/DeadCode.html");
                    XslText explRemText(&body, " for more information.");
                    Table table(&body);
                        TableRow rowHead(&table);
                            TableHeader rowClass(&rowHead, "Class Name");
                            TableHeader rowAttr(&rowHead, "Attribute Name");
                            TableHeader rowType(&rowHead, "Type");
                            TableHeader rowCount(&rowHead, "All Use Count");
                            XslApplyTemplates app(&table, "select=\"MethodUseReport/Oper\"");
                                XslSort sort(&app, "select=\"UseCount\" data-type=\"number\"");
        XslTemplate tplAttr(&ss, "match=\"Oper\"");
            TableRow rowVal(&tplAttr);
                TableCol colClass(&rowVal);
                    XslValueOf valClass(&colClass, "select=\"ClassName\"");
                TableCol colOper(&rowVal);
                    XslValueOf valOper(&colOper, "select=\"OperName\"");
                TableCol colType(&rowVal);
                    XslValueOf valType(&colType, "select=\"Type\"");
                TableCol colCount(&rowVal);
                    XslValueOf valCount(&colCount, "select=\"UseCount\"");

    File transformFile(fullPath, "w");
    XmlWriter writer;
    root.writeElementAndChildren(writer);
    bool success = transformFile.putString(writer.getStr());
    if(!success)
        {
        OovString str = "Unable to write member transform: ";
        str += fullPath;
        OovError::report(ET_Error, str);
        }
    }




/// Keeps counts of operations.
typedef std::map<class ModelOperation const *, int> ModelOperationCounts;

/// Appends the immediate operations that are called by this operation.
static void appendOperationCounts(ModelData const &model,
        ModelOperation const &srchOper, ModelOperationCounts &operCounts)
    {
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

static void getAllOperationCounts(GtkWindow *parentWindow,
    ModelData const &model, ModelOperationCounts &operCounts)
    {
    TaskBusyDialog progressDlg;
    progressDlg.setParentWindow(parentWindow);
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
    }

bool createMethodUsageStaticAnalysisFile(GtkWindow *parentWindow,
        ModelData const &model, std::string &fn)
    {
    FilePath fp = Project::getOutputDir();
    fp.appendFile("MethodUsage");

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

        // Find the counts of all operations.
        ModelOperationCounts operCounts;
        getAllOperationCounts(parentWindow, model, operCounts);

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
                    OovString operTypeStr = "";
                    if(oper->getName().find('~') != std::string::npos)
                        operTypeStr = "destr";
                    else if(classifier->getName() == oper->getName())
                        operTypeStr = "constr";
                    else if(oper->isVirtual())
                        operTypeStr = "virt";
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

static bool createLineStatsStyleTransform(const std::string &fullPath)
    {
    using namespace XML;

    XmlRoot root;
    XmlHeader header(&root);
    XslStyleSheet ss(&root);
        XslOutputHtml out(&ss);
        XslKey key(&ss, "name=\"ModulesByModuleDir\" match=\"Module\" use=\"ModuleDir\"");
        XslTemplate tplRoot(&ss, "match=\"/\"");
            Element html(&tplRoot, "html");
                Element head(&html, "head");
                    Element title(&head, "title");
                        XslText titleText(&title, "Line Statistics Report");
                Element body(&html, "body");
                Element headMem(&body, "h1");
                    XslText headText(&headMem, "Line Statistics");
                XslText outText(&body, "Note that code and comments can be "
                    "on the same line, and will be counted in both counts.");
                Element pout(&body, "p");

                // Output the total project stats
                XslText code(&body, "Total code lines:");
                XslCallTemplate callCode(&body, "name=\"SumCodeLines\"");
                Element brcode(&body, "br");
                XslText comment(&body, "Total comment lines:");
                XslCallTemplate commentCode(&body, "name=\"SumCommentLines\"");
                Element pcomment(&body, "p");

                // Output the directory lines table
                Element headDirMem(&body, "h2");
                    XslText dirLines(&headDirMem, "Directory Lines");
                Table dirTable(&body);
                    TableRow dirHead(&dirTable);
                        TableHeader dirDir(&dirHead, "Module Dir");
                        TableHeader dirCode(&dirHead, "Code Lines");
                        TableHeader dirComment(&dirHead, "Comment Lines");
                        TableHeader dirModule(&dirHead, "Module Lines");
                    XslForEach forEach(&dirTable, "select=\"LineStatisticsReport/Module[count(. | "
                        "key(\'ModulesByModuleDir\', ModuleDir)[1]) = 1]\"");
                        XslSort sort(&forEach, "select=\"sum(key(\'ModulesByModuleDir\', ModuleDir)/ModuleLines)\" "
                            "data-type=\"number\" order=\"ascending\"");
                        TableRow rowVal(&forEach);
                            TableCol dirModuleCol(&rowVal);
                                XslValueOf valDirModule(&dirModuleCol, "select=\"ModuleDir\"");
                            TableCol dirCodeCol(&rowVal);
                                XslValueOf valDirCode(&dirCodeCol,
                                    "select=\"sum(key(\'ModulesByModuleDir\', ModuleDir)/CodeLines)\"");
                            TableCol dirCommentCol(&rowVal);
                                XslValueOf valDirComment(&dirCommentCol,
                                    "select=\"sum(key(\'ModulesByModuleDir\', ModuleDir)/CommentLines)\"");
                            TableCol dirModLinesCol(&rowVal);
                                XslValueOf valDirModLines(&dirModLinesCol,
                                    "select=\"sum(key(\'ModulesByModuleDir\', ModuleDir)/ModuleLines)\"");

                // Output the individual modules table
                Element headModMem(&body, "h2");
                    XslText dirModLines(&headModMem, "Module Lines");
                Table modTable(&body);
                    TableRow modHead(&modTable);
                        TableHeader modName(&modHead, "Module Name");
                        TableHeader modDir(&modHead, "Module Dir");
                        TableHeader modCode(&modHead, "Code Lines");
                        TableHeader modComment(&modHead, "Comment Lines");
                        TableHeader modModule(&modHead, "Module Lines");
                        XslApplyTemplates app(&modTable, "select=\"LineStatisticsReport/Module\"");
                            XslSort sortMod(&app, "select=\"ModuleLines\" data-type=\"number\"");

        // Template function SumCodeLines
        XslTemplate tplSumCodeLines(&ss, "name=\"SumCodeLines\"");
            XslValueOf valSumCode(&tplSumCodeLines, "select='sum(//CodeLines)'");
        XslTemplate tplSumCommentLines(&ss, "name=\"SumCommentLines\"");
            XslValueOf valSumComment(&tplSumCommentLines, "select='sum(//CommentLines)'");

        // The Module template match
        XslTemplate tplModule(&ss, "match=\"Module\"");
            TableRow moduleRow(&tplModule);
                TableCol modNameCol(&moduleRow);
                    XslValueOf modNameVal(&modNameCol, "select=\"ModuleName\"");
                TableCol modDirCol(&moduleRow);
                    XslValueOf modDirVal(&modDirCol, "select=\"ModuleDir\"");
                TableCol modCodeCol(&moduleRow);
                    XslValueOf modCodeVal(&modCodeCol, "select=\"CodeLines\"");
                TableCol modCommentCol(&moduleRow);
                    XslValueOf modCommentVal(&modCommentCol, "select=\"CommentLines\"");
                TableCol modTotalCol(&moduleRow);
                    XslValueOf modTotalVal(&modTotalCol, "select=\"ModuleLines\"");

    File transformFile(fullPath, "w");
    XmlWriter writer;
    root.writeElementAndChildren(writer);
    bool success = transformFile.putString(writer.getStr());
    if(!success)
        {
        OovString str = "Unable to write lines transform: ";
        str += fullPath;
        OovError::report(ET_Error, str);
        }
    return success;
    }

static OovString getRelativeFileName(OovString const &fullFn)
    {
    FilePath srcDir(Project::getSourceRootDirectory(), FP_Dir);
    return Project::getSrcRootDirRelativeSrcFileName(fullFn, srcDir);
    }

bool createLineStatsFile(ModelData const &modelData, std::string &fn)
    {
    FilePath fp = Project::getOutputDir();
    fp.appendFile("LineStatistics");

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
