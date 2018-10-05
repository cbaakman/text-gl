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

#ifndef FONT_H
#define FONT_H

#include <unordered_map>
#include <list>
#include <iostream>

#include "utf8.h"


namespace TextGL
{
    struct GlyphMetrics
    {
        double bearingX, bearingY,
               width, height,
               advanceX;
    };

    struct FontBoundingBox 
    {
        double left, bottom, right, top;
    };

    struct FontMetrics
    {
        double unitsPerEM,
               ascent, descent;  // Together, these two determine the height of one line.

        FontBoundingBox bbox;
    };


    typedef std::unordered_map<UTF8Char, std::unordered_map<UTF8Char, double>> KernTable;

    /**
     *  returns 0.0 if the combination doesn't exist.
     */
    double GetKernValue(const KernTable &, const UTF8Char first, const UTF8Char second);

    enum GlyphPathElementType
    {
        ELEMENT_MOVETO,  // uses x, y
        ELEMENT_LINETO,  // uses x, y
        ELEMENT_CURVETO,  // uses x1, y1, x2, y2, x, y
        ELEMENT_ARCTO,  // uses rx, ry, rotate(radians), largeArc, sweep, x, y
        ELEMENT_CLOSEPATH  // has no arguments
    };

    struct GlyphPathElement
    {
        GlyphPathElementType type;

        union
        {
            struct
            {
                double x1, y1, x2, y2;
            };
            struct
            {
                double rx, ry, rotate;
                bool largeArc, sweep;
            };
        };
        double x, y;
    };

    struct GlyphData
    {
        GlyphMetrics mMetrics;
        std::list<GlyphPathElement> mPath;
    };

    struct FontData
    {
        FontMetrics mMetrics;
        std::unordered_map<UTF8Char, GlyphData> mGlyphs;
        KernTable mHorizontalKernTable;
    };

    void ParseSVGFontData(std::istream &, FontData &);

    class FontParseError: public TextGLError
    {
        public:
            FontParseError(const char *format, ...);
    };

    class MissingGlyphError: public TextGLError
    {
        public:
            MissingGlyphError(const UTF8Char c);
    };

    struct Color
    {
        float r, g, b, a;
    };

    enum LineJoinType
    {
        LINEJOIN_MITER, LINEJOIN_ROUND, LINEJOIN_BEVEL
    };

    enum LineCapType
    {
        LINECAP_BUTT, LINECAP_ROUND, LINECAP_SQUARE
    };

    struct FontStyle
    {
        double size, strokeWidth;
        Color fillColor,
              strokeColor;
        LineJoinType lineJoin;
        LineCapType lineCap;
    };

    class Font
    {
        public:
            virtual const FontMetrics *GetMetrics(void) const = 0;
            virtual const FontStyle *GetStyle(void) const = 0;
            virtual const KernTable *GetHorizontalKernTable(void) const = 0;
            virtual const GlyphMetrics *GetGlyphMetrics(const UTF8Char) const = 0;
    };
}

#endif  // FONT_H
