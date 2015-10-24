#!/bin/sh

# This is the destination compared to the bin directory.
dst=../../../backup/oovaide
# This is the directory named source
source=..
# This is the source directory in the destination
dstFromSource=../../backup/oovaide
#clang=../../../clang+llvm-3.4-x86_64

rm -r $dst
mkdir $dst

find ../test/trunk-oovaide-*/analysis* -delete
find ../test/trunk-oovaide-*/oovaide-tmp* -delete

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

rm -r $dstFromSource/.metadata
rm -r $dstFromSource/examples/simple-oovaide
rm -r $dstFromSource/examples/staticlib-oovaide
rm -r $dstFromSource/examples/sharedlibgtk-oovaide

