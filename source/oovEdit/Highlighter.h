/*
 * Highlighter.h
 *
 *  Created on: Sep 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef HIGHLIGHTER_H_
#define HIGHLIGHTER_H_

#include "clang-c/Index.h"
#include "Gui.h"
#include <vector>

struct Token
    {
    CXTokenKind mTokenKind;
    unsigned int mStartOffset;
    unsigned int mEndOffset;
    };

class TokenRange:public std::vector<Token>
    {
    public:
	void tokenize(CXTranslationUnit transUnit, CXFile srcFile,
		int startLine, int endLine);
    };

class Tokenizer
    {
    public:
	Tokenizer():
	    mSourceFile(nullptr)
	    {}
	void parse(char const * const fileName, char const * const buffer, int bufLen,
		char const * const clang_args[], int num_clang_args);
	// line numbers are 1 based.
	void tokenize(int startLine, int endLine, TokenRange &highlight);

    private:
	CXTranslationUnit mTransUnit;
	CXFile mSourceFile;
	std::string mSourceFilename;
    };

class HighlightTag
    {
    public:
	HighlightTag();
	~HighlightTag();
	void setForegroundColor(GtkTextBuffer *textBuffer, char const * const name,
		char const * const color);
	bool isInitialized() const
	    { return mTag != nullptr; }
	GtkTextTag *getTextTag() const
	    { return mTag; }
    private:
	GtkTextTag *mTag;
    };

class HighlightTags
    {
    public:
	void initTags(GtkTextBuffer *textBuffer);
	GtkTextTag *getTag(int tagIndex)
	    { return mTags[tagIndex].getTextTag(); }

    private:
	HighlightTag mTags[5];
    };

class Highlighter
    {
    public:
	void highlight(GtkTextView *textView, char const * const filename,
		char const * const buffer, int bufLen,
		char const * const clang_args[], int num_clang_args);
	void applyTags(GtkTextBuffer *textBuffer, const TokenRange &tokens);

    private:
	Tokenizer mTokenizer;
	HighlightTags mHighlightTags;
    };

#endif /* HIGHLIGHTER_H_ */
