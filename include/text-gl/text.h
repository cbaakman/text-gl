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

#ifndef TEXT_H
#define TEXT_H

#include <cfloat>

#include "tex.h"

namespace TextGL
{
    struct TextSelectionDetails
    {
        // One dimensional, character position of the selection in the text string.
        size_t startPosition, endPosition;

        GLfloat startX, endX, baseY,  // should render the glyph at startX, baseY
                ascent, descent;  // relative to baseY
    };

    /**
     *  Might be thrown if the text doesn't fit within the given width.
     */
    class TextFormatError: public TextGLError
    {
        public:
            TextFormatError(const char *format, ...);
    };

    struct GlyphVertex
    {
        GLfloat x, y, tx, ty;
    };

    struct GlyphQuad
    {
        GlyphVertex vertices[4];  // counter clockwise
        GLuint texture;
    };

    enum TextAlign
    {
        TEXTALIGN_LEFT,  // extend right from startX
        TEXTALIGN_CENTER,  // extend around startX
        TEXTALIGN_RIGHT  // extend left from startX
    };

    struct TextParams
    {
        GLfloat startX, startY,
                maxWidth,
                lineSpacing;  // between two baselines
        TextAlign align;
    };

    GLfloat GetLineHeight(const GLTextureFont *);

    class GLTextLeftToRightIterator
    {
        protected:
            virtual void OnGlyph(const UTF8Char c, const GlyphQuad &, const TextSelectionDetails &) {}

            virtual void OnLine(const TextSelectionDetails &) {}
        public:

            /**
             *  NULL-terminated UTF-8 encoding is assumed for input sequence 'text'.
             *
             *  Glyphs are placed from small x (left) to high x (right)
             *  and lines are placed from high y (up) to low y (down).
             */
            void IterateText(const GLTextureFont *, const int8_t *text,
                             const TextParams &);
    };

    size_t CountLines(const Font *, const int8_t *text, const TextParams &);
}

#endif  // TEXT_H
