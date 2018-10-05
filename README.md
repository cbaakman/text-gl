# Text GL 1.0.0
A libray that facilitates working with text rendering in OpenGL.

The only supported input font format is SVG. (for now)
See: https://www.w3.org/TR/SVG11/fonts.html
Horizontal kerning is also supported. Either by class or by table.

See also the wiki at https://github.com/cbaakman/text-gl/wiki to find out how to import fonts and render text.

## Contents
* An include dir with headers for the library
* A src dir, containing the implementation
* A test dir, containing a unit test and a visual test

## Dependencies
* GNU/MinGW C++ compiler 4.7 or higher.
* LibXML 2.0 or higher: http://xmlsoft.org/
* cairo 1.10.2 or higher: https://cairographics.org/
* OpenGL 3.2 or higher, should be installed on your OS by default, if the hardware supports it.

For the tests, also:
* Boost 1.68.0 or higher: https://www.boost.org/
* GLM 0.9.9.2 or higher: https://glm.g-truc.net/0.9.9/index.html
* GLEW 2.1.0 or higher: http://glew.sourceforge.net/
* LibSDL 2.0.8 or higher: https://www.libsdl.org/download-2.0.php

## Building the Library
On linux, run 'make'. It will generate a .so file under 'lib'.

On Windows run 'build.cmd'. It will generate a .dll file under 'bin' and an import .a under 'lib'.

## Running the Tests
On Linux, run 'make test'.

On Windows, the test is executed automatically when you build the library.

## Installing
On Linux, run 'make install'. Or run 'make uninstall' to undo the installation.

On Windows, add the .dll under 'bin', the import .a under 'lib' and the headers under 'include' to your build path.
