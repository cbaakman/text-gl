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

#include <cstdarg>
#include <cstring>

#include "text.h"


namespace TextGL
{
    FontParseError::FontParseError(const char *format, ...)
    {
        va_list pArgs;
        va_start(pArgs, format);

        vsnprintf(buffer, ERRORBUF_SIZE, format, pArgs);

        va_end(pArgs);
    }
    FontImageError::FontImageError(const char *format, ...)
    {
        va_list pArgs;
        va_start(pArgs, format);

        vsnprintf(buffer, ERRORBUF_SIZE, format, pArgs);

        va_end(pArgs);
    }
    EncodingError::EncodingError(const char *format, ...)
    {
        va_list pArgs;
        va_start(pArgs, format);

        vsnprintf(buffer, ERRORBUF_SIZE, format, pArgs);

        va_end(pArgs);
    }
    TextFormatError::TextFormatError(const char *format, ...)
    {
        va_list pArgs;
        va_start(pArgs, format);

        vsnprintf(buffer, ERRORBUF_SIZE, format, pArgs);

        va_end(pArgs);
    }
    MissingGlyphError::MissingGlyphError(const UTF8Char c)
    {
        snprintf(buffer, ERRORBUF_SIZE, "No glyph for \'%c\'", c);
    }
    GLError::GLError(const GLenum err, const char *filename, const size_t line)
    {
        switch (err)
        {
        case GL_NO_ERROR:
            snprintf(buffer, ERRORBUF_SIZE, "GL_NO_ERROR at %s line %u", filename, line);
            break;
        case GL_INVALID_ENUM:
            snprintf(buffer, ERRORBUF_SIZE, "GL_INVALID_ENUM at %s line %u", filename, line);
            break;
        case GL_INVALID_VALUE:
            snprintf(buffer, ERRORBUF_SIZE, "GL_INVALID_VALUE at %s line %u", filename, line);
            break;
        case GL_INVALID_OPERATION:
            snprintf(buffer, ERRORBUF_SIZE, "GL_INVALID_OPERATION at %s line %u", filename, line);
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            snprintf(buffer, ERRORBUF_SIZE, "GL_INVALID_FRAMEBUFFER_OPERATION at %s line %u", filename, line);
            break;
        case GL_OUT_OF_MEMORY:
            snprintf(buffer, ERRORBUF_SIZE, "GL_OUT_OF_MEMORY at %s line %u", filename, line);
            break;
        case GL_STACK_UNDERFLOW:
            snprintf(buffer, ERRORBUF_SIZE, "GL_STACK_UNDERFLOW at %s line %u", filename, line);
            break;
        case GL_STACK_OVERFLOW:
            snprintf(buffer, ERRORBUF_SIZE, "GL_STACK_OVERFLOW at %s line %u", filename, line);
            break;
        default:
            snprintf(buffer, ERRORBUF_SIZE, "unknown GL error 0x%x at %s line %u", err, filename, line);
            break;
        }
    }
    GLError::GLError(const char *format, ...)
    {
        va_list pArgs;
        va_start(pArgs, format);

        vsnprintf(buffer, ERRORBUF_SIZE, format, pArgs);

        va_end(pArgs);
    }
    const char *TextGLError::what(void) const noexcept
    {
        return buffer;
    }
}
