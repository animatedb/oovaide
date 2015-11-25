// ModelMethod.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

public class ModelMethod
    {
        public ModelMethod()
            {
            mParameters = new ModelTypeRefs();
            mStatements = new ModelStatements();
            }

        public void setMethodName(String methodName)
            { mMethodName = methodName; }


        public String getMethodName()
            { return mMethodName; }

        /// Use this to get or add parameters.
        public ModelTypeRefs getParameters()
            { return mParameters; }

       /// Use this to get or add statements.
       public ModelStatements getStatements()
           { return mStatements; }

        public void addOpenCondStatement(String condText)
            {
            ModelStatement statement = new ModelStatement(
                ModelStatement.StatementType.ST_OpenNest);
            statement.setName(condText);
            mStatements.addStatement(statement);
            }

        public void addCloseStatement()
            {
            ModelStatement statement = new ModelStatement(
            ModelStatement.StatementType.ST_CloseNest);
            mStatements.addStatement(statement);
            }


        String mMethodName;
        ModelTypeRefs mParameters;
        ModelStatements mStatements;
    };
