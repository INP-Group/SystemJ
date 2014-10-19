#ifndef __ENDIANCONV_H
#define __ENDIANCONV_H


#include "misc_types.h"
#include "cx_sysdeps.h"


static inline uint32  swab32(uint32 ui32)
{
    return
        ((ui32 << 24) & 0xFF000000) |
        ((ui32 << 8 ) & 0x00FF0000) |
        ((ui32 >> 8 ) & 0x0000FF00) |
        ((ui32 >> 24) & 0x000000FF);
}

static inline uint16  swab16(uint16 ui16)
{
    return
        ((ui16 << 8) & 0xFF00) |
        ((ui16 >> 8) & 0x00FF);
}

typedef float  flt32;
typedef double flt64;

#if BYTE_ORDER == LITTLE_ENDIAN

static inline uint16  l2h_i16(uint16 lite_int16) {return lite_int16;}
static inline uint32  l2h_i32(uint32 lite_int32) {return lite_int32;}
static inline uint64  l2h_i64(uint64 lite_int64) {return lite_int64;}
static inline flt32   l2h_f32(flt32  lite_flt32) {return lite_flt32;}
static inline flt64   l2h_f64(flt64  lite_flt64) {return lite_flt64;}

static inline uint16  h2l_i16(uint16 host_int16) {return host_int16;}
static inline uint32  h2l_i32(uint32 host_int32) {return host_int32;}
static inline uint64  h2l_i64(uint32 host_int64) {return host_int64;}
static inline flt32   h2l_f32(flt32  host_flt32) {return host_flt32;}
static inline flt64   h2l_f64(flt64  host_flt64) {return host_flt64;}

static inline uint16  b2h_i16(uint16 bige_int16) {return bige_int16;}
static inline uint32  b2h_i32(uint32 bige_int32) {return bige_int32;}
static inline uint64  b2h_i64(uint64 bige_int64) {return bige_int64;}
static inline flt32   b2h_f32(flt32  bige_flt32) {return bige_flt32;}
static inline flt64   b2h_f64(flt64  bige_flt64) {return bige_flt64;}

static inline uint16  h2b_i16(uint16 host_int16) {return host_int16;}
static inline uint32  h2b_i32(uint32 host_int32) {return host_int32;}
static inline uint64  h2b_i64(uint32 host_int64) {return host_int64;}
static inline flt32   h2b_f32(flt32  host_flt32) {return host_flt32;}
static inline flt64   h2b_f64(flt64  host_flt64) {return host_flt64;}

#elif BYTE_ORDER == BIG_ENDIAN

#else

#error Sorry, the "BYTE_ORDER" is neither "LITTLE_ENDIAN" nor "BIG_ENDIAN"

#endif


#endif /* __ENDIANCONV_H */
