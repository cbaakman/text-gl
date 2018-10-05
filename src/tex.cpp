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

#include "tex.h"

#ifdef DEBUG
    #define CHECK_GL() { GLenum err = glGetError(); if (err != GL_NO_ERROR) throw FontGLError(err, __FILE__, __LINE__); }
#else
    #define CHECK_GL()
#endif

namespace TextGL
{
    GLTextureGlyph::GLTextureGlyph(void)
    {
    }
    GLTextureGlyph::~GLTextureGlyph(void)
    {
    }
    GLTextureFont::GLTextureFont(void)
    {
    }
    GLTextureFont::~GLTextureFont(void)
    {
    }
    void DestroyGLTextureGlyph(GLTextureGlyph *pTextureGlyph)
    {
        glDeleteTextures(1, &(pTextureGlyph->texture));
        CHECK_GL();

        delete pTextureGlyph;
    }
    GLTextureGlyph *MakeGLTextureGlyph(const ImageGlyph *pImageGlyph)
    {
        GLTextureGlyph *pTextureGlyph = new GLTextureGlyph;
        pTextureGlyph->mMetrics = pImageGlyph->mMetrics;

        size_t w, h;
        pImageGlyph->mImage->GetDimensions(w, h);
        pTextureGlyph->textureWidth = w;
        pTextureGlyph->textureHeight = h;

        glGenTextures(1, &(pTextureGlyph->texture));
        CHECK_GL();

        if (pTextureGlyph->texture == NULL)
            throw GLError("No GL texture was generated");

        glBindTexture(GL_TEXTURE_2D, pTextureGlyph->texture);
        CHECK_GL();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        CHECK_GL();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECK_GL();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        CHECK_GL();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        CHECK_GL();

        switch (pImageGlyph->mImage->GetFormat())
        {
        case IMAGEFORMAT_RGBA32:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                         pTextureGlyph->textureWidth, pTextureGlyph->textureHeight,
                         0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
                         pImageGlyph->mImage->GetData());
            break;
        case IMAGEFORMAT_ARGB32:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                         pTextureGlyph->textureWidth, pTextureGlyph->textureHeight,
                         0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                         pImageGlyph->mImage->GetData());
            break;
        default:
            throw FontImageError("Unsupported image format: %x", pImageGlyph->mImage->GetFormat());
        }
        CHECK_GL();

        glBindTexture(GL_TEXTURE_2D, NULL);
        CHECK_GL();

        return pTextureGlyph;
    }

    GLTextureFont *MakeGLTextureFont(const ImageFont *pImageFont)
    {
        GLTextureFont *pTextureFont = new GLTextureFont;
        pTextureFont->mHorizontalKernTable = pImageFont->mHorizontalKernTable;
        pTextureFont->mMetrics = pImageFont->mMetrics;
        pTextureFont->style = pImageFont->style;

        for (const auto &pair : pImageFont->mGlyphs)
        {
            const UTF8Char c = std::get<0>(pair);
            const ImageGlyph *pImageGlyph = std::get<1>(pair);

            pTextureFont->mGlyphs[c] = MakeGLTextureGlyph(pImageGlyph);
        }

        return pTextureFont;
    }
    void DestroyGLTextureFont(GLTextureFont *pTextureFont)
    {
        for (const auto &pair : pTextureFont->mGlyphs)
        {
            DestroyGLTextureGlyph(std::get<1>(pair));
        }

        delete pTextureFont;
    }
    const GlyphMetrics *GLTextureGlyph::GetMetrics(void) const
    {
        return &mMetrics;
    }
    GLuint GLTextureGlyph::GetTexture(void) const
    {
        return texture;
    }
    void GLTextureGlyph::GetTextureDimensions(GLsizei &width, GLsizei &height) const
    {
        width = textureWidth;
        height = textureHeight;
    }
    const FontMetrics *GLTextureFont::GetMetrics(void) const
    {
        return &mMetrics;
    }
    const FontStyle *GLTextureFont::GetStyle(void) const
    {
        return &style;
    }
    const GLTextureGlyph *GLTextureFont::GetGlyph(const UTF8Char c) const
    {
        if (mGlyphs.find(c) == mGlyphs.end())
            throw MissingGlyphError(c);

        return mGlyphs.at(c);
    }
    const GlyphMetrics *GLTextureFont::GetGlyphMetrics(const UTF8Char c) const
    {
        return GetGlyph(c)->GetMetrics();
    }
    const KernTable *GLTextureFont::GetHorizontalKernTable(void) const
    {
        return &mHorizontalKernTable;
    }
}
