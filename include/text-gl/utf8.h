/* Copyright (C) 2018 Coos Baakman
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef UTF8_H
#define UTF8_H

#include <exception>
#include <string>

#include "error.h"


namespace TextGL
{
    typedef int32_t UTF8Char;  // 4 byte placeholders

    const int8_t *NextUTF8Char(const int8_t *bytes, UTF8Char &c);
    const int8_t *PrevUTF8Char(const int8_t *bytes, UTF8Char &c);
    size_t CountCharsUTF8(const int8_t *start, const int8_t *end=NULL);
    const int8_t *GetUTF8Position(const int8_t *bytes, const size_t characterNumber);

    class EncodingError: public TextGLError
    {
        public:
            EncodingError(const char *format, ...);
    };
}

#endif  // UTF8_H
