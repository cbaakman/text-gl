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

#include "text.h"


namespace TextGL
{
    void SetTextSelection(const GLTextureFont *pFont,
                          const size_t startPosition, const size_t endPosition,
                          const GLfloat startX, const GLfloat endX, const GLfloat baseY,
                          TextSelectionDetails &details)
    {
        details.startPosition = startPosition;
        details.endPosition = endPosition;
        details.startX = startX;
        details.endX = endX;
        details.baseY = baseY;
        details.ascent = pFont->GetMetrics()->ascent;
        details.descent = pFont->GetMetrics()->descent;
    }

    void SetGlyphQuad(const GLTextureFont *pFont, const UTF8Char c,
                      const GLfloat x, const GLfloat y,
                      GlyphQuad &quad)
    {
        const GLTextureGlyph *pGlyph = pFont->GetGlyph(c);
        const GlyphMetrics *pGlyphMetrics = pGlyph->GetMetrics();
        const FontMetrics *pFontMetrics = pFont->GetMetrics();

        quad.texture = pGlyph->GetTexture();

        GLsizei tw, th;
        pGlyph->GetTextureDimensions(tw, th);

        // top left
        quad.vertices[3].x = x + pFontMetrics->bbox.left + pGlyphMetrics->bearingX;
        quad.vertices[3].y = y + pFontMetrics->bbox.top + pGlyphMetrics->bearingY;
        quad.vertices[3].tx = 0.0f;
        quad.vertices[3].ty = 1.0f;

        // top right
        quad.vertices[2].x = quad.vertices[3].x + tw;
        quad.vertices[2].y = quad.vertices[3].y;
        quad.vertices[2].tx = 1.0f;
        quad.vertices[2].ty = 1.0f;

        // bottom right
        quad.vertices[1].x = quad.vertices[2].x;
        quad.vertices[1].y = quad.vertices[2].y - th;
        quad.vertices[1].tx = 1.0f;
        quad.vertices[1].ty = 0.0f;

        // bottom left
        quad.vertices[0].x = quad.vertices[3].x;
        quad.vertices[0].y = quad.vertices[3].y - th;
        quad.vertices[0].tx = 0.0f;
        quad.vertices[0].ty = 0.0f;
    }

    double GetKernValue(const KernTable &kernTable, const UTF8Char c1, const UTF8Char c2)
    {
        if (kernTable.find(c1) == kernTable.end())
            return 0.0;

        const auto &m2 = kernTable.at(c1);

        if (m2.find(c2) == m2.end())
            return 0.0;

        return m2.at(c2);
    }

    bool IsSpace(const UTF8Char c)
    {
        return c == ' ' || c == '\t';
    }

    const int8_t *SkipSpaces(const int8_t *p)
    {
        const int8_t *next;
        UTF8Char c;

        while (true)
        {
            next = NextUTF8Char(p, c);
            if (c == NULL || !IsSpace(c))
                return p;
            else
                p = next;
        }
    }

    bool AtLineEnding(const int8_t *p, const int8_t *&past)
    {
        UTF8Char c;

        past = NextUTF8Char(p, c);
        if (c == '\n')
            return true;
        else if (c == '\r')  // Detect windows line endings.
        {
            past = NextUTF8Char(p, c);
            if (c == '\n')
                return true;
        }
        return false;
    }

    bool AtStringEnding(const int8_t *p)
    {
        UTF8Char c;

        NextUTF8Char(p, c);

        return c == NULL;
    }

    GLfloat NextWordWidth(const Font *pFont, const int8_t *text, const int8_t *&pEnd)
    {
        GLfloat w = 0.0f;
        UTF8Char c, cPrev = NULL;
        const int8_t *p = text, *next;

        // First read all the whitespaces preceeding the word.
        while (true)
        {
            next = NextUTF8Char(p, c);
            if (AtStringEnding(p) || AtLineEnding(p, next))
            {
                // Don't count "  \n" as a word.
                pEnd = p;
                return 0.0f;
            }
            else if (!IsSpace(c))
                break;

            if (cPrev != NULL)
                w += GetKernValue(*(pFont->GetHorizontalKernTable()), cPrev, c);

            w += pFont->GetGlyphMetrics(c)->advanceX;

            p = next;
            cPrev = c;
        }

        // Next read until the first whitespace.
        while (true)
        {
            next = NextUTF8Char(p, c);
            if (AtStringEnding(p) || AtLineEnding(p, next) || IsSpace(c))
            {
                pEnd = p;
                return w;
            }

            if (cPrev != NULL)
                w += GetKernValue(*(pFont->GetHorizontalKernTable()), cPrev, c);

            w += pFont->GetGlyphMetrics(c)->advanceX;

            p = next;
            cPrev = c;
        }
    }

    GLfloat NextLineWidth(const Font *pFont, const int8_t *text, const GLfloat maxLineWidth, const int8_t *&pLineEnd)
    {
        GLfloat lineWidth = 0.0f, wordWidth;
        const int8_t *p = SkipSpaces(text),
                     *pWordEnd, *past;
        UTF8Char c = NULL;
        while (true)
        {
            wordWidth = NextWordWidth(pFont, p, pWordEnd);
            if (wordWidth > maxLineWidth)
                throw TextFormatError("Next word of \"%s\" doesn't fit in line width %f", text, maxLineWidth);
            else if ((lineWidth + wordWidth) > maxLineWidth)
            {
                pLineEnd = p;
                return lineWidth;
            }

            lineWidth += wordWidth;
            p = pWordEnd;

            // Check what character ended the word.
            if (AtStringEnding(p) || AtLineEnding(p, past))
            {
                pLineEnd = p;
                return lineWidth;
            }
        }
    }

    void GLTextLeftToRightIterator::IterateText(const GLTextureFont *pFont, const int8_t *text, const TextParams &params)
    {
        GlyphQuad quad;
        TextSelectionDetails glyphSelection, lineSelection;
        GLfloat x, y = params.startY, x0,
                lineWidth;

        size_t count;
        const int8_t *p = text, *next, *pLineEnd;
        UTF8Char c, cPrev;
        while (!AtStringEnding(p))
        {
            lineWidth = NextLineWidth(pFont, p, params.maxWidth, pLineEnd);
            cPrev = NULL;
            if (params.align == TEXTALIGN_CENTER)
                x = params.startX - lineWidth / 2;

            else if (params.align == TEXTALIGN_RIGHT)
                x = params.startX - lineWidth;

            else  // default TEXTALIGN_LEFT
                x = params.startX;

            p = SkipSpaces(p);

            SetTextSelection(pFont,
                             CountCharsUTF8(text, p), CountCharsUTF8(text, pLineEnd),
                             x, x + lineWidth, y, lineSelection);
            OnLine(lineSelection);

            while (p < pLineEnd)
            {
                x0 = x;

                next = NextUTF8Char(p, c);

                if (cPrev != NULL)
                    x += GetKernValue(*(pFont->GetHorizontalKernTable()), cPrev, c);

                SetGlyphQuad(pFont, c, x, y, quad);

                x += pFont->GetGlyph(c)->GetMetrics()->advanceX;

                SetTextSelection(pFont,
                                 CountCharsUTF8(text, p), CountCharsUTF8(text, next),
                                 x0, x, y, glyphSelection);
                OnGlyph(c, quad, glyphSelection);

                cPrev = c;
                p = next;
            }

            // See what character ended the line.
            if (AtStringEnding(p))
                return;

            else if (AtLineEnding(p, next))
                p = next;

            // Otherwise it was just a whitespace.

            y -= params.lineSpacing;
        }
    }

    size_t CountLines(const Font *pFont, const int8_t *text, const TextParams &params)
    {
        const int8_t *p = text, *next;
        size_t count = 0;

        while (!AtStringEnding(p))
        {
            count++;

            NextLineWidth(pFont, p, params.maxWidth, next);

            p = next;

            // See what character ended the line.
            if (AtStringEnding(p))
                break;

            else if (AtLineEnding(p, next))
                p = next;

            // Otherwise it was just a whitespace.
        }

        return count;
    }

    GLfloat GetLineHeight(const GLTextureFont *pFont)
    {
        const FontMetrics *pMetrics = pFont->GetMetrics();
        return pMetrics->ascent - pMetrics->descent;
    }
}


