#!/bin/sh

trnk=..
dst=../../../backup/oovaide
#clang=../../../clang+llvm-3.4-x86_64
dstFromTrunk=../../backup/oovaide

rm -r $dst
mkdir $dst

find ../test/trunk-oovaide-*/analysis* -delete
find ../test/trunk-oovaide-*/oovaide-tmp* -delete

rsync -av --exclude='.svn' $trnk/../web/ $dst/web/
rsync -av --exclude='.svn' $trnk/bin-linux/ $dst/bin/
#rsync -av $clang/lib/clang $dst/lib/
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
rm -r $dstFromTrunk/examples/simple-oovaide
rm -r $dstFromTrunk/examples/staticlib-oovaide
rm -r $dstFromTrunk/examples/sharedlibgtk-oovaide

