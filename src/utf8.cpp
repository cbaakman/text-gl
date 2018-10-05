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

#include <iostream>

#include "utf8.h"

namespace TextGL
{
    int CountSuccessiveLeftBits(const int8_t byte)
    {
        int n = 0;
        while (n < 8 && (byte & (0b0000000010000000 >> n)))
            n ++;

        return n;
    }
    const int8_t *NextUTF8Char(const int8_t *bytes, UTF8Char &ch)
    {
        size_t nBytes,
               i;

        /*
            The first bits of the first byte determine the length
            of the utf-8 character. The number of bytes equals the
            number of uninterrupted 1 bits at the beginning.
            The remaining first byte bits are considered coding.
            ???????? : 0 bytes
            1??????? : 1 bytes
            11?????? : 2 bytes
            110????? : stop, assume 2 byte code!
              ^^^^^^
         */

        nBytes = CountSuccessiveLeftBits(bytes[0]);

        // Always include the first byte.
        ch = 0x000000ff & bytes[0];

        if (nBytes == 0)
        {
            // Assume ascii.
            return bytes + 1;
        }

        /*
            Complement the unicode identifier from the remaining encoding bits.
            All but the first UTF-8 byte must start in 10.., so six coding
            bits per byte:
                            .. 10?????? 10?????? 10?????? ..
         */
        for (i = 1; i < nBytes; i++)
        {
            if ((bytes[i] & 0b11000000) != 0b10000000)
            {
                throw EncodingError("utf-8 byte %u (0x%x) not starting in 10.. !", (i + 1), bytes[i]);
            }

            ch = (ch << 8) | 0x000000ff & bytes[i];
        }

        // Move to the next utf-8 character pointer.
        return bytes + nBytes;
    }
    const int8_t *PrevUTF8Char(const int8_t *bytes, UTF8Char &ch)
    {
        size_t nBytes = 0,
               nBits;
        bool beginFound = false;
        int8_t byte;

        ch = 0x00000000;
        while (!beginFound)
        {
            // Take one byte:
            nBytes++;
            byte = *(bytes - nBytes);

            // is it 10??????
            beginFound = (byte & 0b11000000) != 0b10000000;

            ch |= (0x000000ff & byte) << (8 * (nBytes - 1));
        }

        // Only ascii chars are allowed to start in a 0-bit.
        nBits = CountSuccessiveLeftBits(byte);
        if (nBits != nBytes && nBytes > 1)
        {
            throw EncodingError("%u successive bits, but %u bytes", nBits, nBytes);
        }

        return bytes - nBytes;
    }
    const int8_t *GetUTF8Position(const int8_t *bytes, const size_t n)
    {
        size_t i = 0;
        UTF8Char ch;
        while (i < n)
        {
            bytes = NextUTF8Char(bytes, ch);
            i++;
        }
        return bytes;
    }
    size_t CountCharsUTF8(const int8_t *bytes, const int8_t *end)
    {
        size_t n = 0;
        UTF8Char ch;
        while (*bytes)
        {
            bytes = NextUTF8Char(bytes, ch);
            n++;
            if (end and bytes >= end)
                return n;
        }
        return n;
    }
}
