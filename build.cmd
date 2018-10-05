set CXX=g++
set CFLAGS=-std=c++17
set VERSION=1.0.0
set LIB_NAME=text-gl


if not exist obj (mkdir obj)
if not exist bin (mkdir bin)
if not exist lib (mkdir lib)

del /Q /F /S obj\* bin\%LIB_NAME%-%VERSION%.dll lib\lib%LIB_NAME%.a

:: Make the library.

@for %%m in (parse image tex utf8 error text) do (
    %CXX% %CFLAGS% -I include\text-gl -c src\%%m.cpp -o obj\%%m.o -fPIC

    @if %ERRORLEVEL% neq 0 (
        goto end
    )
)

%CXX% obj\parse.o obj\image.o obj\tex.o obj\utf8.o obj\error.o obj\text.o -lxml2 -lcairo -lopengl32 ^
-o bin\%LIB_NAME%-%VERSION%.dll -shared -fPIC -Wl,--out-implib,lib\lib%LIB_NAME%.a
@if %ERRORLEVEL% neq 0 (
    goto end
)

:: Make the tests.

%CXX% %CFLAGS% -I include -fexec-charset=UTF-8 tests\encoding.cpp lib\lib%LIB_NAME%.a ^
-lboost_unit_test_framework -o bin\test_encoding.exe && bin\test_encoding.exe

%CXX% %CFLAGS% -I include tests\visual.cpp lib\lib%LIB_NAME%.a ^
-lxml2 -lcairo -lopengl32 -lglew32 -lmingw32 -lSDL2main -lSDL2 -o bin\test_visual.exe && bin\test_visual.exe data\sample1.svg

:end
