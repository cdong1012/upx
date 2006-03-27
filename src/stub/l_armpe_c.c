/* l_armpe_c.c -- ARM/PE decompressor for NRV2E

   This file is part of the UPX executable compressor.

   Copyright (C) 1996-2006 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996-2006 Laszlo Molnar
   Copyright (C) 2000-2006 John F. Reiser
   All Rights Reserved.

   UPX and the UCL library are free software; you can redistribute them
   and/or modify them under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer              Laszlo Molnar
   <mfx@users.sourceforge.net>          <ml1050@users.sourceforge.net>

   John F. Reiser
   <jreiser@users.sourceforge.net>
*/

int thumb_nrv2e_d8(const unsigned char * src, unsigned src_len,
                   unsigned char * dst, unsigned * dst_len);
#define ucl_nrv2e_decompress_8 thumb_nrv2e_d8

void *LoadLibraryW(const unsigned short *);
void *GetProcAddressA(const void *, const void *);

static void *get_le32(const unsigned char *p)
{
    //return (void *) (p[0] + p[1] * 0x100 + p[2] * 0x10000 + p[3] * 0x1000000);

    // the code below is 4 bytes shorter than the above when compiled
    unsigned ret;
    int ic;
    for (ic = 3; ic >= 0; ic--)
        ret = ret * 0x100 + p[ic];
    return (void *) ret;
}

static void handle_imports(const unsigned char *imp, unsigned name_offset,
                           unsigned iat_offset)
{
    unsigned short buf[64];
    while (1)
    {
        unsigned short *b;
        //printf("name=%p iat=%p\n", get_le32(imp), get_le32(imp + 4));
        unsigned char *name = get_le32(imp);
        if (name == 0)
            break;
        name += name_offset;
        unsigned *iat = get_le32(imp + 4) + iat_offset;
        //printf("name=%p iat=%p\n", name, iat);
        for (b = buf; *name; name++, b++)
            *b = *name;
        *b = 0;

        void *dll = LoadLibraryW(buf);
        imp += 8;
        unsigned ord;

        while (*imp)
        {
            switch (*imp++)
            {
            case 1:
                // by name
                *iat++ = (unsigned) GetProcAddressA(dll, imp);
                while (*imp++)
                    ;
                break;
            case 0xff:
                // by ordinal
                ord = ((unsigned) imp[0]) + imp[1] * 0x100;
                imp += 2;
                *iat++ = (unsigned) GetProcAddressA(dll, (void *) ord);
                break;
            default:
                // *(int*) 1 = 0;
                break;
            }
        }
        imp++;
    }
}

// debugging stuff
int CFWrap(short *, int, int, int, int, int, int);
void WFwrap(int, const void *, int, int *, int);
void CHWrap(int);
#define WRITEFILE2(name0, buf, len) \
    do { short b[3]; b[0] = '\\'; b[1] = name0; b[2] = 0; \
    int h = CFwrap(b, 0x40000000L, 3, 0, 2, 0x80, 0);\
    int l; WFwrap(h, buf, len, &l, 0); \
    CHwrap(h); \
    } while (0)

void upx_main(const unsigned *info)
{
    int dlen = 0;
    unsigned src0 = *info++;
    unsigned srcl = *info++;
    unsigned dst0 = *info++;
    unsigned dstl = *info++;
    unsigned bimp = *info++;
    unsigned onam = *info++;
    unsigned getp = *info++;
    unsigned load = *info++;
    unsigned entr = *info++;

    //WRITEFILE2('0', (void*) 0x11000, load + 256 - 0x11000);
    ucl_nrv2e_decompress_8((void *) src0, srcl, (void *) dst0, &dlen);
    //WRITEFILE2('1', (void*) 0x11000, load + 256 - 0x11000);
    handle_imports((void *) bimp, onam, dst0);
    //WRITEFILE2('2', (void*) 0x11000, load + 256 - 0x11000);
}
