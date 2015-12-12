#!/bin/sh

# This is the destination compared to the bin directory.
dstBack=../../../backup
# This is a single version for release
dst=$dstBack/oovaide
# This gets to the directory named source from bin
source=..
# This is the source directory in the destination
dstFromSource=../../backup/oovaide
#clang=../../../clang+llvm-3.4-x86_64

if [ -d $dst ] ; then
  rm -r $dst
  mkdir $dst
else
  if ! [ -d $dstBack ] ; then
    mkdir $dstBack
  fi
  mkdir $dst
  mkdir $dst/bin
  mkdir $dst/web
fi

# Delete files from the source before the copy
find ../../test/oovaide-*/analysis* -delete
find ../../test/oovaide-*/oovaide-tmp* -delete

# copy the logos and images.
rsync -av ../bin/ $dst/bin
rsync -av --exclude='.svn' $source/../web/ $dst/web/
rsync -av --exclude='.svn' $source/bin-linux/ $dst/bin/
# The libclang.so is in the bin-linux dir and will be copied elsewhere in this script
#rsync -av $clang/lib/clang $dst/lib/
cd $source
find . -name '*.cpp' | cpio -pdm $dstFromSource
find . -name '*.h' | cpio -pdm $dstFromSource
find . -name '*.html' | cpio -pdm $dstFromSource
find . -name '*.txt' | cpio -pdm $dstFromSource
find . -name '*.py' | cpio -pdm $dstFromSource
find . -name '*.in' | cpio -pdm $dstFromSource
find . -name '*.cproject' | cpio -pdm $dstFromSource
find . -name '*.project' | cpio -pdm $dstFromSource

if [ -d $dstFromSource/.metadata ] ; then
  rm -r $dstFromSource/.metadata
fi
if [ -d $dstFromSource/examples/simple-oovaide ] ; then
  rm -r $dstFromSource/examples/simple-oovaide
fi
if [ -d $dstFromSource/examples/staticlib-oovaide ] ; then
  rm -r $dstFromSource/examples/staticlib-oovaide
fi
if [ -d $dstFromSource/examples/sharedlibgtk-oovaide ] ; then
  rm -r $dstFromSource/examples/sharedlibgtk-oovaide
fi


