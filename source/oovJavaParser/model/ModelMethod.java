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

        String mMethodName;
        ModelTypeRefs mParameters;
        ModelStatements mStatements;
    };
