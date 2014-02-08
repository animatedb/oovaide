/*
 * Builder.cpp
 *
 *  Created on: Jan 20, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Builder.h"

static Builder *gBuilder;

void Builder::init()
    {
    if(!gBuilder)
	mGtkBuilder = gtk_builder_new();
    gBuilder = this;
    }

Builder *Builder::getBuilder()
    {
    return gBuilder;
    }

bool Builder::addFromFile(char const * const fn)
    {
    GError *err = nullptr;
    init();
    gtk_builder_add_from_file(mGtkBuilder, fn, &err);
    return(!err);
    }
