
set gtk=C:\msys64\mingw64
rem set clang=C:\Program Files\LLVM
set clang=C:\msys64\mingw64\
set trnk=..
set dst=..\..\backup\oovaide
rd /s /q %dst%
mkdir %dst%

rem This is for LLVM 3.9 in msys2
xcopy /s /Y "%clang%\bin\libclang.dll" %dst%\bin\
xcopy /s /Y /i "%clang%\lib\clang\*.*" %dst%\lib\

del ..\lib\LTO.dll
del OovAideSettings.txt

del /s %dst%\web\*.bak
xcopy /s %trnk%\..\web\*.* %dst%\web\
xcopy /s %trnk%\bin\*.* %dst%\bin\
del %dst%\bin\sqlite3.dll
xcopy /s /Y %trnk%\bin\data\*.* %dst%\bin\data\
xcopy /s %trnk%\*.cpp %dst%
xcopy /s /Y %trnk%\*.h %dst%
xcopy /s %trnk%\*.html %dst%
xcopy /s /Y %trnk%\*.txt %dst%
xcopy /s %trnk%\*.py %dst%
xcopy /s %trnk%\*.in %dst%
xcopy /s %trnk%\*.cproject %dst%
xcopy /s %trnk%\*.project %dst%
rd /s /q %dst%\.metadata
rd /s /q %dst%\RemoteSystemsTempFiles

xcopy /s /Y .\ %dst%\bin\
xcopy /s /Y ..\lib %dst%\lib\

xcopy /s "%gtk%\share\glib-2.0\schemas" %dst%\share\glib-2.0\schemas\

robocopy %gtk%\share\icons\ %CD%\%dst%\share\icons\ justify-fill.png /S
robocopy %gtk%\share\icons\ %CD%\%dst%\share\icons\ document-open.png /S
robocopy %gtk%\share\icons\ %CD%\%dst%\share\icons\ preferences.7png /S
robocopy %gtk%\share\icons\ %CD%\%dst%\share\icons\ process-stop.png /S
robocopy %gtk%\share\icons\ %CD%\%dst%\share\icons\ window-*.png /S
robocopy %gtk%\share\icons\ %CD%\%dst%\share\icons\ go-top.png /S
robocopy %gtk%\share\icons\ %CD%\%dst%\share\icons\ go-up.png /S
robocopy %gtk%\share\icons\ %CD%\%dst%\share\icons\ go-down.png /S
robocopy %gtk%\share\icons\ %CD%\%dst%\share\icons\ go-bottom.png /S

copy "%gtk%\bin\libatk-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libbz2-1.dll" %dst%\bin
copy "%gtk%\bin\libcairo-2.dll" %dst%\bin
copy "%gtk%\bin\libcairo-gobject-2.dll" %dst%\bin
copy "%gtk%\bin\libepoxy-0.dll" %dst%\bin
copy "%gtk%\bin\libexpat-1.dll" %dst%\bin
copy "%gtk%\bin\libffi-6.dll" %dst%\bin
copy "%gtk%\bin\libfontconfig-1.dll" %dst%\bin
copy "%gtk%\bin\libfreetype-6.dll" %dst%\bin
copy "%gtk%\bin\libgcc_s_seh-1.dll" %dst%\bin
copy "%gtk%\bin\libgdk_pixbuf-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libgdk-3-0.dll" %dst%\bin
copy "%gtk%\bin\libgio-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libglib-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libgmodule-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libgobject-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libgraphite2.dll" %dst%\bin
copy "%gtk%\bin\libgtk-3-0.dll" %dst%\bin
copy "%gtk%\bin\libharfbuzz-0.dll" %dst%\bin
copy "%gtk%\bin\libiconv-2.dll" %dst%\bin
copy "%gtk%\bin\libintl-8.dll" %dst%\bin
copy "%gtk%\bin\libpango-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libpangocairo-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libpangoft2-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libpangowin32-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libpcre-1.dll" %dst%\bin
copy "%gtk%\bin\libpixman-1-0.dll" %dst%\bin
copy "%gtk%\bin\libpng16-16.dll" %dst%\bin
copy "%gtk%\bin\libstdc++-6.dll" %dst%\bin
copy "%gtk%\bin\libwinpthread-1.dll" %dst%\bin
copy "%gtk%\bin\zlib1.dll" %dst%\bin
pause
