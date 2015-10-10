/*
 * Components.h
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTS_H_
#define COMPONENTS_H_

#include "NameValueFile.h"
#include "FilePath.h"

FilePaths getCppHeaderExtensions();
FilePaths getCppSourceExtensions();
FilePaths getLibExtensions();

bool isCppHeader(OovStringRef const file);
bool isCppSource(OovStringRef const file);
bool isLibrary(OovStringRef const file);


/// The component types information is read and written by both oovcde and
/// oovBuilder.
///
/// This class is made up of two files.
///    - One file defines the component types defining directories. The
///      component types file is a NameValue file that contains a list
///      of component / directories, where each directory can be defined as
///      a component, which has a component type.
///    - The other file is a NameValue file that defines which source and
///      header files are in each directory.
class ComponentTypesFile
    {
    public:
        enum eCompTypes
            {
            CT_Unknown,
            CT_StaticLib,       // .a or .lib
            CT_SharedLib,       // .so or .dll
            CT_Program          // no extension or .exe
            };

        /// This reads both component files.
        OovStatusReturn read();
        /// This reads the component type information only.
        OovStatusReturn readTypesOnly(OovStringRef const fn);
        /// Set the component names for a project.
        /// @param compNames A list of component names using the default
        ///     CompoundValue separator.
        void setComponentNames(OovStringRef const compNames)
            { mCompTypesFile.setNameValue("Components", compNames); }
        /// Get the component names for a project.
        /// @param definedComponentsOnly If true, this does not return any
        ///     CT_Unknown components.
        OovStringVec getComponentNames(bool definedComponentsOnly = false) const;

        /// Gets the child name of the component. The child name is the
        /// last name of the directory.  /parent-part1/parent-part2/child
        /// @param compName The path name of the component
        static std::string getComponentChildName(std::string const &compName);
        /// Gets the parent name of the component. The parent is the part
        /// of the path that is not the child.
        /// @param compName The path name of the component
        static std::string getComponentParentName(std::string const &compName);

        /// Returns true if any of the components in a project are not unknown.
        bool anyComponentsDefined() const;
        /// Returns the component type of a directory / component.
        /// @param compName The name of the component.
        enum eCompTypes getComponentType(OovStringRef const compName) const;
        /// Set the component type of a directory / component.
        /// @param compName The component name.
        /// @param typeName The component type to set.
        void setComponentType(OovStringRef const compName, OovStringRef const typeName);

        /// Set the list of C++ files for a component.
        /// @param cft The component file type.
        /// @param compName The component name.
        /// @param srcs The list of sources with CompoundValue default delimeters.
        enum CompFileTypes { CFT_CppSource, CFT_CppInclude };
        void setComponentFiles(CompFileTypes cft, OovStringRef const compName,
                OovStringSet const &srcs);

        /// Get the list of C++ files for a component. This matches files
        /// in subdirectories of the component.
        /// @param cft The component file type.
        /// @param compName The component name.
        /// @param getNested Set true to get all files for a component. Set
        ///        false to get the files in the specified directory/component.
        OovStringVec getComponentFiles(CompFileTypes cft,
            OovStringRef const compName, bool getNested=true) const;

        /// Gets the build arguments that are specific for a component.
        /// @param compName The component name.
        OovString getComponentBuildArgs(OovStringRef const compName) const;
        /// Sets the build arguments that are specific for a component.
        /// @param compName The component name.
        void setComponentBuildArgs(OovStringRef const compName, OovStringRef const args);

        /// Get the long type of component type name.  This is typically
        /// shown the the user.
        /// @param ct The component type
        static OovStringRef const getLongComponentTypeName(eCompTypes ct);
        /// Get the short type of component type name.  This is typically
        /// saved in a file.
        /// @param ct The component type
        static OovStringRef const getShortComponentTypeName(eCompTypes ct)
            { return getComponentTypeAsFileValue(ct); }

        /// Get the absolute path of a component.  Note that a directory may
        /// contain a subdirectory of type unknown, and both directories are
        /// listed as a component with a component type.  So each
        /// directory / component only has a single absolute path.
        /// @param compName The component name.
        OovString getComponentAbsolutePath(OovStringRef const compName) const;

        /// Write the files to disk.
        OovStatusReturn writeFile();

        /// Only write the component types file to disk.
        OovStatusReturn writeTypesOnly(OovStringRef const fn);

    private:
        NameValueFile mCompTypesFile;
        NameValueFile mCompSourceListFile;

        OovStringVec getComponentFiles(OovStringRef const compName,
                OovStringRef const tagStr, bool getNested=true) const;
        void setComponentType(OovStringRef const compName, eCompTypes ct);
        // Setting a component below some parent must make sure the parents are unknown
        void coerceParentComponents(OovStringRef const compName);
        // Setting a component above some child must make sure children are unknown
        void coerceChildComponents(OovStringRef const compName);
        static OovString getCompTagName(OovStringRef const compName, OovStringRef const tag);
        static OovStringRef const getComponentTypeAsFileValue(eCompTypes ct);
        static enum eCompTypes getComponentTypeFromTypeName(
                OovStringRef const compTypeName);
        static OovStringRef getCompFileTypeTagName(CompFileTypes cft);
    };

/// This class stores the directories that contain include files within a project.
/// These paths are used as default include paths to analyze a file.
/// This file is stored in the analysis directory.
/// This class does not open the file. The read must be done before calling any of
/// the other members.
class ComponentsFile:public NameValueFile
    {
    public:
        /// Reads the specified components file.
        /// @param fn The path of the file to open.
        void read(OovStringRef const fn);
// DEAD CODE
//        /// Get the include paths for the project.
//        OovStringVec getProjectIncludeDirs() const
//            {
//            return CompoundValueRef::parseString(getProjectIncludeDirsStr());
//            }
//        /// Gets the absolute include paths for the project.
//        OovStringVec getAbsoluteIncludeDirs() const;

        /// This splits the root search dir into a root dir and exclude directories.
        /// This can accept a rootSrch dir such as "/rootdir/.!/excludedir1/!/excludedir2/"
        /// @param rootSrch The entire search dir including exclude directories.
        /// @param rootDir The returned rootDir part of the search dir.
        /// @param excludes The returned list of exclude directories.
        static void parseProjRefs(OovStringRef const rootSrch, OovString &rootDir,
                OovStringVec &excludes);
        /// Checks to see if the filePath is a substring of any of the exclude
        /// paths.
        /// @param filePath The filePath to search for within the excludes.
        /// @param excludes The list of exclude directories.
        static bool excludesMatch(OovStringRef const filePath,
                OovStringVec const &excludes);

    private:
        // DEAD CODE
//        OovString getProjectIncludeDirsStr() const;
    };

#endif /* COMPONENTS_H_ */
