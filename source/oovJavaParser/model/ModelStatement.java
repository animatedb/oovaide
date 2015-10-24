// ModelStatement.java
// Created on: Oct 21, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

public class ModelStatement
    {
    public enum StatementType { ST_OpenNest, ST_CloseNest, ST_Call, ST_VarRef };

    public ModelStatement(StatementType type)
        {
        mName = "";
        mStatementType = type;
        }

    // Use for everything except ST_CloseNest
    public void setName(String name)
        { mName = name; }

    // Used for ST_Call or ST_VarRef
    public void setClassType(ModelType type)
        { mClassType = type; }

    // Used for ST_Call or ST_VarRef
    public void setVarType(ModelType type)
        { mVarType = type; }


    public StatementType getStatementType()
        { return mStatementType; }

    public String getName()
        { return mName; }

    public ModelType getClassType()
        { return mClassType; }

    public ModelType getVarType()
        { return mVarType; }

    StatementType mStatementType;
    String mName;          // Set for everything except ST_CloseNest
    ModelType mClassType;  // Only set for ST_Call or ST_VarRef
    ModelType mVarType;    // Only set for ST_VarRef
    }
