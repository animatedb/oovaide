/*
 * Complexity.cpp
 *
 *  Created on: Oct 23, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */
#include "Complexity.h"
#include "FilePath.h"
#include "Project.h"
#include "OovString.h"
#include <map>
#include <algorithm>
#include <string.h>

// This is pretty close to McCabe complexity.
static int calcMcCabeComplexity(ModelStatements const &stmts)
    {
    int complexity = 1;
    for(auto const &stmt : stmts)
	{
	if(stmt.getStatementType() == ST_OpenNest)
	    {
	    if(stmt.getCondName() != "[else]" && stmt.getCondName() != "[default]")
		{
		complexity++;
		}
	    }
	}
    return complexity;
    }

// This does not find typedefs for built in types
/*
static bool builtInNumericType(const char *str)
    {
    std::vector<class OovString> typeStrs = StringSplit(str, ' ');
    static char *builtinNumericTypeStrs
	{
//	These are not numeric types: "bool", wchar_t
	"char", "int", "float", "double"
	};
    }
*/

class OovComplexity
    {
    public:
	OovComplexity(ModelOperation const &oper);
	int calcComplexity();

    private:
	ModelOperation const &mOper;
	ModelStatements const &mStmts;
	size_t mStmtIndex;
	std::map<std::string, int> mParamList;	// str=param name, int=complexity
	std::map<std::string, int> mWriteMemberRef;	// str=param name, int=complexity

	int getControlComplexityRecurse();
	void makeParamList(ModelOperation const &oper);
	int getDataComplexity();
	int getDataFunctionCallComplexity();
	static int getOperationComplexity(const ModelOperation *oper);
	static bool isIdentPresent(OovStringRef const str, OovStringRef const ident);
	static int getDeclComplexity(ModelType const *type);
    };

OovComplexity::OovComplexity(ModelOperation const &oper):
    mOper(oper), mStmts(oper.getStatements()), mStmtIndex(0)
    {
    makeParamList(oper);
    }

int OovComplexity::getDeclComplexity(ModelType const *type)
    {
    int paramComplexity = 0;
    if(type)
	{
	if(type->getName() == "void")
	    {
	    // complexity is 0.
	    }
	else if(isIdentPresent(type->getName(), "bool"))
	    {
	    paramComplexity = 1;
	    }
	else if(isIdentPresent(type->getName(), "unsigned"))
	    {
	    paramComplexity = 2;
	    }
	else
	    {
	    paramComplexity = 3;
	    }
	}
    else
	{
	paramComplexity = 3;
	}
    return paramComplexity;
    }

void OovComplexity::makeParamList(ModelOperation const &oper)
    {
    for(auto const &param : oper.getParams())
	{
	mParamList[param->getName()] = getDeclComplexity(param->getDeclType());
	}
    }

bool OovComplexity::isIdentPresent(OovStringRef const str, OovStringRef const ident)
    {
    OovString testStr = " ";
    testStr += str;
    testStr += " ";
    bool present = false;
    size_t pos = 0;
    while(pos != std::string::npos)
	{
	pos = testStr.find(ident, pos);
	if(pos != std::string::npos)
	    {
	    if(!isalnum(testStr[pos-1]) && !isalnum(testStr[pos+strlen(ident)]))
		{
		present = true;
		break;
		}
	    pos++;
	    }
	}
    return present;
    }

int OovComplexity::getDataComplexity()
    {
    int complexity = 0;
    for(auto const &param : mParamList)
	{
	complexity += param.second;
	}
    for(auto const &param : mWriteMemberRef)
	{
	complexity += param.second;
	}
    complexity += getDataFunctionCallComplexity();
    return complexity;
    }

/// @todo - add return type
int OovComplexity::getOperationComplexity(const ModelOperation *oper)
    {
    int complexity = 0;
    for(auto const &param : oper->getParams())
	{
	if(!param->isConst() && param->isRefer())
	    {
	    complexity += getDeclComplexity(param->getDeclType());
	    }
	}
#if(OPER_RET_TYPE)
    complexity += getDeclComplexity(oper->getReturnType().getDeclType());
#endif
    return complexity;
    }

int OovComplexity::getDataFunctionCallComplexity()
    {
    int complexity = 0;
    struct operItem
	{
	operItem(const std::string &name, ModelClassifier const *cls, int complexity):
	    mName(name), mCls(cls), mComplexity(complexity)
	    {}
	void operator=(const operItem &oper)
	    {
	    mName = oper.mName;
	    mCls = oper.mCls;
	    mComplexity = oper.mComplexity;
	    }
	bool operator<(const operItem &oper) const
	    {
	    return(mCls < oper.mCls && mName < oper.mName);
	    }
	std::string mName;
	ModelClassifier const *mCls;
	int mComplexity;
	};
    std::set<operItem> opers;
    for(auto const &stmt : mStmts)
	{
	if(stmt.getStatementType() == ST_Call)
	    {
	    ModelClassifier const *cls = stmt.getClassDecl().getDeclType()->getClass();
	    if(cls)
		{
		const ModelOperation *oper = cls->getOperationAnyConst(
			stmt.getFuncName(), false);
		if(oper)
		    {
		    int opComplexity = getOperationComplexity(oper);
		    if(opComplexity > 0)
			{
			operItem item(stmt.getFuncName(), cls, opComplexity);
			opers.insert(item);
			}
		    }
		}
	    }
	}
    for(auto const &oper : opers)
	{
	complexity += oper.mComplexity;
	}
    return complexity;
    }


// This code uses a cheat that is not correct. It only checks that some number
// of expressions match. It does not know the difference between variables and
// constants.  This means that [v==2] and [v==3] are considered a match.  But
// the error is that [v1 == 2] and [v2 == 2] are also considered a match, even
// though they are not a match for determining combinatorial complexity.
// The CppParser must be modified to get more constant information, but this
// is somewhat difficult. This code should be better than nothing.

// This is for one if, while, etc.
class SingleStatementConditional
    {
    public:
	SingleStatementConditional(OovStringRef const condStr);
	// Returns true if the passed in condition is a subset of the this
	// condition.
	bool contains(SingleStatementConditional const &cond) const;

    private:
	// This contains all expressions on any side of a relation operator
	// or if no relations, contains the full cond.
	std::set<std::string> mSideExpressions;
	int mNumConditionalExpressions;
	void splitConditions(OovString const cond);
	OovString discardJunkChars(OovStringRef const str);
    };

SingleStatementConditional::SingleStatementConditional(OovStringRef const cond):
	mNumConditionalExpressions(0)
    {
    splitConditions(cond);
    }

bool SingleStatementConditional::contains(SingleStatementConditional const &cond) const
    {
    int matches = 0;
    for(auto const &expr : cond.mSideExpressions)
	{
	if(mSideExpressions.find(expr) != mSideExpressions.end())
	    {
	    matches++;
	    }
	}
    return(matches >= mNumConditionalExpressions);
    }

void SingleStatementConditional::splitConditions(OovString const fullCond)
    {
    // Remove first and last char []
    OovString cond = fullCond.substr(1, fullCond.size()-2);
    OovStringVec delims = { "&&", "||" };
    OovStringVec condExpressions = cond.split(delims);
    mNumConditionalExpressions = condExpressions.size();
    for(auto const &cond : condExpressions)
	{
	// Split strings based on relational operators.
	OovStringVec relDelims = { "==", "!=", ">=", "<=", ">", "<" };
	OovStringVec sideExprs = discardJunkChars(cond).split(relDelims);
	for(auto const &expr : sideExprs)
	    {
	    mSideExpressions.insert(expr);
	    }
	}
    }

OovString SingleStatementConditional::discardJunkChars(OovStringRef const str)
    {
    OovString shortStr = str;
    shortStr.replaceStrs(" ", "");
    shortStr.replaceStrs("!", "");
    return shortStr;
    }



class FunctionConditionals
    {
    public:
	bool isCombinatorial(std::string const &condition);
	void addConditional(std::string const &cond);

    private:
	bool contains(SingleStatementConditional const &cond) const;
	std::vector<SingleStatementConditional> mConditionals;
    };

bool FunctionConditionals::contains(SingleStatementConditional const &cond) const
    {
    bool hasit = false;
    for(auto const &funcCond : mConditionals)
	{
	if(funcCond.contains(cond))
	    {
	    hasit = true;
	    break;
	    }
	}
    return hasit;
    }

void FunctionConditionals::addConditional(std::string const &condStr)
    {
    SingleStatementConditional cond(condStr);
    if(!contains(cond))
	mConditionals.push_back(cond);
    }

// The first conditional in a sequence of statements is not combinatorial.
bool FunctionConditionals::isCombinatorial(std::string const &condStr)
    {
    SingleStatementConditional cond(condStr);
    return (mConditionals.size() > 0) && !contains(cond);
    }

int OovComplexity::getControlComplexityRecurse()
    {
    FunctionConditionals conditionals;
    int complexity = 0;
    if(mStmtIndex == 0)
	complexity++;	// Add one for main path.
    while(mStmtIndex<mStmts.size())
	{
	auto const &stmt = mStmts[mStmtIndex];
	mStmtIndex++;
	if(stmt.getStatementType() == ST_OpenNest)
	    {
	    complexity += getControlComplexityRecurse();
	    if(stmt.getCondName() != "[else]" && stmt.getCondName() != "[default]")
		{
		if(conditionals.isCombinatorial(stmt.getCondName()))
		    complexity += 2;
		else
		    complexity += 1;
		conditionals.addConditional(stmt.getCondName());
		// Don't double count a variable that is also used as a loop
		// conditional.
		for(auto &param : mParamList)
		    {
		    if(isIdentPresent(stmt.getCondName(), param.first)
			    && param.second > 0)
			{
			param.second--;
			break;
			}
		    }
		}
	    }
	else if(stmt.getStatementType() == ST_CloseNest)
	    {
	    break;
	    }
	if(stmt.getStatementType() == ST_VarRef)
	    {
	    // Reading adds complexity to function being analyzed.
	    if(stmt.getVarAccessWrite() == false)
		{
		mWriteMemberRef[stmt.getAttrName()] = getDeclComplexity(
			stmt.getVarDecl().getDeclType());
		}
	    }
	}
    return complexity;
    }

int OovComplexity::calcComplexity()
    {
    makeParamList(mOper);
    mStmtIndex = 0;
    int complexity = getControlComplexityRecurse();
    complexity += getDataComplexity();
    return complexity;
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
	    "          <xsl:text>Complexity Report</xsl:text>\n"
	    "        </title>\n"
	    "      </head>\n"
	    "      <body>\n"
	    "        <h1>\n"
	    "          <xsl:text>Complexity</xsl:text>\n"
	    "        </h1>\n"
	    "    See <xsl:element name=\"a\">\n"
	    "        <xsl:attribute name=\"href\">http://oovcde.sourceforge.net/articles/Complexity.shtml</xsl:attribute>\n"
	    "        <xsl:text>http://oovcde.sourceforge.net/articles/Complexity.shtml</xsl:text>\n"
	    "        </xsl:element> for more information.\n"
	    "        <table border=\"1\">\n"
	    "          <tr>\n"
	    "            <th>Class Name</th>\n"
	    "            <th>Operation Name</th>\n"
	    "            <th>McCabe<br/>Complexity</th>\n"
	    "            <th>Oovcde<br/>Complexity</th>\n"
	    "          </tr>\n"
	    "          <xsl:apply-templates select=\"ComplexityReport/Oper\">\n"
	    "	    <xsl:sort select=\"Oovcde\" data-type=\"number\" order=\"descending\" />\n"
	    "	  </xsl:apply-templates>\n"
	    "        </table>\n"
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
    createStyleTransform(fp + ".xslt");

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

		    OovComplexity oovComp(*oper);
		    int oov = oovComp.calcComplexity();

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


