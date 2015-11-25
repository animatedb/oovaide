// ModelData.java
// Created on: Oct 16, 2015
// \copyright 2015 DCBlaha.  Distributed under the GPL.

package model;

import java.util.ArrayList;
import java.util.Iterator;


public class ModelData implements Iterable<ModelType>
    {
        public ModelData()
            {
            super();
            mTypes = new ArrayList<ModelType>();
            mImports = new ArrayList<String>();
            }

        public void setModuleName(String moduleName)
            { mModuleName = moduleName; }
        public void setModuleLines(int numModuleLines)
            { mModuleLines = numModuleLines; }

        public void addType(ModelType type)
            { mTypes.add(type); }

        public void addImport(String importStr)
            { mImports.add(importStr); }

        public void setPackage(String packageName)
            { mPackageName = packageName; }

        public String getModuleName()
            { return mModuleName; }

        public ModelType findType(String name)
            {
            for(int i=0; i<mTypes.size(); i++)
                {
                ModelType type = mTypes.get(i);
                if(type.getTypeName().compareTo(name) == 0)
                    {
                    return type;
                    }
                }
            return null;
            }

        public int getModuleLines()
            { return mModuleLines; }

        public String getPackage()
            { return mPackageName; }

        public Iterator<ModelType> getTypes()
            { return mTypes.iterator(); }

        public Iterator<ModelType> iterator()
            { return getTypes(); }

        public ArrayList<String> getImports()
            { return mImports; }

        String mModuleName;
        int mModuleLines;              // Output as moduleLines="23"
	ArrayList<ModelType> mTypes;
	ArrayList<String> mImports;
        String mPackageName;
    };
