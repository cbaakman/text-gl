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
#include <math.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <boost/algorithm/string.hpp>

#include "font.h"


# define PI 3.14159265358979323846

#define HAS_ID(map, id) (map.find(id) != map.end())

namespace TextGL
{
    class XMLChildIterator : public std::iterator<std::input_iterator_tag, xmlNodePtr>
    {
        private:
            xmlNodePtr pChildren;
        public:
            XMLChildIterator(xmlNodePtr p): pChildren(p) {}
            XMLChildIterator(const XMLChildIterator &it): pChildren(it.pChildren) {}

            XMLChildIterator &operator++(void)
            {
                pChildren = pChildren->next;
                return *this;
            }
            XMLChildIterator operator++(int)
            {
                XMLChildIterator tmp(*this);
                operator++();
                return tmp;
            }
            bool operator==(const XMLChildIterator &other) const
            {
                return pChildren == other.pChildren;
            }
            bool operator!=(const XMLChildIterator &other) const
            {
                return pChildren != other.pChildren;
            }
            xmlNodePtr operator*(void)
            {
                return pChildren;
            }
    };

    class XMLChildIterable
    {
        private:
            xmlNodePtr pParent;
        public:
            XMLChildIterable(xmlNodePtr p): pParent(p) {}

            XMLChildIterator begin(void)
            {
                return XMLChildIterator(pParent->children);
            }

            XMLChildIterator end(void)
            {
                return XMLChildIterator(nullptr);
            }
    };

    bool HasChild(xmlNodePtr pTag, const char *tagName)
    {
        for (xmlNodePtr pChild : XMLChildIterable(pTag))
        {
            if (boost::iequals((const char *)pChild->name, tagName))
                return true;
        }
        return false;
    }

    xmlNodePtr FindChild(xmlNodePtr pParent, const char *tagName)
    {
        for (xmlNodePtr pChild : XMLChildIterable(pParent))
        {
            if (boost::iequals((const char *)pChild->name, tagName))
                return pChild;
        }

        throw FontParseError("No %s tag found in %s tag", tagName, (const char *)pParent->name);
    }

    std::list<xmlNodePtr> IterFindChildren(xmlNodePtr pParent, const char *tagName)
    {
        std::list<xmlNodePtr> l;

        for (xmlNodePtr pChild : XMLChildIterable(pParent))
        {
            if (boost::iequals((const char *)pChild->name, tagName))
                l.push_back(pChild);
        }

        return l;
    }

    xmlDocPtr ParseXML(std::istream &is)
    {
        const size_t bufSize = 1024;
        std::streamsize res;
        char buf[bufSize];
        int wellFormed;

        xmlParserCtxtPtr pCtxt;
        xmlDocPtr pDoc;

        // Read the first 4 bytes.
        is.read(buf, 4);
        if (!is.good())
            throw FontParseError("Error reading the first xml bytes!");
        res = is.gcount();

        // Create a progressive parsing context.
        pCtxt = xmlCreatePushParserCtxt(NULL, NULL, buf, res, NULL);
        if (!pCtxt)
            throw FontParseError("Failed to create parser context!");

        // Loop on the input, getting the document data.
        while (is.good())
        {
            is.read(buf, bufSize);
            res = is.gcount();

            xmlParseChunk(pCtxt, buf, res, 0);
        }

        // There is no more input, indicate the parsing is finished.
        xmlParseChunk(pCtxt, buf, 0, 1);

        // Check if it was well formed.
        pDoc = pCtxt->myDoc;
        wellFormed = pCtxt->wellFormed;
        xmlFreeParserCtxt(pCtxt);

        if (!wellFormed)
        {
            xmlFreeDoc(pDoc);
            throw FontParseError("xml document is not well formed");
        }

        return pDoc;
    }

    /**
     *  Locale independent, always uses a dot as decimal separator.
     *
     *  returns the pointer to the string after the number.
     */
    const char *ParseDouble(const char *in, double &out)
    {
        double d = 10.0f;
        int digit, ndigit = 0;

        // start from zero
        out = 0.0;

        const char *p = in;
        while (*p)
        {
            if (isdigit(*p))
            {
                digit = (*p - '0');

                if (d > 1.0) // left from period
                {
                    out *= d;
                    out += digit;
                }
                else // right from period, decimal
                {
                    out += d * digit;
                    d *= 0.1;
                }
                ndigit++;
            }
            else if (tolower(*p) == 'e')
            {
                // exponent

                // if no digits precede the exponent, assume 1
                if (ndigit <= 0)
                    out = 1.0;

                p++;
                if (*p == '+') // '+' just means positive power, default
                    p++; // skip it, don't give it to atoi

                int e = atoi(p);

                out = out * pow(10, e);

                // read past exponent number
                if (*p == '-')
                    p++;

                while (isdigit(*p))
                    p++;

                return p;
            }
            else if (*p == '.')
            {
                // expect decimal digits after this

                d = 0.1;
            }
            else if (*p == '-')
            {
                // negative number
                double v;
                p = ParseDouble(p + 1, v);

                out = -v;

                return p;
            }
            else
            {
                // To assume success, must have read at least one digit
                if (ndigit > 0)
                    return p;
                else
                    return nullptr;
            }
            p++;
        }

        return p;
    }

    void ParseUnicodeAttrib(const xmlNodePtr pTag, const char *key, UTF8Char &c)
    {
        xmlChar *pS = xmlGetProp(pTag, (const xmlChar *)key);
        if (pS == nullptr)
            throw FontParseError("Missing %s attribute: %s", (const char *)pTag->name, key);
        int len = xmlStrlen(pS);
        if (len > 4)
            throw FontParseError("%s attribute %s has length %s. Expecting unicode", (const char *)pTag->name, key, len);

        // libxml2 automatically converts the html code to utf-8.

        if (*NextUTF8Char((const int8_t *)pS, c) != NULL)
            throw FontParseError("Cannot read string %s as utf-8", pS);

        xmlFree(pS);
    }

    void ParseStringAttrib(const xmlNodePtr pTag, const char *key, std::string &s)
    {
        xmlChar *pS = xmlGetProp(pTag, (const xmlChar *)key);
        if (pS == nullptr)
            throw FontParseError("Missing %s attribute: %s", (const char *)pTag->name, key);

        s.assign((const char *)pS);
        xmlFree(pS);
    }

    void ParseDoubleAttrib(const xmlNodePtr pTag, const char *key, double &d)
    {
        xmlChar *pS = xmlGetProp(pTag, (const xmlChar *)key);
        if (pS == nullptr)
            throw FontParseError("Missing %s attribute: %s", (const char *)pTag->name, key);

        if (ParseDouble((const char *)pS, d) == NULL)
            throw FontParseError("Cannot convert string %s to number", pS);
        xmlFree(pS);
    }

    void ParseBoundingBoxAttrib(const xmlNodePtr pTag, FontBoundingBox &bbox)
    {
        std::string s;
        ParseStringAttrib(pTag, "bbox", s);

        double numbers[4];
        size_t i;
        const char *p = s.c_str();
        for (i = 0; i < 4; i++)
        {
            while (isspace(*p)) p++;
            if ((p = ParseDouble(p, numbers[i])) == NULL)
                throw FontParseError("bbox attribute doesn't contain 4 numbers");
        }

        bbox.left = numbers[0];
        bbox.bottom = numbers[1];
        bbox.right = numbers[2];
        bbox.top = numbers[3];
    }

    /**
     *  Parses n comma/space separated floats from the given string and
     *  returns a pointer to the text after it.
     */
    const char *SVGParsePathDoubles(const int n, const char *text, double outs [])
    {
        int i;
        for (i = 0; i < n; i++)
        {
            if (!*text)
                return NULL;

            while (isspace(*text) || *text == ',')
            {
                text++;
                if (!*text)
                    return NULL;
            }

            text = ParseDouble(text, outs[i]);
            if (!text)
                return NULL;
        }

        return text;
    }

    /**
     *  This function, used to draw quadratic curves in cairo, is
     *  based on code from cairosvg(http://cairosvg.org/)
     */
    void Quadratic2Bezier(double &x1, double &y1, double &x2, double &y2, const double x, const double y)
    {
        double xq1 = (x2) * 2 / 3 + (x1) / 3,
               yq1 = (y2) * 2 / 3 + (y1) / 3,
               xq2 = (x2) * 2 / 3 + x / 3,
               yq2 = (y2) * 2 / 3 + y / 3;

        x1 = xq1;
        y1 = yq1;
        x2 = xq2;
        y2 = yq2;
    }

    void ParseSVGPath(const char *d, std::list<GlyphPathElement> &path)
    {
        const char *nd;
        double ds[6],
               qx1, qy1, qx2, qy2;

        char prevSymbol, symbol = 'm';
        bool upper = false;
        GlyphPathElement el;

        while (*d)
        {
            prevSymbol = symbol;

            // Get the next path symbol:

            while (isspace(*d))
                d++;

            upper = isupper(*d); // upper is absolute, lower is relative
            symbol = tolower(*d);
            d++;

            // Take the approriate action for the symbol:
            switch (symbol)
            {
            case 'z':  // closepath

                el.type = ELEMENT_CLOSEPATH;
                path.push_back(el);
                break;

            case 'm':  // moveto(x y)+

                while ((nd = SVGParsePathDoubles(2, d, ds)))
                {
                    if (upper)
                    {
                        el.x = ds[0];
                        el.y = ds[1];
                    }
                    else
                    {
                        el.x += ds[0];
                        el.y += ds[1];
                    }

                    el.type = ELEMENT_MOVETO;
                    path.push_back(el);

                    d = nd;
                }
                break;

            case 'l':  // lineto(x y)+

                while ((nd = SVGParsePathDoubles(2, d, ds)))
                {
                    if (upper)
                    {
                        el.x = ds[0];
                        el.y = ds[1];
                    }
                    else
                    {
                        el.x += ds[0];
                        el.y += ds[1];
                    }

                    el.type = ELEMENT_LINETO;
                    path.push_back(el);

                    d = nd;
                }
                break;

            case 'h':  // horizontal lineto x+

                while ((nd = SVGParsePathDoubles(1, d, ds)))
                {
                    if (upper)
                    {
                        el.x = ds[0];
                    }
                    else
                    {
                        el.x += ds[0];
                    }

                    el.type = ELEMENT_LINETO;
                    path.push_back(el);

                    d = nd;
                }
                break;

            case 'v':  // vertical lineto y+

                while ((nd = SVGParsePathDoubles(1, d, ds)))
                {
                    if (upper)
                    {
                        el.y = ds[0];
                    }
                    else
                    {
                        el.y += ds[0];
                    }

                    el.type = ELEMENT_LINETO;
                    path.push_back(el);

                    d = nd;
                }
                break;

            case 'c':  // curveto(x1 y1 x2 y2 x y)+

                while ((nd = SVGParsePathDoubles(6, d, ds)))
                {
                    if (upper)
                    {
                        el.x1 = ds[0]; el.y1 = ds[1];
                        el.x2 = ds[2]; el.y2 = ds[3];
                        el.x = ds[4]; el.y = ds[5];
                    }
                    else
                    {
                        el.x1 = el.x + ds[0]; el.y1 = el.y + ds[1];
                        el.x2 = el.x + ds[2]; el.y2 = el.y + ds[3];
                        el.x += ds[4]; el.y += ds[5];
                    }

                    el.type = ELEMENT_CURVETO;
                    path.push_back(el);

                    d = nd;
                }
                break;

            case 's':  // shorthand/smooth curveto(x2 y2 x y)+

                while ((nd = SVGParsePathDoubles(4, d, ds)))
                {
                    if (prevSymbol == 's' || prevSymbol == 'c')
                    {
                        el.x1 = el.x + (el.x - el.x2);
                        el.y1 = el.y + (el.y - el.y2);
                    }
                    else
                    {
                        el.x1 = el.x;
                        el.y1 = el.y;
                    }
                    prevSymbol = symbol;

                    if (upper)
                    {
                        el.x2 = ds[0];
                        el.y2 = ds[1];
                        el.x = ds[2];
                        el.y = ds[3];
                    }
                    else
                    {
                        el.x2 = el.x + ds[0];
                        el.y2 = el.y + ds[1];
                        el.x += ds[2];
                        el.y += ds[3];
                    }

                    el.type = ELEMENT_CURVETO;
                    path.push_back(el);

                    d = nd;
                }
                break;

            case 'q': // quadratic Bezier curveto(x1 y1 x y)+

                while ((nd = SVGParsePathDoubles(4, d, ds)))
                {
                    el.x1 = el.x;
                    el.y1 = el.y;
                    if (upper)
                    {
                        el.x2 = ds[0];
                        el.y2 = ds[1];
                        el.x = ds[2];
                        el.y = ds[3];
                    }
                    else
                    {
                        el.x2 = el.x + ds[0];
                        el.y2 = el.y + ds[1];
                        el.x += ds[2];
                        el.y += ds[3];
                    }

                    // Cairo doesn't do quadratic, so fake it:

                    qx1 = el.x1;
                    qy1 = el.y1;
                    qx2 = el.x2;
                    qy2 = el.y2;
                    Quadratic2Bezier(qx1, qy1, qx2, qy2, el.x, el.y);
                    el.x1 = qx1;
                    el.y1 = qy1;
                    el.x2 = qx2;
                    el.y2 = qy2;

                    el.type = ELEMENT_CURVETO;
                    path.push_back(el);

                    d = nd;
                }
                break;

            case 't':  // Shorthand/smooth quadratic Bézier curveto(x y)+

                while ((nd = SVGParsePathDoubles(2, d, ds)))
                {
                    el.x1 = el.x;
                    el.y1 = el.y;
                    if (prevSymbol == 't' || prevSymbol == 'q')
                    {
                        el.x2 = el.x + (el.x - el.x2);
                        el.y2 = el.y + (el.y - el.y2);
                    }
                    else
                    {
                        el.x2 = el.x;
                        el.y2 = el.y;
                    }

                    if (upper)
                    {
                        el.x = ds[0];
                        el.y = ds[1];
                    }
                    else
                    {
                        el.x += ds[0];
                        el.y += ds[1];
                    }

                    // Cairo doesn't do quadratic, so fake it:

                    qx1 = el.x1;
                    qy1 = el.y1;
                    qx2 = el.x2;
                    qy2 = el.y2;
                    Quadratic2Bezier(qx1, qy1, qx2, qy2, el.x, el.y);

                    el.type = ELEMENT_CURVETO;
                    path.push_back(el);

                    d = nd;
                }
                break;

            case 'a':  // elliptical arc(rx ry x-axis-rotation large-arc-flag sweep-flag x y)+

                while (isspace(*d))
                    d++;

                while (isdigit(*d) || *d == '-')
                {
                    nd = SVGParsePathDoubles(3, d, ds);
                    if (!nd)
                    {
                        throw FontParseError("arc incomplete. Missing first 3 floats in %s", d);
                    }

                    el.rx = ds[0];
                    el.ry = ds[1];
                    el.rotate = ds[2] * PI / 180;  // convert degrees to radians

                    while (isspace(*nd) || *nd == ',')
                        nd++;

                    if (!isdigit(*nd))
                    {
                        throw FontParseError("arc incomplete. Missing large arc digit in %s", d);
                    }

                    el.largeArc = *nd != '0';

                    do {
                        nd++;
                    } while (isspace(*nd) || *nd == ',');

                    if (!isdigit(*nd))
                    {
                        throw FontParseError("arc incomplete. Missing sweep digit in %s", d);
                    }

                    el.sweep = *nd != '0';

                    do
                    {
                        nd++;
                    }
                    while (isspace(*nd) || *nd == ',');

                    nd = SVGParsePathDoubles(2, nd, ds);
                    if (!nd)
                    {
                        throw FontParseError("arc incomplete. Missing last two floats in %s", d);
                    }

                    while (isspace(*nd))
                        nd++;

                    d = nd;

                    // end points:
                    if (upper)
                    {
                        el.x = ds[0];
                        el.y = ds[1];
                    }
                    else
                    {
                        el.x += ds[0];
                        el.y += ds[1];
                    }

                    el.type = ELEMENT_ARCTO;
                    path.push_back(el);
                }  // end loop

                break;
            }
        }
    }

    void ParseGlyphTag(const xmlNodePtr pGlyphTag, const GlyphMetrics &defaults,
                       FontData &fontData,
                       std::unordered_map<std::string, UTF8Char> &namesToCharacters)
    {
        if (!xmlHasProp(pGlyphTag, (const xmlChar *)"unicode"))
            return;

        UTF8Char c;
        ParseUnicodeAttrib(pGlyphTag, "unicode", c);

        if (xmlHasProp(pGlyphTag, (const xmlChar *)"glyph-name"))
        {
            std::string name;
            ParseStringAttrib(pGlyphTag, "glyph-name", name);

            namesToCharacters[name] = c;
        }

        fontData.mGlyphs[c].mMetrics = defaults;
        if (xmlHasProp(pGlyphTag, (const xmlChar *)"horiz-adv-x"))
            ParseDoubleAttrib(pGlyphTag, "horiz-adv-x", fontData.mGlyphs[c].mMetrics.advanceX);
        if (xmlHasProp(pGlyphTag, (const xmlChar *)"horiz-origin-x"))
            ParseDoubleAttrib(pGlyphTag, "horiz-origin-x", fontData.mGlyphs[c].mMetrics.bearingX);
        if (xmlHasProp(pGlyphTag, (const xmlChar *)"horiz-origin-y"))
            ParseDoubleAttrib(pGlyphTag, "horiz-origin-y", fontData.mGlyphs[c].mMetrics.bearingY);

        std::string d = "";
        if (xmlHasProp(pGlyphTag, (const xmlChar *)"d"))  // 'd' might be missing for a whitespace glyph
            ParseStringAttrib(pGlyphTag, "d", d);
        ParseSVGPath(d.c_str(), fontData.mGlyphs[c].mPath);
    }

    void ParseGlyphNameListAttrib(xmlNodePtr pTag, const char *id, std::list<std::string> &names)
    {
        std::string value;
        ParseStringAttrib(pTag, id, value);

        boost::split(names, value, boost::is_any_of(","));
    }

    void ParseGlyphUnicodeListAttrib(xmlNodePtr pTag, const char *id, std::list<UTF8Char> &characters)
    {
        std::string value;
        ParseStringAttrib(pTag, id, value);

        std::list<std::string> reprs;
        boost::split(reprs, value, boost::is_any_of(","));
        for (const std::string &repr : reprs)
        {
            UTF8Char c;
            if (*NextUTF8Char((const int8_t *)repr.c_str(), c) != NULL)
                throw FontParseError("Error interpreting %s attribute %s %s as utf-8", (const char *)pTag->name, id, repr.c_str());
            characters.push_back(c);
        }
    }

    void ParseHKernTag(const xmlNodePtr pHKernTag,
                       const std::unordered_map<std::string, UTF8Char> &namesToCharacters,
                       FontData &fontData)
    {
        double k;
        ParseDoubleAttrib(pHKernTag, "k", k);

        std::list<std::string> g1, g2;
        std::list<UTF8Char> u1, u2;

        if (xmlHasProp(pHKernTag, (const xmlChar *)"g1"))
        {
            ParseGlyphNameListAttrib(pHKernTag, "g1", g1);
            for (std::string &name : g1)
            {
                if (!HAS_ID(namesToCharacters, name))
                    throw FontParseError("No such glyph: %s", name);
                u1.push_back(namesToCharacters.at(name));
            }
        }
        if (xmlHasProp(pHKernTag, (const xmlChar *)"g2"))
        {
            ParseGlyphNameListAttrib(pHKernTag, "g2", g2);
            for (std::string &name : g2)
            {
                if (!HAS_ID(namesToCharacters, name))
                    throw FontParseError("No such glyph: %s", name);
                u2.push_back(namesToCharacters.at(name));
            }
        }
        if (xmlHasProp(pHKernTag, (const xmlChar *)"u1"))
            ParseGlyphUnicodeListAttrib(pHKernTag, "u1", u1);
        if (xmlHasProp(pHKernTag, (const xmlChar *)"u2"))
            ParseGlyphUnicodeListAttrib(pHKernTag, "u2", u2);

        for (const UTF8Char c1 : u1)
            for (const UTF8Char c2 : u2)
                fontData.mHorizontalKernTable[c1][c2] = k;
    }

    void ParseSVGFontData(std::istream &is, FontData &fontData)
    {
        xmlDocPtr pDoc = ParseXML(is);

        try
        {
            xmlNodePtr pRoot = xmlDocGetRootElement(pDoc);
            if(pRoot == nullptr)
                throw FontParseError("no root element found in xml tree");

            if (!boost::iequals((const char *)pRoot->name, "svg"))
                throw FontParseError("no root element is not \"svg\"");

            xmlNodePtr pDefsTag = FindChild(pRoot, "defs"),
                       pFontTag = FindChild(pDefsTag, "font"),
                       pFaceTag = FindChild(pFontTag, "font-face");

            ParseDoubleAttrib(pFaceTag, "ascent", fontData.mMetrics.ascent);
            ParseDoubleAttrib(pFaceTag, "descent", fontData.mMetrics.descent);
            ParseDoubleAttrib(pFaceTag, "units-per-em", fontData.mMetrics.unitsPerEM);

            ParseBoundingBoxAttrib(pFaceTag, fontData.mMetrics.bbox);

            GlyphMetrics defaultGlyphMetrics = {0.0f, 0.0f, 0.0f,
                                                fontData.mMetrics.bbox.top - fontData.mMetrics.bbox.bottom,
                                                fontData.mMetrics.bbox.right - fontData.mMetrics.bbox.left};
            if (xmlHasProp(pFontTag, (const xmlChar *)"horiz-adv-x"))
                ParseDoubleAttrib(pFontTag, "horiz-adv-x", defaultGlyphMetrics.advanceX);
            if (xmlHasProp(pFontTag, (const xmlChar *)"horiz-origin-x"))
                ParseDoubleAttrib(pFontTag, "horiz-origin-x", defaultGlyphMetrics.bearingX);
            if (xmlHasProp(pFontTag, (const xmlChar *)"horiz-origin-y"))
                ParseDoubleAttrib(pFontTag, "horiz-origin-y", defaultGlyphMetrics.bearingY);

            std::unordered_map<std::string, UTF8Char> namesToCharacters;

            for (xmlNodePtr pGlyphTag : IterFindChildren(pFontTag, "glyph"))
                ParseGlyphTag(pGlyphTag, defaultGlyphMetrics, fontData, namesToCharacters);

            for (xmlNodePtr pHKernTag : IterFindChildren(pFontTag, "hkern"))
                ParseHKernTag(pHKernTag, namesToCharacters, fontData);
        }
        catch(...)
        {
            xmlFreeDoc(pDoc);

            std::rethrow_exception(std::current_exception());
        }
        xmlFreeDoc(pDoc);
    }
}
