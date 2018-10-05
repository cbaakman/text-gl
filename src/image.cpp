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

#include <math.h>
#include <algorithm>

#include <cairo/cairo.h>

#include "image.h"


namespace TextGL
{
    class CairoImage: public Image
    {
        private:
            cairo_surface_t *pSurface;
        public:
            CairoImage(const size_t w, const size_t h)
            {
                pSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);

                cairo_status_t status = cairo_surface_status(pSurface);
                if (status != CAIRO_STATUS_SUCCESS)
                {
                    throw FontImageError("%s while creating a cairo surface", cairo_status_to_string(status));
                }
            }

            ~CairoImage(void)
            {
                cairo_surface_destroy(pSurface);
            }

            const void *GetData(void) const
            {
                cairo_image_surface_get_data(pSurface);
            }

            ImageDataFormat GetFormat(void) const
            {
                return IMAGEFORMAT_ARGB32;
            }

            void GetDimensions(size_t &w, size_t &h) const
            {
                w = cairo_image_surface_get_width(pSurface);
                h = cairo_image_surface_get_height(pSurface);
            }

            friend CairoImage *MakeCairoGlyphImage(const FontData &fontData,
                                                   const FontStyle &style,
                                                   const GlyphData &glyphData);
    };

    void CairoArcTo(cairo_t *cr, const double currentX, const double currentY,
                                 double rx, double ry, const double rotate,
                                 const bool largeArc, const bool sweep, const double x, const double y)
    {
        if (rx == 0.0 || ry == 0.0)  // means straight line
        {
            cairo_line_to(cr, x, y);
            return;
        }
        else if (x == currentX && y == currentY)
            return;

        double radiiRatio = ry / rx,

               dx = x - currentX,
               dy = y - currentY,

               // Transform the target point from elipse to circle
               xe = dx * cos(-rotate) - dy * sin(-rotate),
               ye =(dy * cos(-rotate) + dx * sin(-rotate)) / radiiRatio,

               // angle between the line from current to target point and the x axis
               angle = atan2(ye, xe);

        // Move the target point onto the x axis
        // The current point was already on the x-axis
        xe = sqrt(xe * xe + ye * ye);
        ye = 0.0;

        // Update the first radius if it is too small
        rx = std::max(rx, xe / 2);

        // Find one circle centre
        double xc = xe / 2,
               yc = sqrt(rx * rx - xc * xc);

        // fix for a glitch, appearing on some machines:
        if (rx == xc)
            yc = 0.0;

        // Use the flags to pick a circle center
        if (!(largeArc != sweep))
            yc = -yc;

        // Rotate the target point and the center back to their original circle positions

        double sinAngle = sin(angle),
               cosAngle = cos(angle);

        ye = xe * sinAngle;
        xe = xe * cosAngle;

        double ax = xc * cosAngle - yc * sinAngle,
               ay = yc * cosAngle + xc * sinAngle;
        xc = ax;
        yc = ay;

        // Find the drawing angles, from center to current and target points on circle:
        double angle1 = atan2(0.0 - yc, 0.0 - xc),  // current is shifted to 0,0
               angle2 = atan2( ye - yc,  xe - xc);

        cairo_save(cr);
        cairo_translate(cr, currentX, currentY);
        cairo_rotate(cr, rotate);
        cairo_scale(cr, 1.0, radiiRatio);

        if (sweep)
        {
            cairo_arc(cr, xc, yc, rx, angle1, angle2);
        }
        else
        {
            cairo_arc_negative(cr, xc, yc, rx, angle1, angle2);
        }

        cairo_restore(cr);
    }

    void PathToCairo(const std::list<GlyphPathElement> &path, cairo_t *cr)
    {
        double currentX = 0.0,
               currentY = 0.0;
        for (const GlyphPathElement &el : path)
        {
            switch (el.type)
            {
            case ELEMENT_MOVETO:
                cairo_move_to(cr, el.x, el.y);
                break;
            case ELEMENT_LINETO:
                cairo_line_to(cr, el.x, el.y);
                break;
            case ELEMENT_CURVETO:
                cairo_curve_to(cr, el.x1, el.y1, el.x2, el.y2, el.x, el.y);
                break;
            case ELEMENT_ARCTO:
                CairoArcTo(cr, currentX, currentY,
                               el.rx, el.ry,
                               el.rotate, el.largeArc, el.sweep,
                               el.x, el.y);
                break;
            case ELEMENT_CLOSEPATH:
                cairo_close_path(cr);
                break;
            default:
                throw FontImageError("Unsupported path element: %x", el.type);
            }

            currentX = el.x;
            currentY = el.y;
        }
    }

    /**
     *  Don't set scale if the stroke width has to be transformed in cairo space.
     */
    void CairoDrawPath(cairo_t *cr, const FontStyle &style, const double scale=1.0)
    {
        if (style.fillColor.a > 0.0)
        {
            cairo_set_source_rgba(cr, style.fillColor.r, style.fillColor.g, style.fillColor.b, style.fillColor.a);

            cairo_fill_preserve(cr);
        }

        if (style.strokeWidth > 0.0 && style.strokeColor.a > 0.0)
        {
            cairo_set_source_rgba(cr, style.strokeColor.r, style.strokeColor.g, style.strokeColor.b, style.strokeColor.a);
            cairo_set_line_width(cr, style.strokeWidth / scale);

            switch (style.lineJoin)
            {
            case LINEJOIN_MITER:
                cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
                break;
            case LINEJOIN_ROUND:
                cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
                break;
            case LINEJOIN_BEVEL:
                cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
                break;
            default:
                throw FontImageError("Unsupported line join type: %x", style.lineJoin);
            }

            switch (style.lineCap)
            {
            case LINECAP_BUTT:
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
                break;
            case LINECAP_ROUND:
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                break;
            case LINECAP_SQUARE:
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
                break;
            default:
                throw FontImageError("Unsupported line cap type: %x", style.lineCap);
            }

            cairo_stroke(cr);
        }
    }

    CairoImage *MakeCairoGlyphImage(const FontData &fontData,
                                    const FontStyle &style,
                                    const GlyphData &glyphData)
    {
        double scale = style.size / fontData.mMetrics.unitsPerEM;

        /* Cairo surfaces and OpenGL textures have integer dimensions, but
         * glyph bouding boxes consist floating points. Make sure the bounding box fits onto the texture:
         */
        int w = (int)ceil((fontData.mMetrics.bbox.right - fontData.mMetrics.bbox.left) * scale),
            h = (int)ceil((fontData.mMetrics.bbox.top - fontData.mMetrics.bbox.bottom) * scale);

        CairoImage *pCairoImage = new CairoImage(w, h);

        cairo_t *cr = cairo_create(pCairoImage->pSurface);

        cairo_status_t status = cairo_status(cr);
        if (status != CAIRO_STATUS_SUCCESS)
        {
            delete pCairoImage;
            throw FontImageError("%s while creating a cairo context", cairo_status_to_string(status));
        }

        cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

        // Within the cairo surface, move to the glyph's coordinate system.
        cairo_scale(cr, scale, scale);
        cairo_translate(cr, -fontData.mMetrics.bbox.left, -fontData.mMetrics.bbox.bottom);

        #ifdef DEBUG
            // Draw a red rectangle to indicate the bounding box.
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
            cairo_rectangle(cr, fontData.mMetrics.bbox.left, fontData.mMetrics.bbox.bottom,
                                fontData.mMetrics.bbox.right - fontData.mMetrics.bbox.left,
                                fontData.mMetrics.bbox.top - fontData.mMetrics.bbox.bottom);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        #endif  // DEBUG

        // Set the path in cairo.
        PathToCairo(glyphData.mPath, cr);

        // Fill it in, according to the font style.
        CairoDrawPath(cr, style, scale);
        cairo_surface_flush(pCairoImage->pSurface);

        // Don't need this anymore, we're done drawing to the surface.
        cairo_destroy(cr);

        return pCairoImage;
    }

    void ScaleGlyphMetrics(const GlyphMetrics &metricsSrc, const double scale, GlyphMetrics &metricsDest)
    {
        metricsDest.bearingX = metricsSrc.bearingX * scale;
        metricsDest.bearingY = metricsSrc.bearingY * scale;
        metricsDest.width = metricsSrc.width * scale;
        metricsDest.height = metricsSrc.height * scale;
        metricsDest.advanceX = metricsSrc.advanceX * scale;
    }

    void DestroyImageGlyph(ImageGlyph *p)
    {
        delete p->mImage;
        delete p;
    }

    ImageGlyph *MakeImageGlyph(const FontData &fontData,
                               const FontStyle &style,
                               const GlyphData &glyphData)
    {
        double scale = style.size / fontData.mMetrics.unitsPerEM;

        ImageGlyph *pImageGlyph = new ImageGlyph;

        pImageGlyph->mImage = MakeCairoGlyphImage(fontData, style, glyphData);

        ScaleGlyphMetrics(glyphData.mMetrics, scale, pImageGlyph->mMetrics);

        return pImageGlyph;
    }

    void ScaleFontMetrics(const FontMetrics &metricsSrc, const double scale, FontMetrics &metricsDest)
    {
        metricsDest.unitsPerEM = metricsSrc.unitsPerEM * scale;
        metricsDest.ascent = metricsSrc.ascent * scale;
        metricsDest.descent = metricsSrc.descent * scale;

        metricsDest.bbox.left = metricsSrc.bbox.left * scale;
        metricsDest.bbox.right = metricsSrc.bbox.right * scale;
        metricsDest.bbox.top = metricsSrc.bbox.top * scale;
        metricsDest.bbox.bottom = metricsSrc.bbox.bottom * scale;
    }

    void ScaleKernTable(const KernTable &tableSrc, const double scale, KernTable &tableDest)
    {
        for (const std::pair<UTF8Char, std::unordered_map<UTF8Char, double>> &pair1 : tableSrc)
        {
            UTF8Char c1 = std::get<0>(pair1);
            std::unordered_map<UTF8Char, double> m2 = std::get<1>(pair1);
            for (const std::pair<UTF8Char, double> &pair2 : m2)
            {
                UTF8Char c2 = std::get<0>(pair2);

                double k = std::get<1>(pair2);

                tableDest[c1][c2] = k * scale;
            }
        }
    }

    ImageFont *MakeImageFont(const FontData &fontData, const FontStyle &style)
    {
        double scale = style.size / fontData.mMetrics.unitsPerEM;

        ImageFont *pImageFont = new ImageFont;

        ScaleFontMetrics(fontData.mMetrics, scale, pImageFont->mMetrics);

        ScaleKernTable(fontData.mHorizontalKernTable, scale, pImageFont->mHorizontalKernTable);

        for (const std::pair<UTF8Char, GlyphData> &pair : fontData.mGlyphs)
        {
            UTF8Char c = std::get<0>(pair);

            try
            {
                ImageGlyph *pGlyph = MakeImageGlyph(fontData, style, fontData.mGlyphs.at(c));
                pImageFont->mGlyphs[c] = pGlyph;
            }
            catch (...)
            {
                DestroyImageFont(pImageFont);
                std::rethrow_exception(std::current_exception());
            }
        }

        return pImageFont;
    }
    void DestroyImageFont(ImageFont *p)
    {
        if (p == NULL)
            return;

        for (auto &pair : p->mGlyphs)
        {
            DestroyImageGlyph(std::get<1>(pair));
        }

        delete p;
    }

    ImageGlyph::ImageGlyph(void)
    {
    }
    ImageGlyph::~ImageGlyph(void)
    {
    }
    ImageFont::ImageFont(void)
    {
    }
    ImageFont::~ImageFont(void)
    {
    }
    const FontStyle *ImageFont::GetStyle(void) const
    {
        return &style;
    }
    const FontMetrics *ImageFont::GetMetrics(void) const
    {
        return &mMetrics;
    }
    const KernTable *ImageFont::GetHorizontalKernTable(void) const
    {
        return &mHorizontalKernTable;
    }
    const ImageGlyph *ImageFont::GetGlyph(const UTF8Char c) const
    {
        if (mGlyphs.find(c) == mGlyphs.end())
            throw MissingGlyphError(c);

        return mGlyphs.at(c);
    }
    const GlyphMetrics *ImageFont::GetGlyphMetrics(const UTF8Char c) const
    {
        return GetGlyph(c)->GetMetrics();
    }
    const GlyphMetrics *ImageGlyph::GetMetrics(void) const
    {
        return &mMetrics;
    }
}
