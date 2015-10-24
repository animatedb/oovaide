// ModelTypeRef.java
// Created on: Oct 19, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

public class ModelTypeRef
    {
    public void setName(String name)
        { mVarName = name; }

    public String getName()
        { return mVarName; }

    public void setType(ModelType type)
        { mType = type; }

    public ModelType getType()
        { return mType; }

    String mVarName;
    ModelType mType;
    }
