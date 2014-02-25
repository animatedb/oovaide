#!/bin/sh

cmake ..
mkdir bin
cp ../bin/* bin
cp oovBuilder/oovBuilder bin
cp oovcde/oovcde bin
cp oovCMaker/oovCMaker bin
cp oovCppParser/oovCppParser bin
cp oovEdit/oovEdit bin

