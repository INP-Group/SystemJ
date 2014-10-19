#ifndef __GRAY2IMAGE_H
#define __GRAY2IMAGE_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <X11/Xlib.h>

typedef int (*gray2image_converter_t)(void *data, XImage *image, void *info);

int GetGray2ImageConverter(unsigned int bytes_per_gray,
                           unsigned int bits_per_gray,
                           XImage *img,
                           gray2image_converter_t *cvtfunc, void **info,
                           int slow_but_debug);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GRAY2IMAGE_H */
