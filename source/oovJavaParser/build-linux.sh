#!/bin/sh

OUT=out
SRC=..
JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64;
CLASSPATH=$JAVA_HOME/lib/tools.jar
mkdir $OUT
rm $OUT\*.class
javac -cp $CLASSPATH -d $OUT @sources.txt
cd $OUT
jar cfm oovJavaParser.jar $SRC/Manifest.txt *.class parser/*.class model/*.class
cd ..

#read -rp "Press any key to continue..." key

EXEC_OUT=../bin-linux/
cp $OUT/oovJavaParser.jar $EXEC_OUT

java -cp "$OUT/oovJavaParser.jar:$JAVA_HOME/lib/tools.jar" oovJavaParser parser/Parser.java ./ out/ana

