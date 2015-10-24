
set OUT=out
set SRC=..
mkdir %OUT%
del %OUT%\*.class
javac -d %OUT% @sources.txt
cd %OUT%
jar cfm oovJavaParser.jar %SRC%\Manifest.txt *.class parser\*.class model\*.class
cd ..
pause
set EXECOUT=..\bin\
copy %OUT%\oovJavaParser.jar %EXECOUT%
java -cp "%EXECOUT%\oovJavaParser.jar;%JAVA_HOME%\lib\tools.jar" oovJavaParser parser/Parser.java ./ out/ana
