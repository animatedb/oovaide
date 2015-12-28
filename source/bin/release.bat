

set gtk=C:\Program Files\GTK+-Bundle-3.6.4
set clang=C:\Program Files\LLVM
set mingw=C:\Program Files\mingw-w64\i686-5.1.0-posix-dwarf-rt_v4-rev0\mingw32
set trnk=..
set dst=..\..\backup\oovaide
rd /s /q %dst%
mkdir %dst%

rem update clang in source directory area
rd /s /q .\clang
xcopy /s /Y "%clang%\bin\libclang.dll" .\
rem This is for LLVM 3.6 and before
rem xcopy /s /Y /i "%clang%\lib\clang\*.*" .\clang\
rem This is for LLVM 3.7
xcopy /s /Y /i "%clang%\lib\*.*" ..\lib
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

copy "%gtk%\bin\libatk-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libcairo-2.dll" %dst%\bin
copy "%gtk%\bin\libcairo-gobject-2.dll" %dst%\bin
copy "%gtk%\bin\libexpat-1.dll" %dst%\bin
copy "%gtk%\bin\libffi-6.dll" %dst%\bin
copy "%gtk%\bin\libfontconfig-1.dll" %dst%\bin
copy "%gtk%\bin\libfreetype-6.dll" %dst%\bin
copy "%gtk%\bin\libgcc_s_dw2-1.dll" %dst%\bin
copy "%gtk%\bin\libgdk-3-0.dll" %dst%\bin
copy "%gtk%\bin\libgdk_pixbuf-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libgio-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libglib-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libgmodule-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libgobject-2.0-0.dll" %dst%\bin
copy "%gtk%\bin\libgtk-3-0.dll" %dst%\bin
copy "%gtk%\bin\libiconv-2.dll" %dst%\bin
copy "%gtk%\bin\libintl-8.dll" %dst%\bin
copy "%gtk%\bin\libpango-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libpangocairo-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libpangoft2-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libpangowin32-1.0-0.dll" %dst%\bin
copy "%gtk%\bin\libpixman-1-0.dll" %dst%\bin
copy "%gtk%\bin\libpng15-15.dll" %dst%\bin
copy "%gtk%\bin\zlib1.dll" %dst%\bin
copy "%mingw%\bin\libstdc++-6.dll" %dst%\bin
copy "%mingw%\bin\libwinpthread-1.dll" %dst%\bin
pause
