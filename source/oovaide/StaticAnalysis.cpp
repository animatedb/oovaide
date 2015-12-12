/*
 * StaticAnalysis.cpp
 *
 *  Created on: Nov 24, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "StaticAnalysis.h"
#include "FilePath.h"
#include "Project.h"
#include "IncludeMap.h"
#include "XmlWriter.h"
#include "OovError.h"
#include <algorithm>

class XslFile
    {
    public:
        XslFile():
            mHeader(&mRoot), mStyleSheet(&mRoot), mOut(&mStyleSheet)
            {
            }
        XML::XslStyleSheet &getStyleSheet()
            { return mStyleSheet; }
        OovStatus writeXsl(const std::string &fullPath, char const *err);

    private:
        XML::XmlRoot mRoot;
        XML::XmlHeader mHeader;
        XML::XslStyleSheet mStyleSheet;
            XML::XslOutputHtml mOut;
    };

OovStatus XslFile::writeXsl(const std::string &fullPath, char const *err)
    {
    File transformFile;
    OovStatus status = transformFile.open(fullPath, "w");
    if(status.ok())
        {
        XML::XmlWriter writer;
        mRoot.writeElementAndChildren(writer);
        status = transformFile.putString(writer.getStr());
        }
    if(status.needReport())
        {
        OovString str = err;
        str += fullPath;
        status.report(ET_Error, str);
        }
    return status;
    }

static bool createMemberVarUsageStyleTransform(const std::string &fullPath)
    {
    using namespace XML;

    XslFile xslFile;
        XslTemplate tplRoot(&xslFile.getStyleSheet(), "match=\"/\"");
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
                        OovStringRef linkStr = "http://oovaide.sourceforge.net/articles/DeadCode.html";
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
        XslTemplate tplAttr(&xslFile.getStyleSheet(), "match=\"Attr\"");
            TableRow rowVal(&tplAttr);
                TableCol colClass(&rowVal);
                    XslValueOf valClass(&colClass, "select=\"ClassName\"");
                TableCol colAttr(&rowVal);
                    XslValueOf valAttr(&colAttr, "select=\"AttrName\"");
                TableCol colUse(&rowVal);
                    XslValueOf valUseCount(&colUse, "select=\"UseCount\"");
                TableCol colAllUse(&rowVal);
                    XslValueOf valAllCount(&colAllUse, "select=\"AllUseCount\"");

    OovStatus status = xslFile.writeXsl(fullPath, "Unable to write member transform: ");
    return status.ok();
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
    TaskTimedBusyDialog progressDlg(parentWindow, model.mTypes.size(),
        "Searching Operations");
    while(progressDlg.keepGoing())
        {
        ModelType const *modelType = model.mTypes[progressDlg.getIndex()].get();
        ModelClassifier const *classifier = ModelType::getClass(modelType);
        if(classifier)
            {
            for(auto const &oper : classifier->getOperations())
                {
                appendAttrCounts(model, *oper, attrCounts);
                }
            }
        }
    }

bool StaticAnalysis::createMemberVarUsageFile(GtkWindow *parentWindow,
        ModelData const &modelData, std::string &fn)
    {
    FilePath fp = Project::getOutputDir();
    fp.appendFile("MemberVarUsage");

    bool successXsl = createMemberVarUsageStyleTransform(fp + ".xslt");
    // Attempt to create the xml file anyway.

    fp.appendFile(".xml");
    fn = fp;
    File useFile;
    OovStatus status = useFile.open(fp, "w");
    if(status.ok())
        {
        static const char *header =
                "<?xml version=\"1.0\"?>\n"
                "<?xml-stylesheet type=\"text/xsl\" href=\"MemberVarUsage.xslt\"?>\n"
                "<MemberAttrUseReport>\n";
        status = useFile.putString(header);
        }
    if(status.ok())
        {
        // Find the counts of all attributes.
        ModelAttrCounts attrCounts;
        getAllAttrCounts(parentWindow, modelData, attrCounts);

        for(auto const &type : modelData.mTypes)
            {
            ModelClassifier *classifier = ModelClassifier::getClass(type.get());
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
                    if(fprintf(useFile.getFp(), item,
                        classifier->getName().makeXml().c_str(),
                        attr->getName().c_str(), usageCount, allUseCount) < 0)
                        {
                        status.set(false, SC_File);
                        break;
                        }
                    }
                }
            }
        if(status.ok())
            {
            status = useFile.putString("</MemberAttrUseReport>\n");
            }
        }
    if(status.needReport())
        {
        OovString str = "Unable to write member usage: ";
        str += fn;
        status.report(ET_Error, str);
        }
    return status.ok() && successXsl;
    }


static bool createMethodUsageStyleTransform(const std::string &fullPath)
    {
    using namespace XML;

    XslFile xslFile;
        XslTemplate tplRoot(&xslFile.getStyleSheet(), "match=\"/\"");
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
                        OovStringRef linkStr = "http://oovaide.sourceforge.net/articles/DeadCode.html";
                        XslAttribute attr(&link, "name=\"href\"", linkStr);
                        XslText linkText(&link, "http://oovaide.sourceforge.net/articles/DeadCode.html");
                    XslText explRemText(&body, " for more information.");
                    Table table(&body);
                        TableRow rowHead(&table);
                            TableHeader rowClass(&rowHead, "Class Name");
                            TableHeader rowAttr(&rowHead, "Attribute Name");
                            TableHeader rowType(&rowHead, "Type");
                            TableHeader rowCount(&rowHead, "All Use Count");
                            XslApplyTemplates app(&table, "select=\"MethodUseReport/Oper\"");
                                XslSort sort(&app, "select=\"UseCount\" data-type=\"number\"");
        XslTemplate tplAttr(&xslFile.getStyleSheet(), "match=\"Oper\"");
            TableRow rowVal(&tplAttr);
                TableCol colClass(&rowVal);
                    XslValueOf valClass(&colClass, "select=\"ClassName\"");
                TableCol colOper(&rowVal);
                    XslValueOf valOper(&colOper, "select=\"OperName\"");
                TableCol colType(&rowVal);
                    XslValueOf valType(&colType, "select=\"Type\"");
                TableCol colCount(&rowVal);
                    XslValueOf valCount(&colCount, "select=\"UseCount\"");

    OovStatus status = xslFile.writeXsl(fullPath, "Unable to write method transform: ");
    return status.ok();
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
    TaskTimedBusyDialog progressDlg(parentWindow, model.mTypes.size(),
        "Searching Operations");
    while(progressDlg.keepGoing())
        {
        ModelType const *modelType = model.mTypes[progressDlg.getIndex()].get();
        ModelClassifier const *classifier = ModelType::getClass(modelType);
        if(classifier)
            {
            for(auto const &oper : classifier->getOperations())
                {
                appendOperationCounts(model, *oper, operCounts);
                }
            }
        }
    }

bool StaticAnalysis::createMethodUsageFile(GtkWindow *parentWindow,
        ModelData const &model, std::string &fn)
    {
    FilePath fp = Project::getOutputDir();
    fp.appendFile("MethodUsage");

    createMethodUsageStyleTransform(fp + ".xslt");

    fp.appendFile(".xml");
    fn = fp;
    File useFile;
    OovStatus status = useFile.open(fp, "w");
    if(status.ok())
        {
        static const char *header =
                "<?xml version=\"1.0\"?>\n"
                "<?xml-stylesheet type=\"text/xsl\" href=\"MethodUsage.xslt\"?>\n"
                "<MethodUseReport>\n";
        status = useFile.putString(header);
        }
    if(status.ok())
        {
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
                    if(fprintf(useFile.getFp(), item,
                        classifier->getName().makeXml().getStr(),
                        oper->getName().makeXml().getStr(), operTypeStr.getStr(),
                        usageCount) < 0)
                        {
                        status.set(false, SC_File);
                        break;
                        }
                    }
                }
            }
        if(status.ok())
            {
            status = useFile.putString("</MethodUseReport>\n");
            }
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to write method use data");
        }
    return status.ok();
    }


bool StaticAnalysis::createProjectStats(ModelData const &modelData, std::string &displayStr)
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
        ModelClassifier const *classifier = ModelClassifier::getClass(type.get());
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

    XslFile xslFile;
        XslKey key(&xslFile.getStyleSheet(),
            "name=\"ModulesByModuleDir\" match=\"Module\" use=\"ModuleDir\"");
        XslTemplate tplRoot(&xslFile.getStyleSheet(), "match=\"/\"");
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
        XslTemplate tplSumCodeLines(&xslFile.getStyleSheet(), "name=\"SumCodeLines\"");
            XslValueOf valSumCode(&tplSumCodeLines, "select='sum(//CodeLines)'");
        XslTemplate tplSumCommentLines(&xslFile.getStyleSheet(), "name=\"SumCommentLines\"");
            XslValueOf valSumComment(&tplSumCommentLines, "select='sum(//CommentLines)'");

        // The Module template match
        XslTemplate tplModule(&xslFile.getStyleSheet(), "match=\"Module\"");
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

    OovStatus status = xslFile.writeXsl(fullPath, "Unable to write lines transform: ");
    return status.ok();
    }

static OovString getRelativeFileName(OovString const &fullFn)
    {
    FilePath srcDir(Project::getSourceRootDirectory(), FP_Dir);
    return Project::getSrcRootDirRelativeSrcFileName(fullFn, srcDir);
    }

bool StaticAnalysis::createLineStatsFile(ModelData const &modelData, std::string &fn)
    {
    FilePath fp = Project::getOutputDir();
    fp.appendFile("LineStatistics");

    createLineStatsStyleTransform(fp + ".xslt");

    fp.appendFile(".xml");
    fn = fp;
    File useFile;
    OovStatus status = useFile.open(fp, "w");
    if(status.ok())
        {
        static const char *header =
                "<?xml version=\"1.0\"?>\n"
                "<?xml-stylesheet type=\"text/xsl\" href=\"LineStatistics.xslt\"?>\n"
                "<LineStatisticsReport>\n";
        status = useFile.putString(header);
        }
    if(status.ok())
        {
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
            if(fprintf(useFile.getFp(), item,
                modName.getNameExt().makeXml().c_str(),
                modName.getDrivePath().makeXml().c_str(),
                module.get()->mLineStats.mNumCodeLines,
                module.get()->mLineStats.mNumCommentLines,
                module.get()->mLineStats.mNumModuleLines) < 0)
                {
                status.set(false, SC_File);
                break;
                }
            }
        if(status.ok())
            {
            status = useFile.putString("</LineStatisticsReport>\n");
            }
        }
    return status.ok();
    }


static std::vector<ModelOperation const*> getModuleDefinedOperations(
    ModelData const &modelData, OovString const &consumerFile,
    std::set<ModelClassifier const*> &operClasses)
    {
    std::vector<ModelOperation const*> ops;
    for(auto const &type : modelData.mTypes)
        {
        ModelClassifier const *classifier = ModelType::getClass(type.get());
        if(classifier)
            {
            for(auto const &oper : classifier->getOperations())
                {
                if(oper->getModule())
                    {
                    if(consumerFile == oper->getModule()->getModulePath())
                        {
                        ops.push_back(oper.get());
                        operClasses.insert(classifier);
                        }
                    }
                }
            }
        }
    return ops;
    }

bool isProjectFile(OovStringRef file)
    {
    OovString const &srcDir = Project::getSourceRootDirectory();
    size_t len = srcDir.length();
    return(srcDir.compare(0, len, file, len) == 0);
    }

static void filterProjectSourceFiles(std::set<OovString> &projFiles)
    {
    // remove/erase idiom does not work on sets.
    //   projFiles.erase(std::remove_if(projFiles.begin(), projFiles.end(),
    //     isProjectFile), projFiles.end());
    for(auto it = projFiles.begin(); it != projFiles.end(); )
        {
        if(!isProjectFile(*it))
            {
            projFiles.erase(it++);
            }
        else
            {
            ++it;
            }
        }
    }

static void filterProjectIncFiles(std::set<IncludedPath> &incFiles)
    {
    // remove/erase idiom does not work on sets.
    for(auto it = incFiles.begin(); it != incFiles.end(); )
        {
        if(!isProjectFile((*it).getFullPath()))
            {
            incFiles.erase(it++);
            }
        else
            {
            ++it;
            }
        }
    }

static bool isTypeReferencedByOperations(ModelData const &modelData,
    std::vector<ModelOperation const*> const &moduleOps, ModelType const &type)
    {
    bool referenced = false;
    for(auto const &conOper : moduleOps)
        {
        if(modelData.isTypeReferencedByOperation(*conOper, type))
            {
            referenced = true;
            break;
            }
        }
    return referenced;
    }

static bool isTypeReferencedByOperClasses(
    std::set<ModelClassifier const*> &operClasses, ModelType const &type)
    {
    bool referenced = false;
    for(auto const &cls : operClasses)
        {
        if(cls == &type)
            {
            referenced = true;
            break;
            }
        }
    return referenced;
    }

static bool createIncludeTypeUsageStyleTransform(const std::string &fullPath)
    {
    using namespace XML;

    XslFile xslFile;
        XslTemplate tplRoot(&xslFile.getStyleSheet(), "match=\"/\"");
            Element html(&tplRoot, "html");
                Element head(&html, "head");
                    Element title(&head, "title");
                        XslText titleText(&title, "Include Types Usage Report");
                Element body(&html, "body");
                    Element headMem(&body, "h1");
                        XslText headText(&headMem, "Include Type Usage");
                    XslText outText(&body, "See the output directory for the "
                        "text file output. The count is the count of "
                         "the number of types used in the provider include file.");
                    Table table(&body);
                        TableRow rowHead(&table);
                            TableHeader rowConsModule(&rowHead, "Consumer");
                            TableHeader rowSuppModule(&rowHead, "Provider");
                            TableHeader rowCount(&rowHead, "Count");
                            XslApplyTemplates app(&table, "select=\"IncludeTypeUseReport/Module\"");
                                XslSort sort(&app, "select=\"UseCount\" data-type=\"number\"");
        XslTemplate tplAttr(&xslFile.getStyleSheet(), "match=\"Module\"");
            TableRow rowVal(&tplAttr);
                TableCol colClass(&rowVal);
                    XslValueOf valConsModule(&colClass, "select=\"ConsumerModule\"");
                TableCol colOper(&rowVal);
                    XslValueOf valSuppModule(&colOper, "select=\"SupplierModule\"");
                TableCol colCount(&rowVal);
                    XslValueOf valCount(&colCount, "select=\"UseCount\"");

    OovStatus status = xslFile.writeXsl(fullPath, "Unable to write method transform: ");
    return status.ok();
    }


bool StaticAnalysis::createIncludeTypeUsageFile(GtkWindow *parentWindow,
    ModelData const &modelData, IncDirDependencyMapReader const &incMap,
    std::string &fn)
    {
    FilePath fp = Project::getOutputDir();
    fp.appendFile("IncludeUsage");

    createIncludeTypeUsageStyleTransform(fp + ".xslt");

    fp.appendFile(".xml");
    fn = fp;
    File useFile;
    OovStatus status = useFile.open(fp, "w");
    if(status.ok())
        {
        static const char *header =
                "<?xml version=\"1.0\"?>\n"
                "<?xml-stylesheet type=\"text/xsl\" href=\"IncludeUsage.xslt\"?>\n"
                "<IncludeTypeUseReport>\n";
        status = useFile.putString(header);
        }
    typedef std::multimap<OovString, ModelClassifier const*> ModuleType;
    ModuleType moduleTypeMap;
    typedef std::pair<ModuleType::const_iterator, ModuleType::const_iterator> moduleTypeRange;
    if(status.ok())
        {
        // Build an intermediate module/type map for efficiency
        for(auto const &type : modelData.mTypes)
            {
            ModelClassifier const *classifier = ModelType::getClass(type.get());
            if(classifier)
                {
                ModelModule const *module = classifier->getModule();
                if(module)
                    {
                    moduleTypeMap.insert(std::pair<OovString, ModelClassifier const*>(
                        module->getModulePath(), classifier));
                    }
                }
            }
        std::set<OovString> projFiles = incMap.getAllFiles();
        filterProjectSourceFiles(projFiles);
        for(auto const &consumerFile : projFiles)
            {
            moduleTypeRange consumerTypesRange = moduleTypeMap.equal_range(
                consumerFile);
            std::set<IncludedPath> incFiles;
            incFiles.clear();
#define DEBUG_INC 0
#if(DEBUG_INC)
bool consDisplay = (consumerFile.find("EditorIpc") != std::string::npos);
if(consDisplay)
    {
    printf("--\n");
    fflush(stdout);
    }
#endif
            incMap.getImmediateIncludeFilesUsedBySourceFile(consumerFile, incFiles);
            std::set<ModelClassifier const*> modelDefOperClasses;
            modelDefOperClasses.clear();
            std::vector<ModelOperation const*> moduleDefOps = getModuleDefinedOperations(
                 modelData, consumerFile, modelDefOperClasses);
#if(DEBUG_INC)
if(consDisplay)
    {
    printf("%s defined types %d, operations %d\n", consumerFile.getStr(),
            moduleTypeMap.count(consumerFile), moduleDefOps.size());
    for(auto const &oper : moduleDefOps)
        {
        printf("  O: %s\n", oper->getName().getStr());
        }
    for(auto const &cls : modelDefOperClasses)
        {
        printf("  C: %s\n", cls->getName().getStr());
        }
    fflush(stdout);
    }
#endif
            filterProjectIncFiles(incFiles);
            for(auto const &supplierFile : incFiles)
                {
#if(DEBUG_INC)
bool supDisplay = consDisplay &&
    (supplierFile.getFullPath().find("OovIpc") != std::string::npos);
#endif
                std::set<ModelClassifier const*> usedTypes;
                // Output the counts.
                moduleTypeRange supplierTypesRange = moduleTypeMap.equal_range(
                    supplierFile.getFullPath());
                // Go through all types in the module to find used types.
                for(auto conIt = consumerTypesRange.first;
                    conIt != consumerTypesRange.second; conIt++)
                    {
                    ModelClassifier const &conClass = *(*conIt).second;
                    for(auto supIt = supplierTypesRange.first;
                        supIt != supplierTypesRange.second; supIt++)
                        {
                        ModelClassifier const &supClass = *(*supIt).second;
                        bool ref = modelData.isTypeReferencedByClassAttributes(
                            conClass, *(*supIt).second);
                        if(!ref)
                            {
                            // Go through function declarations to find used types.
                            ref = modelData.isTypeReferencedByClassOperationInterfaces(conClass,
                                *(*supIt).second);
                            }
                        if(!ref)
                            {
                            ref = modelData.isTypeReferencedByParentClass(conClass, supClass);
                            }
                        if(ref)
                            {
                            usedTypes.insert(&supClass);
                            }
                        }
                    }
                // Go through all functions defined in the module to find used types.
                for(auto supIt = supplierTypesRange.first;
                    supIt != supplierTypesRange.second; supIt++)
                    {
                    ModelClassifier const &supClass = *(*supIt).second;
                    if(isTypeReferencedByOperations(modelData, moduleDefOps, supClass) ||
                        isTypeReferencedByOperClasses(modelDefOperClasses, supClass))
                        {
                        usedTypes.insert(&supClass);
                        }
                    }
#if(DEBUG_INC)
if(supDisplay)
    {
    printf("  %s %d %d\n\n", supplierFile.getFullPath().getStr(),
        moduleTypeMap.count(supplierFile.getFullPath()), usedTypes.size());
    moduleTypeRange supplierTypesRange = moduleTypeMap.equal_range(
        supplierFile.getFullPath());
    for(auto supIt = supplierTypesRange.first;
        supIt != supplierTypesRange.second; supIt++)
        {
        printf("    supplier types %s\n", (*supIt).second->getName().getStr());
        }
    fflush(stdout);
    }
#endif
                static const char *item =
                    "  <Module>\n"
                    "    <ConsumerModule>%s</ConsumerModule>\n"
                    "    <SupplierModule>%s</SupplierModule>\n"
                    "    <UseCount>%d</UseCount>\n"
                    "  </Module>\n";
                /// @todo - add error checking
                if(fprintf(useFile.getFp(), item, consumerFile.getStr(),
                    supplierFile.getFullPath().getStr(), usedTypes.size()) < 0)
                    {
                    status.set(false, SC_File);
                    break;
                    }
                }
            }
        if(status.ok())
            {
            status = useFile.putString("</IncludeTypeUseReport>\n");
            }
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to write include type use data");
        }
    return status.ok();
    }
