#!/bin/sh

trnk=..
dst=../../../backup/oovcde
clang=../../../clang+llvm-3.4-x86_64
dstFromTrunk=../../backup/oovcde

rm -r $dst
mkdir $dst

rsync -av --exclude='.svn' $trnk/../web/ $dst/web/
rsync -av --exclude='.svn' $trnk/bin-linux/ $dst/bin/
rsync -av $clang/lib/clang $dst/lib/
cd $trnk
find . -name '*.cpp' | cpio -pdm $dstFromTrunk
find . -name '*.h' | cpio -pdm $dstFromTrunk
find . -name '*.html' | cpio -pdm $dstFromTrunk
find . -name '*.txt' | cpio -pdm $dstFromTrunk
find . -name '*.py' | cpio -pdm $dstFromTrunk
find . -name '*.in' | cpio -pdm $dstFromTrunk
find . -name '*.cproject' | cpio -pdm $dstFromTrunk
find . -name '*.project' | cpio -pdm $dstFromTrunk

rm -r $dstFromTrunk/.metadata
rm -r $dstFromTrunk/examples/simple-oovcde
rm -r $dstFromTrunk/examples/staticlib-oovcde
rm -r $dstFromTrunk/examples/sharedlibgtk-oovcde

