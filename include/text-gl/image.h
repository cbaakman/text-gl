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

#ifndef IMAGE_H
#define IMAGE_H

#include "font.h"


namespace TextGL
{
    class GLTextureGlyph;
    class GLTextureFont;


    enum ImageDataFormat
    {
        IMAGEFORMAT_RGBA32,
        IMAGEFORMAT_ARGB32
    };

    class Image
    {
        public:
            virtual const void *GetData(void) const = 0;
            virtual ImageDataFormat GetFormat(void) const = 0;
            virtual void GetDimensions(size_t &w, size_t &h) const = 0;
    };

    class ImageGlyph
    {
        private:
            GlyphMetrics mMetrics;  // transformed by size

            Image *mImage;

            ImageGlyph(void);
            ~ImageGlyph(void);

            void operator=(const ImageGlyph &) = delete;
            ImageGlyph(const ImageGlyph &) = delete;
        public:
            const GlyphMetrics *GetMetrics(void) const;

        friend ImageGlyph *MakeImageGlyph(const FontData &,
                                          const FontStyle &,
                                          const GlyphData &);
        friend GLTextureGlyph *MakeGLTextureGlyph(const ImageGlyph *);
        friend void DestroyImageGlyph(ImageGlyph *);
    };

    class ImageFont: public Font
    {
        private:
            FontMetrics mMetrics;  // transformed by size
            FontStyle style;

            std::unordered_map<UTF8Char, ImageGlyph *> mGlyphs;
            KernTable mHorizontalKernTable;  // transformed by size

            ImageFont(void);
            ~ImageFont(void);

            void operator=(const ImageFont &) = delete;
            ImageFont(const ImageFont &) = delete;
        public:
            const FontStyle *GetStyle(void) const;
            const FontMetrics *GetMetrics(void) const;
            const KernTable *GetHorizontalKernTable(void) const;
            const GlyphMetrics *GetGlyphMetrics(const UTF8Char) const;
            const ImageGlyph *GetGlyph(const UTF8Char) const;

        friend ImageFont *MakeImageFont(const FontData &, const FontStyle &);
        friend GLTextureFont *MakeGLTextureFont(const ImageFont *);
        friend void DestroyImageFont(ImageFont *);
    };

    ImageFont *MakeImageFont(const FontData &, const FontStyle &);
    void DestroyImageFont(ImageFont *);

    class FontImageError: public TextGLError
    {
        public:
            FontImageError(const char *format, ...);
    };
}

#endif  // IMAGE_H
