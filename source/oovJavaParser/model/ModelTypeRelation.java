// ModelTypeRelation.java
// Created on: Oct 19, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

public class ModelTypeRelation
    {
    public enum RelationType { RT_Extends, RT_Implements };

    public void setRelationType(RelationType relType)
        { mRelationType = relType; }

    public RelationType getRelationType()
        { return mRelationType; }

    public void setType(ModelType type)
        { mType = type; }

    public ModelType getType()
        { return mType; }

    RelationType mRelationType;
    ModelType mType;
    }
