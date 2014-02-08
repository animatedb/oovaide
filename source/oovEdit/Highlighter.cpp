/*
 * Highlighter.cpp
 *
 *  Created on: Sep 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Highlighter.h"


void TokenRange::tokenize(CXTranslationUnit transUnit, CXFile srcFile,
	int startLine, int endLine)
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

void Tokenizer::parse(char const * const fileName, char const * const buffer, int bufLen,
	char const * const clang_args[], int num_clang_args)
    {
    if(!mSourceFile)
	{
	unsigned options = 0;
	mSourceFilename = fileName;
	CXIndex index = clang_createIndex(1, 1);
	mTransUnit = clang_parseTranslationUnit(index, fileName,
	    clang_args, num_clang_args, 0, 0, options);
	mSourceFile = clang_getFile(mTransUnit, fileName);
	}
    else
	{
	unsigned options = clang_defaultEditingTranslationUnitOptions();
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
    }

void Tokenizer::tokenize(int startLine, int endLine, TokenRange &tokens)
    {
    if(mTransUnit)
	tokens.tokenize(mTransUnit, mSourceFile, startLine, endLine);
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

void Highlighter::highlight(GtkTextView *textView, char const * const filename,
	char const * const buffer, int bufLen, char const * const clang_args[], int num_clang_args)
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

    int startLineY = gtk_text_iter_get_line(&startIter);
    int endLineY = gtk_text_iter_get_line(&endIter);
    if(startLineY < 1)
	startLineY = 1;

    mTokenizer.parse(filename, buffer, bufLen, clang_args, num_clang_args);
    TokenRange tokens;
    mTokenizer.tokenize(startLineY, endLineY, tokens);
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
