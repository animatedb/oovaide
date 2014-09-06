/*
 * PackagesDialogs.cpp
 *
 *  Created on: Jan 20, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "PackagesDialogs.h"
#include "OovString.h"
#include "DirList.h"
#include "ctype.h"
#include <algorithm>

static ProjectPackagesDialog *sProjectPackagesDialog;
static AddPackageDialog *sAddPackageDialog;


static void setEntry(char const * const widgetName, char const * const str)
    {
    GtkEntry *entry = GTK_ENTRY(Builder::getBuilder()->getWidget(widgetName));
    Gui::setText(entry, str);
    }

static std::string getEntry(char const * const widgetName)
    {
    GtkEntry *entry = GTK_ENTRY(Builder::getBuilder()->getWidget(widgetName));
    return Gui::getText(entry);
    }


AddPackageDialog::AddPackageDialog():
    Dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("AddPackageDialog")),
	    GTK_WINDOW(Builder::getBuilder()->getWidget("ProjectPackagesDialog")))
    {
    sAddPackageDialog = this;
    mAllPackagesList.init(*Builder::getBuilder(), "AllPackagesTreeview", "Available Packages");
    mAllPackagesList.clear();
    for(auto const &pkgName : mAvailPackages.getAvailablePackages())
    	mAllPackagesList.appendText(pkgName.c_str());
    }

AddPackageDialog::~AddPackageDialog()
    {
    sAddPackageDialog = nullptr;
    }

Package AddPackageDialog::getPackage() const
    {
    return mAvailPackages.getPackage(getEntry("AddPackageEntry").c_str());
    }

void AddPackageDialog::selectPackage()
    {
    setEntry("AddPackageEntry", mAllPackagesList.getSelected().c_str());
    }

/////////////////

ProjectPackagesDialog::ProjectPackagesDialog(std::string &baseBuildArgs):
    Dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("ProjectPackagesDialog")),
	    GTK_WINDOW(Builder::getBuilder()->getWidget("OptionsDialog"))),
    mProjectPackages(true), mBaseBuildArgs(baseBuildArgs), mAllowSelection(true)
    {
    sProjectPackagesDialog = this;
    mProjectPackagesList.init(*Builder::getBuilder(), "ProjectPackagesTreeview", "Packages");
#ifdef __linux__
    Gui::setVisible(GTK_LABEL(Builder::getBuilder()->getWidget(
	    "MissingDirectoryLabel")), false);
    Gui::setVisible(GTK_BUTTON(Builder::getBuilder()->getWidget(
	    "ScanDirectoriesButton")), false);
#endif
    }

ProjectPackagesDialog::~ProjectPackagesDialog()
    {
    sProjectPackagesDialog = nullptr;
    }


void ProjectPackagesDialog::beforeRun()
    {
    // Don't read packages from args. They must be in project package file.
    size_t pos=0;

    OovString args(mBaseBuildArgs.c_str());
    while(pos != std::string::npos)
	{
	pos = args.find("-EP", pos);
	if(pos != std::string::npos)
	    {
	    size_t endPos = args.findSpace(pos+3);
//	    std::string pkg;
	    if(endPos != std::string::npos)
		{
//		pkg = args.substr(pos+3, endPos-(pos+3));
		if(args[endPos] == '\n')
		    endPos++;
		args.erase(pos, endPos-pos);
		}
	    else
		{
//		pkg = args.substr(pos+3);
		args.erase(pos);
		}
	    // Could use the package names for something.
//	    pkgNames.push_back(pkg.c_str());
	    }
	}
    updatePackageList();
    mBaseBuildArgs = args;
    }

void ProjectPackagesDialog::afterRun(bool ok)
    {
    if(ok)
	{
	savePackage(mProjectPackagesList.getSelected().c_str());
	mProjectPackages.savePackages();
	int len = mBaseBuildArgs.length();
	if(len > 0)
	    {
	    if(mBaseBuildArgs[len-1] != '\n')
		mBaseBuildArgs.append("\n");
	    }
	for(auto const &str : mProjectPackagesList.getText())
	    {
	    mBaseBuildArgs += "-EP";
	    mBaseBuildArgs += str;
	    mBaseBuildArgs += "\n";
	    }
	}
    }

void ProjectPackagesDialog::savePackage(const std::string &pkgName)
    {
    if(pkgName.length() > 0)
	{
	Package pkg(pkgName.c_str(), getEntry("PackageRootDirEntry").c_str());
	pkg.setCompileInfo(getEntry("PackageIncDirEntry"),
		getEntry("PackageCompileArgsEntry"));
	pkg.setLinkInfo(getEntry("PackageLibDirEntry"), getEntry("PackageLibNamesEntry"),
		getEntry("PackageLinkArgsEntry"));
	pkg.setExternalReferenceDir(getEntry("PackageExternRefEntry"));
	mProjectPackages.insertPackage(pkg);
	}
    }

void ProjectPackagesDialog::selectPackage()
    {
    if(mAllowSelection)
	{
	std::string pkgName = mProjectPackagesList.getSelected();
	savePackage(mLastSelectedPackage);
	Package pkg = mProjectPackages.getPackage(pkgName.c_str());

	setEntry("PackageRootDirEntry", pkg.getRootDir().c_str());
	setEntry("PackageIncDirEntry", pkg.getIncludeDirsAsString().c_str());
	setEntry("PackageCompileArgsEntry", pkg.getCompileArgsAsStr().c_str());
	setEntry("PackageLibNamesEntry", pkg.getLibraryNamesAsString().c_str());
	setEntry("PackageLibDirEntry", pkg.getLibraryDirsAsString().c_str());
	setEntry("PackageLinkArgsEntry", pkg.getLinkArgsAsStr().c_str());
	setEntry("PackageExternRefEntry", pkg.getExtRefDirsAsString().c_str());
	mLastSelectedPackage = pkgName.c_str();
	}
    }

void ProjectPackagesDialog::clearPackageDisplay()
    {
    setEntry("PackageRootDirEntry", "");
    setEntry("PackageIncDirEntry", "");
    setEntry("PackageCompileArgsEntry", "");
    setEntry("PackageLibNamesEntry", "");
    setEntry("PackageLibDirEntry", "");
    setEntry("PackageLinkArgsEntry", "");
    setEntry("PackageExternRefEntry", "");
    mLastSelectedPackage.clear();
    }

void ProjectPackagesDialog::removePackage()
    {
    std::string pkgName = mProjectPackagesList.getSelected();
    mProjectPackages.removePackage(pkgName.c_str());
    updatePackageList();
    }

void ProjectPackagesDialog::updatePackageList()
    {
    mAllowSelection = false;
    clearPackageDisplay();
    mProjectPackagesList.clear();
    for(auto const &pkg : mProjectPackages.getPackages())
	mProjectPackagesList.appendText(pkg.getPkgName().c_str());
    mAllowSelection = true;
    }

void ProjectPackagesDialog::displayAddPackageDialog()
    {
    AddPackageDialog dlg;
    if(dlg.run(true))
	{
	std::vector<std::string> packages = mProjectPackagesList.getText();

	Package pkg = dlg.getPackage();
	if(std::find(packages.begin(), packages.end(), pkg.getPkgName()) ==
		packages.end())
	    {
	    mProjectPackages.insertPackage(pkg);
	    }
	else
	    {
	    Gui::messageBox("Package already exists");
	    }
	updatePackageList();
	}
    }


#ifndef __linux__

void ProjectPackagesDialog::winSetEnableScanning()
    {
    bool missing = !fileExists(getEntry("PackageRootDirEntry").c_str());
    Gui::setEnabled(GTK_LABEL(Builder::getBuilder()->getWidget(
	    "MissingDirectoryLabel")), missing);
    Gui::setEnabled(GTK_BUTTON(Builder::getBuilder()->getWidget(
	    "ScanDirectoriesButton")), missing);
    }

static int winMatchPackage(char const * const pkgName, char const * const dirName)
    {
    int matchQuality = 0;
    std::vector<std::string> pkgParts;
    char const * p = pkgName;
    enum CharTypes { CT_Start, CT_Other, CT_Num, CT_Alpha };
    CharTypes lastCt = CT_Start;
    std::string part;
    while(*p)
	{
	CharTypes ct;
	if(isalpha(*p))
	    ct = CT_Alpha;
	else if(isdigit(*p))
	    ct = CT_Num;
	else
	    ct = CT_Other;
	if(lastCt == CT_Start)
	    lastCt = ct;
	if(ct == lastCt)
	    {
	    if(ct != CT_Other)
		part += *p;
	    }
	else
	    {
	    pkgParts.push_back(part);
	    lastCt = ct;
	    part += *p;
	    }
	p++;
	}
    pkgParts.push_back(part);
    // Rate alpha more than numeric, and longer strings more than many shorter strings
    std::string dir = dirName;
    std::transform(dir.begin(), dir.end(), dir.begin(), ::tolower);
    for(auto &part : pkgParts)
	{
	std::transform(part.begin(), part.end(), part.begin(), ::tolower);
	if(std::string(dir).find(part) != std::string::npos)
	    {
	    int len = part.length();
	    int mult = 1;
	    if(isalpha(part[0]))
		mult = 2;
	    matchQuality += len * len * mult;
	    }
	}
    return matchQuality;
    }

void ProjectPackagesDialog::winScanDirectories()
    {
    Package pkg = mProjectPackages.getPackage(
	    mProjectPackagesList.getSelected().c_str());
    if(pkg.getPkgName().length() > 0)
	{
	char const * const srchDirs[] =
		{ "/", "/Program Files" };
	std::vector<std::string> dirs;
	for(auto const &dir : srchDirs)
	    {
	    getDirList(dir, DL_Dirs, dirs);
	    }
	int bestMatchQuality = 0;
	std::string bestDir;
	for(auto const dir : dirs)
	    {
	    int qual = winMatchPackage(pkg.getPkgName().c_str(), dir.c_str());
	    if(qual > bestMatchQuality)
		{
		bestDir = dir;
		bestMatchQuality = qual;
		}
	    }
	pkg.setRootDir(bestDir.c_str());
	setEntry("PackageRootDirEntry", pkg.getRootDir().c_str());
	}
    else
	Gui::messageBox("Select a package to scan", GTK_MESSAGE_INFO);
    }

#endif


///////////////////

extern "C" G_MODULE_EXPORT void on_PackageAddButton_clicked(GtkWidget *button, gpointer data)
    {
    if(sProjectPackagesDialog)
	sProjectPackagesDialog->displayAddPackageDialog();
    }

extern "C" G_MODULE_EXPORT void on_PackageRemoveButton_clicked(GtkWidget *button, gpointer data)
    {
    if(sProjectPackagesDialog)
	sProjectPackagesDialog->removePackage();
    }

extern "C" G_MODULE_EXPORT void on_ProjectPackagesTreeview_cursor_changed(GtkWidget *button, gpointer data)
    {
    if(sProjectPackagesDialog)
	sProjectPackagesDialog->selectPackage();
    }

//////////////

extern "C" G_MODULE_EXPORT void on_AllPackagesTreeview_cursor_changed(GtkWidget *button, gpointer data)
    {
    if(sAddPackageDialog)
	sAddPackageDialog->selectPackage();
    }

extern "C" G_MODULE_EXPORT void on_ScanDirectoriesButton_clicked(GtkWidget *button, gpointer data)
    {
#ifndef __linux__
    if(sProjectPackagesDialog)
	sProjectPackagesDialog->winScanDirectories();
#endif
    }

extern "C" G_MODULE_EXPORT void on_PackageRootDirEntry_changed(GtkWidget *button, gpointer data)
    {
#ifndef __linux__
    if(sProjectPackagesDialog)
	sProjectPackagesDialog->winSetEnableScanning();
#endif
    }
