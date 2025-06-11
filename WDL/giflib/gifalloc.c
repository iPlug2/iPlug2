/*****************************************************************************
 *   "Gif-Lib" - Yet another gif library.                     
 *                                         
 * Written by:  Gershon Elber                Ver 0.1, Jun. 1989   
 * Extensively hacked by: Eric S. Raymond        Ver 1.?, Sep 1992    
 *****************************************************************************
 * GIF construction tools                              
 *****************************************************************************
 * History:                                     
 * 15 Sep 92 - Version 1.0 by Eric Raymond.                     
 ****************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gif_lib.h"

#define MAX(x, y)    (((x) > (y)) ? (x) : (y))

/******************************************************************************
 * Miscellaneous utility functions                          
 *****************************************************************************/

/* return smallest bitfield size n will fit in */
int
BitSize(int n) {
    
    register int i;

    for (i = 1; i <= 8; i++)
        if ((1 << i) >= n)
            break;
    return (i);
}

/******************************************************************************
 * Color map object functions                              
 *****************************************************************************/

/*
 * Allocate a color map of given size; initialize with contents of
 * ColorMap if that pointer is non-NULL.
 */
ColorMapObject *
MakeMapObject(int ColorCount,
              const GifColorType * ColorMap) {
    
    ColorMapObject *Object;

    /*** FIXME: Our ColorCount has to be a power of two.  Is it necessary to
     * make the user know that or should we automatically round up instead? */
    if (ColorCount != (1 << BitSize(ColorCount))) {
        return ((ColorMapObject *) NULL);
    }
    
    Object = (ColorMapObject *)malloc(sizeof(ColorMapObject));
    if (Object == (ColorMapObject *) NULL) {
        return ((ColorMapObject *) NULL);
    }

    Object->Colors = (GifColorType *)calloc(ColorCount, sizeof(GifColorType));
    if (Object->Colors == (GifColorType *) NULL) {
        return ((ColorMapObject *) NULL);
    }

    Object->ColorCount = ColorCount;
    Object->BitsPerPixel = BitSize(ColorCount);

    if (ColorMap) {
        memcpy((char *)Object->Colors,
               (char *)ColorMap, ColorCount * sizeof(GifColorType));
    }

    return (Object);
}

/*
 * Free a color map object
 */
void
FreeMapObject(ColorMapObject * Object) {

    if (Object != NULL) {
        free(Object->Colors);
        free(Object);
        /*** FIXME:
         * When we are willing to break API we need to make this function
         * FreeMapObject(ColorMapObject **Object)
         * and do this assignment to NULL here:
         * *Object = NULL;
         */
    }
}

#ifdef DEBUG
void
DumpColorMap(ColorMapObject * Object,
             FILE * fp) {

    if (Object) {
        int i, j, Len = Object->ColorCount;

        for (i = 0; i < Len; i += 4) {
            for (j = 0; j < 4 && j < Len; j++) {
                fprintf(fp, "%3d: %02x %02x %02x   ", i + j,
                        Object->Colors[i + j].Red,
                        Object->Colors[i + j].Green,
                        Object->Colors[i + j].Blue);
            }
            fprintf(fp, "\n");
        }
    }
}
#endif /* DEBUG */

/*
 * Compute the union of two given color maps and return it.  If result can't 
 * fit into 256 colors, NULL is returned, the allocated union otherwise.
 * ColorIn1 is copied as is to ColorUnion, while colors from ColorIn2 are
 * copied iff they didn't exist before.  ColorTransIn2 maps the old
 * ColorIn2 into ColorUnion color map table.
 */
ColorMapObject *
UnionColorMap(const ColorMapObject * ColorIn1,
              const ColorMapObject * ColorIn2,
              GifPixelType ColorTransIn2[]) {

    int i, j, CrntSlot, RoundUpTo, NewBitSize;
    ColorMapObject *ColorUnion;

    /* 
     * Allocate table which will hold the result for sure.
     */
    ColorUnion = MakeMapObject(MAX(ColorIn1->ColorCount,
                               ColorIn2->ColorCount) * 2, NULL);

    if (ColorUnion == NULL)
        return (NULL);

    /* Copy ColorIn1 to ColorUnionSize; */
    /*** FIXME: What if there are duplicate entries into the colormap to begin
     * with? */
    for (i = 0; i < ColorIn1->ColorCount; i++)
        ColorUnion->Colors[i] = ColorIn1->Colors[i];
    CrntSlot = ColorIn1->ColorCount;

    /* 
     * Potentially obnoxious hack:
     *
     * Back CrntSlot down past all contiguous {0, 0, 0} slots at the end
     * of table 1.  This is very useful if your display is limited to
     * 16 colors.
     */
    while (ColorIn1->Colors[CrntSlot - 1].Red == 0
           && ColorIn1->Colors[CrntSlot - 1].Green == 0
           && ColorIn1->Colors[CrntSlot - 1].Blue == 0)
        CrntSlot--;

    /* Copy ColorIn2 to ColorUnionSize (use old colors if they exist): */
    for (i = 0; i < ColorIn2->ColorCount && CrntSlot <= 256; i++) {
        /* Let's see if this color already exists: */
        /*** FIXME: Will it ever occur that ColorIn2 will contain duplicate
         * entries?  So we should search from 0 to CrntSlot rather than
         * ColorIn1->ColorCount?
         */
        for (j = 0; j < ColorIn1->ColorCount; j++)
            if (memcmp (&ColorIn1->Colors[j], &ColorIn2->Colors[i], 
                        sizeof(GifColorType)) == 0)
                break;

        if (j < ColorIn1->ColorCount)
            ColorTransIn2[i] = j;    /* color exists in Color1 */
        else {
            /* Color is new - copy it to a new slot: */
            ColorUnion->Colors[CrntSlot] = ColorIn2->Colors[i];
            ColorTransIn2[i] = CrntSlot++;
        }
    }

    if (CrntSlot > 256) {
        FreeMapObject(ColorUnion);
        return ((ColorMapObject *) NULL);
    }

    NewBitSize = BitSize(CrntSlot);
    RoundUpTo = (1 << NewBitSize);

    if (RoundUpTo != ColorUnion->ColorCount) {
        register GifColorType *Map = ColorUnion->Colors;

        /* 
         * Zero out slots up to next power of 2.
         * We know these slots exist because of the way ColorUnion's
         * start dimension was computed.
         */
        for (j = CrntSlot; j < RoundUpTo; j++)
            Map[j].Red = Map[j].Green = Map[j].Blue = 0;

        /* perhaps we can shrink the map? */
        if (RoundUpTo < ColorUnion->ColorCount)
            ColorUnion->Colors = (GifColorType *)realloc(Map,
                                 sizeof(GifColorType) * RoundUpTo);
    }

    ColorUnion->ColorCount = RoundUpTo;
    ColorUnion->BitsPerPixel = NewBitSize;

    return (ColorUnion);
}

