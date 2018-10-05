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


#ifndef TEX_H
#define TEX_H


#include <GL/glew.h>
#include <GL/gl.h>

#include "image.h"


namespace TextGL
{
    class GLTextureGlyph
    {
        private:
            GlyphMetrics mMetrics;  // transformed by size

            GLuint texture;
            GLsizei textureWidth, textureHeight;  // Never smaller than the metrics.

            GLTextureGlyph(void);
            ~GLTextureGlyph(void);

            void operator=(const GLTextureGlyph &) = delete;
            GLTextureGlyph(const GLTextureGlyph &) = delete;
        public:
            const GlyphMetrics *GetMetrics(void) const;
            GLuint GetTexture(void) const;
            void GetTextureDimensions(GLsizei &width, GLsizei &height) const;

        friend GLTextureGlyph *MakeGLTextureGlyph(const ImageGlyph *);
        friend void DestroyGLTextureGlyph(GLTextureGlyph *);
    };

    class GLTextureFont: public Font
    {
        private:
            FontMetrics mMetrics;  // transformed by size
            FontStyle style;

            std::unordered_map<UTF8Char, GLTextureGlyph *> mGlyphs;
            KernTable mHorizontalKernTable;  // transformed by size

            GLTextureFont(void);
            ~GLTextureFont(void);

            void operator=(const GLTextureFont &) = delete;
            GLTextureFont(const GLTextureFont &) = delete;
        public:
            const FontMetrics *GetMetrics(void) const;
            const FontStyle *GetStyle(void) const;
            const GLTextureGlyph *GetGlyph(const UTF8Char) const;
            const GlyphMetrics *GetGlyphMetrics(const UTF8Char) const;
            const KernTable *GetHorizontalKernTable(void) const;

        friend GLTextureFont *MakeGLTextureFont(const ImageFont *);
        friend void DestroyGLTextureFont(GLTextureFont *);
    };

    // A valid GL context is required to call these functions.
    GLTextureFont *MakeGLTextureFont(const ImageFont *);
    void DestroyGLTextureFont(GLTextureFont *);

    class GLError: public TextGLError
    {
        public:
            GLError(const GLenum, const char *filename, const size_t line);
            GLError(const char *format, ...);
    };
}

#endif  // TEX_H
