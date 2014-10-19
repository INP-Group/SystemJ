#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <X11/Xmd.h>

#include "gray2image.h"


/*
 *  Note: we return the number of bits *integrally* divided by 8, so that
 *        less than 8 bits per pixel (1, 4) will result in "0", which
 *        will be refused by a check in GetGray2ImageConverter().
 */

static unsigned int BytesPerPixelOfImage(XImage *img)
{
    return img->bits_per_pixel / 8;
}

/*
 *  Note: SplitComponentMask() supposed that the mask is contiguos, so
 *        this func ...
 */

static void SplitComponentMask(unsigned long component_mask, int *max_p, int *shift_p)
{
  int  shift_factor;
  int  num_bits;
    
    /* Sanity check */
    if (component_mask == 0)
    {
        *max_p = 1;
        *shift_p = 0;
        return;
    }

    /* I. Find out how much is it shifted */
    for (shift_factor = 0;  (component_mask & 1) == 0;  shift_factor++)
        component_mask >>= 1;

    /* II. And how many bits are there */
    for (num_bits = 1;  (component_mask & 2) != 0;  num_bits++)
        component_mask >>= 1;

    /* Return values */
    *max_p   = (1 << num_bits) - 1;
    *shift_p = shift_factor;
}

static unsigned long GetComponentBits(int gray, int gray_max,
                                      int component_max, int shift_factor)
{
    return ((gray * component_max) / gray_max) << shift_factor;
}

static unsigned int GetPixelValue(int gray, int gray_max,
                                  int red_max,   int red_shift,
                                  int green_max, int green_shift,
                                  int blue_max,  int blue_shift)
{
    return GetComponentBits(gray, gray_max, red_max,   red_shift)   |
           GetComponentBits(gray, gray_max, green_max, green_shift) |
           GetComponentBits(gray, gray_max, blue_max,  blue_shift);
}

#define DECLARE_FILLER(TABLE_TYPE)                                  \
static void FillTable_##TABLE_TYPE(TABLE_TYPE *table, int bits_per_gray, XImage *img)  \
{                                                                   \
  TABLE_TYPE *p = table;                                            \
  int         count = 1 << bits_per_gray;                           \
  int         x;                                                    \
  int         red_max,   red_shift;                                 \
  int         green_max, green_shift;                               \
  int         blue_max,  blue_shift;                                \
                                                                    \
    SplitComponentMask(img->red_mask,   &red_max,   &red_shift);    \
    SplitComponentMask(img->green_mask, &green_max, &green_shift);  \
    SplitComponentMask(img->blue_mask,  &blue_max,  &blue_shift);   \
                                                                    \
    for (x = 0;  x < count;  x++)                                   \
        *p++ = GetPixelValue(x, count - 1,                          \
                             red_max,   red_shift,                  \
                             green_max, green_shift,                \
                             blue_max,  blue_shift);                \
                                                                    \
    *p++ = img->red_mask;                                           \
}

DECLARE_FILLER(CARD8)
DECLARE_FILLER(CARD16)
DECLARE_FILLER(CARD32)

typedef struct _cvt_rec_t
{
    struct _cvt_rec_t *next;

    void         *table;

    unsigned int   bytes_per_gray;
    unsigned int   bits_per_gray;
    unsigned int   img_bytes_per_pixel;
    unsigned long  red_mask;
    unsigned long  green_mask;
    unsigned long  blue_mask;
} cvt_rec_t;


static cvt_rec_t *allocated = NULL;

static cvt_rec_t *FindRec(unsigned int bytes_per_gray,
                          unsigned int bits_per_gray,
                          XImage *img)
{
  cvt_rec_t *rec = allocated;

    while(rec != NULL)
    {
        if (rec->bytes_per_gray      == bytes_per_gray             &&
            rec->bits_per_gray       == bits_per_gray              &&
            rec->img_bytes_per_pixel == BytesPerPixelOfImage(img)  &&
            rec->red_mask   == img->red_mask                       &&
            rec->green_mask == img->green_mask                     &&
            rec->blue_mask  == img->blue_mask)
            return rec;

        rec = rec->next;
    }

    return NULL;
}

static cvt_rec_t *CreateRec(unsigned int bytes_per_gray,
                            unsigned int bits_per_gray,
                            XImage *img)
{
  cvt_rec_t *rec = malloc(sizeof(cvt_rec_t));
  size_t     tablesize = ((1 << bits_per_gray) + 1) * BytesPerPixelOfImage(img);
  void      *table;
  
    if (rec == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    /* Fill in the record */
    bzero(rec, sizeof(*rec));
    rec->bytes_per_gray      = bytes_per_gray;
    rec->bits_per_gray       = bits_per_gray;
    rec->img_bytes_per_pixel = BytesPerPixelOfImage(img);
    rec->red_mask            = img->red_mask;
    rec->green_mask          = img->green_mask;
    rec->blue_mask           = img->blue_mask;

    /* Allocate a conversion table */
    rec->table = table = malloc(tablesize);
    if (table == NULL)
    {
        free(rec);
        errno = ENOMEM;
        return NULL;
    }

    /* Fill in the table */
    switch (rec->img_bytes_per_pixel)
    {
        case 1:
            FillTable_CARD8 (table, bits_per_gray, img);
            break;

        case 2:
            FillTable_CARD16(table, bits_per_gray, img);
            break;

        case 3:
        case 4:
            FillTable_CARD32(table, bits_per_gray, img);
            break;
    }

    /* Put this record into the list */
    rec->next = allocated;
    allocated = rec;

    return rec;
}

#define DECLARE_CONVERTER(SRC_TYPE, DST_TYPE)                        \
static int ConverterFrom_##SRC_TYPE##_To_##DST_TYPE(void *data, XImage *image, void *info)  \
{                                                                    \
  register DST_TYPE *table   = ((cvt_rec_t *)(info))->table;         \
  register SRC_TYPE *src;                                            \
  register DST_TYPE *dst;                                            \
  int                interline_delta;                                \
                                                                     \
  int     x;                                                         \
  int     y;                                                         \
                                                                     \
    src = data;                                                      \
    dst = (DST_TYPE *)(image->data);                                 \
    interline_delta = (image->bytes_per_line - image->width * sizeof(DST_TYPE)) / sizeof(DST_TYPE);  \
                                                                     \
    for (y = image->height;  y > 0;  y--)                            \
    {                                                                \
        for (x = image->width;  x > 0;  x--)                         \
        {                                                            \
            *dst++ = table[*src++];                                  \
        }                                                            \
        dst += interline_delta;                                      \
    }                                                                \
    return 0;                                                        \
}

DECLARE_CONVERTER(CARD8,  CARD8)
DECLARE_CONVERTER(CARD16, CARD8)
DECLARE_CONVERTER(CARD32, CARD8)
DECLARE_CONVERTER(CARD8,  CARD16)
DECLARE_CONVERTER(CARD16, CARD16)
DECLARE_CONVERTER(CARD32, CARD16)
DECLARE_CONVERTER(CARD8,  CARD32)
DECLARE_CONVERTER(CARD16, CARD32)
DECLARE_CONVERTER(CARD32, CARD32)

static gray2image_converter_t converters[4][4] =
{
    {ConverterFrom_CARD8_To_CARD8,  ConverterFrom_CARD16_To_CARD8,  NULL, ConverterFrom_CARD32_To_CARD8},
    {ConverterFrom_CARD8_To_CARD16, ConverterFrom_CARD16_To_CARD16, NULL, ConverterFrom_CARD32_To_CARD16},
    {NULL,                          NULL,                           NULL, NULL},
    {ConverterFrom_CARD8_To_CARD32, ConverterFrom_CARD16_To_CARD32, NULL, ConverterFrom_CARD32_To_CARD32}
};

int GetGray2ImageConverter(unsigned int bytes_per_gray,
                           unsigned int bits_per_gray,
                           XImage *img,
                           gray2image_converter_t *cvtfunc, void **info,
                           int slow_but_debug)
{
  cvt_rec_t    *rec;
  unsigned int  bytes_per_image_pixel;

    bytes_per_image_pixel = BytesPerPixelOfImage(img);
    
    /* Check validity of parameters */
    if (
        (bytes_per_gray != 1  &&  bytes_per_gray != 2  &&  bytes_per_gray != 4)
            ||
        (bits_per_gray > 16)
            ||
        (bytes_per_gray * 8 < bits_per_gray)
            ||
        (bytes_per_image_pixel != 1  &&  bytes_per_image_pixel != 2  &&  bytes_per_image_pixel != 4)
       )
    {
        errno = EINVAL;
        return -1;
    }

    /* Obtain a conversion table */
    rec = FindRec(bytes_per_gray, bits_per_gray, img);
    if (rec == NULL) rec = CreateRec(bytes_per_gray, bits_per_gray, img);

    if (rec == NULL) return -1;

    /* Find an appropriate routine */
    *cvtfunc = converters[bytes_per_image_pixel-1][bytes_per_gray-1];

    *info = rec;

    return 0;
}
