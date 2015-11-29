
set OUT=out
set SRC=..
set CLASSPATH=C:\Program Files\Java\jdk1.8.0_51\lib\tools.jar
mkdir %OUT%
del %OUT%\*.class
javac -d %OUT% @sources.txt
cd %OUT%
jar cfm oovJavaParser.jar %SRC%\Manifest.txt *.class parser\*.class model\*.class common\*.class
cd ..
pause
set EXECOUT=..\bin\
copy %OUT%\oovJavaParser.jar %EXECOUT%
java -cp "%EXECOUT%\oovJavaParser.jar;%CLASSPATH%" oovJavaParser parser/AnalysisParser.java ./ out/ana
