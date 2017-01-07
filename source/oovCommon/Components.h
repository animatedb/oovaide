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
#include "Project.h"

FilePaths getCppHeaderExtensions();
FilePaths getCppSourceExtensions();
FilePaths getLibExtensions();

bool isJavaSource(OovStringRef const file);
bool isCppHeader(OovStringRef const file);
bool isCppSource(OovStringRef const file);
bool isLibrary(OovStringRef const file);

struct ComponentDefinition
    {
    OovString mCompName;
    eCompTypes mCompType;
    OovString const &getCompName() const
        { return mCompName; }
    eCompTypes const &getCompType() const
        { return mCompType; }
    };
typedef std::vector<ComponentDefinition> ComponentDefinitions;

/// The component types information is read and written by both oovaide and
/// oovBuilder.
///
/// This class uses information from the the project. The project has all
/// user entered information such as the type of each component.
class ComponentTypesFile
    {
    public:
        ComponentTypesFile(ProjectReader &project):
            mProject(project), mBuildEnv(nullptr)
            {}

        /// If there is a build env, then this class returns data for the matching env.
        /// If there is no build env, then functions return component type info for
        /// the superset of all matching possibilities.
        void setBuildEnvironment(BuildVariableEnvironment const *buildEnv)
            { mBuildEnv = buildEnv; }

        OovStatusReturn writeComponentTypes() const
            { return mProject.writeFile(); }

        /// This returns a defined component even when some build
        /// configurations do not have the component defined.
        ///
        /// Get the component names with a defined component type of
        /// a project.  The component names do not have an absolute path,
        /// and is <Root> for the root.
        OovStringVec getDefinedComponentNames() const;

        /// If there is no build environment, this returns a defined component
        /// even when some build configurations do not have the component defined.
        /// If there is a build environment, this returns component names for
        /// any build configuration.
        ///
        /// Get the components with a defined component type of a project.
        /// The component names do not have an absolute path,
        /// and is <Root> for the root.
        /// @param compName If null, returns all defined components.
        ComponentDefinitions getDefinedComponents(char const *compName=nullptr) const;

        /// Returns a directory relative to relDir.
        /// This returns relDir for the root compName.
        static OovString getComponentDir(OovStringRef relDir,
            OovStringRef compName);

        /// Returns a directory relative to relDir.
        /// Uses getComponentDir for the directory, and uses the compName to
        /// build a base file name. If compName is root, then the component
        /// name is built using getRootComponentFileName().
        static OovString getComponentBaseFileName(OovStringRef relDir,
            OovStringRef compName);

        /// Returns a directory relative to relDir.
        /// Uses getComponentBaseFileName and adds the extension.
        static OovString getComponentFileName(OovStringRef relDir,
            OovStringRef compName, OovStringRef ext);

        /// Returns a directory relative to relDir.
        /// Uses getComponentBaseFileName and adds the prefix and extension.
        static OovString getComponentFileName(OovStringRef relDir,
            OovStringRef compName, OovStringRef basePrefix, OovStringRef ext);

        /// Get the component names that match the type.
        /// @param cft The component type.
        OovStringVec getDefinedComponentNamesByType(eCompTypes cft) const;

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

        /// Build a variable filter name for the component type that matches
        /// the passed in component.
        /// WARNING: This does not work for build environment names.
        static OovString buildCompTypeVarFilterName(OovStringRef const compName);

        /// If there is no owner, this returns the root component.
        /// This can return the owner as the passed in name if it is a defined
        /// component.
        OovString getComponentNameOwner(OovStringRef compName) const;

    private:
        ProjectReader &mProject;
        /// The build environment references the project, but also contains
        /// build variable filter values.
        BuildVariableEnvironment const *mBuildEnv;

        void setComponentType(OovStringRef const compName, eCompTypes ct);

        // Setting a component below some parent must make sure the parents are unknown
        void coerceParentComponents(OovStringRef const compName);

        // Setting a component above some child must make sure children are unknown
        void coerceChildComponents(OovStringRef const compName);

        /// Returns the matching component type variables if there is an
        /// environment, else returns all component type variables.
        OovStringVec getMatchingCompTypeVariables() const;

        static OovStringRef const getComponentTypeAsFileValue(eCompTypes ct);

        static enum eCompTypes getComponentTypeFromTypeName(
            OovStringRef const compTypeName);
    };


// This contains a list of directories, and a lists of source files
// by component and file type.
class ScannedComponentInfo
    {
    public:
        OovStatusReturn readScannedInfo();

        /// Write the file to disk.
        OovStatusReturn writeScannedInfo();

        /// Set the component names for a project.
        /// @param compNames A list of component names using the default
        ///     CompoundValue separator.
        void setComponentNames(OovStringRef const compNames)
            { mCompSourceListFile.setNameValue("Components", compNames); }

        /// Get the component names for a project.  The component names
        /// do not have an absolute path, and is <Root> for the root.
        /// @param definedComponentsOnly If true, this does not return any
        ///     CT_Unknown components.
        OovStringVec getComponentNames() const;

        /// Set the list of files for a component.
        /// @param cft The component file type.
        /// @param compName The component name.
        /// @param srcs The list of sources with CompoundValue default delimeters.
        enum CompFileTypes { CFT_CppSource, CFT_CppInclude, CFT_JavaSource };
        void setComponentFiles(CompFileTypes cft, OovStringRef const compName,
            OovStringSet const &srcs);

        /// Gets the files in a single component directory. (No nesting).
        OovStringVec getComponentDirFiles(OovStringRef compName,
            OovStringRef tagStr) const;

        /// Get the list of full paths to the source files for a component.
        /// This can match files in subdirectories of the component.
        /// @param cft The component file type.
        /// @param compName The component name.
        /// @param getNested Set true to get all files for a component. Set
        ///        false to get the files in the specified directory/component.
        OovStringVec getComponentFiles(ComponentTypesFile compInfo,
            CompFileTypes cft, OovStringRef const compName, bool getNested=true) const;

    protected:
        static OovStringRef getCompFileTypeTagName(CompFileTypes cft);

    private:
        NameValueFile mCompSourceListFile;

        static OovString getCompTagName(OovStringRef const compName, OovStringRef const tag);
        OovStringVec getComponentFiles(ComponentTypesFile compInfo,
            OovStringRef const compName, OovStringRef const tagStr,
            bool getNested=true) const;
    };


#endif /* COMPONENTS_H_ */
