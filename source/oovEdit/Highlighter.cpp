/*
 * Highlighter.cpp
 *
 *  Created on: Sep 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Highlighter.h"


class CXStringDisposer:public std::string
    {
    public:
	CXStringDisposer(const CXString &xstr):
	    std::string(clang_getCString(xstr))
	    {
	    clang_disposeString(xstr);
	    }
    };


void TokenRange::tokenize(CXTranslationUnit transUnit, CXFile srcFile /*,
	int startLine, int endLine*/)
    {
    CXCursor cursor = clang_getTranslationUnitCursor(transUnit);
    CXSourceRange range = clang_getCursorExtent(cursor);

    CXToken *tokens = 0;
    unsigned int numTokens = 0;
    clang_tokenize(transUnit, range, &tokens, &numTokens);
    resize(numTokens);
    for (size_t i = 0; i < numTokens; i++)
	{
	at(i).mTokenKind = clang_getTokenKind(tokens[i]);
	CXSourceRange tokRange = clang_getTokenExtent(transUnit, tokens[i]);
	clang_getExpansionLocation(clang_getRangeStart(tokRange), NULL, NULL,
		NULL, &at(i).mStartOffset);
	clang_getExpansionLocation(clang_getRangeEnd(tokRange), NULL, NULL,
		NULL, &at(i).mEndOffset);
	}
    clang_disposeTokens(transUnit, tokens, numTokens);
    }

Tokenizer::~Tokenizer()
    {
    // There is a bug somewhere that the wrong tokenizer is deleted.
    // Probably related to EditFiles::removeNotebookPage(GtkWidget *pageWidget)
    if(mTransUnit)
	clang_disposeTranslationUnit(mTransUnit);
    }

void Tokenizer::parse(char const * const fileName, char const * const buffer, int bufLen,
	char const * const clang_args[], int num_clang_args)
    {
    unsigned options = clang_defaultEditingTranslationUnitOptions();
    if(!mSourceFile)
	{
	mSourceFilename = fileName;
	CXIndex index = clang_createIndex(1, 1);
	mTransUnit = clang_parseTranslationUnit(index, fileName,
	    clang_args, num_clang_args, 0, 0, options);

#if(0)
	printf("%s\n", fileName);
	int numDiags = clang_getNumDiagnostics(mTransUnit);
	for (int i = 0; i<numDiags; i++)
	    {
	    CXDiagnostic diag = clang_getDiagnostic(mTransUnit, i);
//	    CXDiagnosticSeverity sev = clang_getDiagnosticSeverity(diag);
//	    if(sev >= CXDiagnostic_Error)
	    CXStringDisposer diagStr = clang_formatDiagnostic(diag,
		clang_defaultDiagnosticDisplayOptions());
		printf("%s\n", diagStr.c_str());
	    }
	fflush(stdout);
#endif
	}
    else
	{
	CXUnsavedFile file;
	file.Filename = mSourceFilename.c_str();
	file.Contents = buffer;
	file.Length = bufLen;
	int stat = clang_reparseTranslationUnit(mTransUnit, 1, &file, options);
	if(stat != 0)
	    {
	    clang_disposeTranslationUnit(mTransUnit);
	    mTransUnit = nullptr;
	    }
	}
    mSourceFile = clang_getFile(mTransUnit, fileName);
    }

void Tokenizer::tokenize(/*int startLine, int endLine, */TokenRange &tokens)
    {
    if(mTransUnit)
	tokens.tokenize(mTransUnit, mSourceFile/*, startLine, endLine*/);
    }


#if(0)
static CXCursor getCursorUsingTokens(CXTranslationUnit tu, CXCursor cursor,
	unsigned int desiredOffset)
    {
    // The following does not return a more definitive cursor.
    // For example, if a compound statement is returned, this does not find
    // any variables in the statement.
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens;
    unsigned numTokens;
    clang_tokenize(tu, range, &tokens, &numTokens);
    unsigned int closestOffset = 0;
    for(unsigned int i=0; i<numTokens; i++)
	{
	CXSourceLocation loc = clang_getTokenLocation(tu, tokens[i]);
	unsigned int offset;
	CXFile file;
	clang_getSpellingLocation(loc, &file, nullptr, nullptr, &offset);
	if(offset < desiredOffset && offset > closestOffset)
	    {
	    closestOffset = offset;
	    cursor = clang_getCursor(tu, loc);
	    }
	}
    clang_disposeTokens(tu, tokens, numTokens);
    return cursor;
    }
#endif

#if(1)
// If this doesn't descend to a detailed cursor, then most likely
// there is a compile error.
static CXCursor getCursorAtOffset(CXTranslationUnit tu, CXFile file,
	unsigned desiredOffset)
    {
    CXSourceLocation loc = clang_getLocationForOffset(tu, file, desiredOffset);
    CXCursor cursor = clang_getCursor(tu, loc);
//    CXSourceRange range = clang_getTokenExtent(tu, CXToken)
//    cursor = getCursorUsingTokens(tu, cursor, desiredOffset);
    return cursor;
    }

#else

struct visitTranslationUnitData
    {
    visitTranslationUnitData(CXFile file, unsigned offset):
	mFile(file), mDesiredOffset(offset), mSize(INT_MAX)
	{}
    CXFile mFile;
    unsigned mDesiredOffset;
    unsigned mSize;
    CXCursor mCursor;
    };

#include <memory.h>
static bool clangFilesEqual(CXFile f1, CXFile f2)
    {
// Not declared?
//    return clang_File_isEqual(f1, f2);
    CXFileUniqueID f1id;
    CXFileUniqueID f2id;
    if(f1)
	clang_getFileUniqueID(f1, &f1id);
    if(f2)
	clang_getFileUniqueID(f2, &f2id);
    return(memcmp(&f1id.data, &f2id.data, sizeof(f1id.data)) == 0);
    }

static CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor parent,
	CXClientData client_data)
    {
    visitTranslationUnitData *data = static_cast<visitTranslationUnitData*>(client_data);
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXSourceLocation startLoc = clang_getRangeStart(range);
    CXSourceLocation endLoc = clang_getRangeEnd(range);
    CXFile file;
    unsigned startOffset;
    unsigned endOffset;
    clang_getSpellingLocation(startLoc, &file, nullptr, nullptr, &startOffset);
    clang_getSpellingLocation(endLoc, &file, nullptr, nullptr, &endOffset);
    // Use the smallest cursor that surrounds the desired location.
    unsigned size = endOffset - startOffset;
    if(clangFilesEqual(file, data->mFile) &&
	    startOffset < data->mDesiredOffset && endOffset > data->mDesiredOffset)
	{
	if(size < data->mSize)
	    {
	    data->mCursor = cursor;
	    data->mSize = size;

	    printf("GOOD:\n   ");
	    }
	}
    CXStringDisposer sp = clang_getCursorSpelling(cursor);
    CXStringDisposer kind = clang_getCursorKindSpelling(cursor.kind);
    std::string fn;
    if(file)
	{
	CXStringDisposer s = clang_getFileName(file);
	fn = s;
	}
    printf("%s %s off %d size %d des offset %d file %s\n", kind.c_str(), sp.c_str(), startOffset,
	    size, data->mDesiredOffset, fn.c_str());
//    clang_visitChildren(cursor, ::visitTranslationUnit, client_data);
//    return CXChildVisit_Continue;
    return CXChildVisit_Recurse;
    }

static CXCursor getCursorAtOffset(CXTranslationUnit tu, CXFile file, unsigned offset)
    {
    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
    visitTranslationUnitData data(file, offset);
    clang_visitChildren(rootCursor, ::visitTranslationUnit, &data);
    printf("--\n");
    fflush(stdout);
    CXCursor cursor = data.mCursor;
    return cursor;
    }
#endif

// Desired functions:
//	Go to definition of variable/function
//	Go to declaration of function/class
bool Tokenizer::find(eFindTokenTypes ft, int origOffset, std::string &fn,
	int &line)
    {
    CXCursor cursor = getCursorAtOffset(mTransUnit, mSourceFile, origOffset);
    // Instantiating type - <class> <type> - CXCursor_TypeRef
    // Method declaration - <class> { <method>(); }; CXCursor_NoDeclFound
    // Method definition - <class>::<method>(){} - CXCursor_CXXMethod
    // Class/method usage - <class>.<method>()
    // Instance usage - method(){ int v = typename[4]; } - CXCursor_DeclStmt
    //		clang_getCursorSemanticParent returns method
    //		clang_getCursorDefinition returns invalid
    if(cursor.kind == CXCursor_InclusionDirective)
	{
	CXFile file = clang_getIncludedFile(cursor);
	if(file)
	    {
	    CXStringDisposer cfn = clang_getFileName(file);
	    fn = cfn;
	    line = 1;
	    }
	else
	    {
	    /// @todo - need to get the full path.
//	    CXStringDisposer cfn = clang_getCursorSpelling(cursor);
//	    fn = cfn;
	    line = 1;
	    }
	}
    else
	{
	switch(ft)
	    {
	    case FT_FindDecl:
		cursor = clang_getCursorReferenced(cursor);
		break;

	    case FT_FindDef:
		// If instantiating a type (CXCursor_TypeRef), this goes to the type delaration.
		cursor = clang_getCursorDefinition(cursor);
    //	    cursor = clang_getCursor(mTransUnit, clang_getCursorLocation(cursor));
    //	    cursor = clang_getCursorDefinition(cursor);
		break;
    //    	cursor = clang_getCursorReferenced(cursor);
    //	cursor = clang_getCanonicalCursor(cursor);
    //	cursor = clang_getCursorSemanticParent(cursor);
    //	cursor = clang_getCursorLexicalParent(cursor);
	    }
	if(!clang_Cursor_isNull(cursor))
	    {
	    CXSourceLocation loc = clang_getCursorLocation(cursor);

	    CXFile file;
	    unsigned int uline;
	    clang_getFileLocation(loc, &file, &uline, nullptr, nullptr);
	    if(file)
		{
		line = uline;
		CXStringDisposer cfn = clang_getFileName(file);
		fn = cfn;
		}
	    }
	}
    return(fn.size() > 0);
    }


HighlightTag::HighlightTag():
	mTag(nullptr)
    {
    }

void HighlightTag::setForegroundColor(GtkTextBuffer *textBuffer,
	char const * const name, char const * const color)
    {
    if(!mTag)
	mTag = gtk_text_buffer_create_tag(textBuffer, name, NULL);
    g_object_set(G_OBJECT(mTag), "foreground", color, "foreground-set", TRUE, NULL);
    }

HighlightTag::~HighlightTag()
    {
    /// @todo - delete tag?
    }

void HighlightTags::initTags(GtkTextBuffer *textBuffer)
    {
    if(!mTags[CXToken_Punctuation].isInitialized())
	{
	mTags[CXToken_Punctuation].setForegroundColor(textBuffer, "punc", "black");
	mTags[CXToken_Keyword].setForegroundColor(textBuffer, "key", "blue");
	mTags[CXToken_Identifier].setForegroundColor(textBuffer, "iden", "black");
	mTags[CXToken_Literal].setForegroundColor(textBuffer, "lit", "red");
	mTags[CXToken_Comment].setForegroundColor(textBuffer, "comm", "dark green");
	}
    }

void Highlighter::highlight(GtkTextView *textView, OovStringRef const filename,
	OovStringRef const buffer, int bufLen, char const * const clang_args[], int num_clang_args)
    {
    GdkRectangle rect;
    gtk_text_view_get_visible_rect(textView, &rect);
    int startBufY;
    int endBufY;
    gtk_text_view_window_to_buffer_coords(textView, GTK_TEXT_WINDOW_WIDGET,
	    0, rect.y, NULL, &startBufY);
    gtk_text_view_window_to_buffer_coords(textView, GTK_TEXT_WINDOW_WIDGET,
	    0, rect.y+rect.height, NULL, &endBufY);

    // Get iterators
    GtkTextIter startIter;
    GtkTextIter endIter;
    gtk_text_view_get_line_at_y(textView, &startIter, startBufY, NULL);
    gtk_text_view_get_line_at_y(textView, &endIter, endBufY, NULL);

//    int startLineY = gtk_text_iter_get_line(&startIter);
//    int endLineY = gtk_text_iter_get_line(&endIter);
//    if(startLineY < 1)
//	startLineY = 1;

    mTokenizer.parse(filename, buffer, bufLen, clang_args, num_clang_args);
    TokenRange tokens;
    mTokenizer.tokenize(/*startLineY, endLineY, */tokens);
    applyTags(gtk_text_view_get_buffer(textView), tokens);
    }

void Highlighter::applyTags(GtkTextBuffer *textBuffer, const TokenRange &tokens)
    {
    mHighlightTags.initTags(textBuffer);

    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(textBuffer, &start, &end);
    gtk_text_buffer_remove_all_tags(textBuffer, &start, &end);
    for(size_t i=0; i<tokens.size(); i++)
	{
	gtk_text_buffer_get_iter_at_offset(textBuffer, &start, tokens[i].mStartOffset);
	gtk_text_buffer_get_iter_at_offset(textBuffer, &end, tokens[i].mEndOffset);
	gtk_text_buffer_apply_tag(textBuffer,
		mHighlightTags.getTag(tokens[i].mTokenKind), &start, &end);
	}
    }
