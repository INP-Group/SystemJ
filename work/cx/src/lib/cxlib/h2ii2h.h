/*********************************************************************
*                                                                    *
*  h2ii2h.h                                                          *
*      Host-to-intel and intel-to-host conversions                   *
*                                                                    *
*      In case of LITTLE_ENDIAN these functions do nothing           *
*      (except memcpy_x2y(), which just call memcpy()).              *
*                                                                    *
*      h2i -- host to intel                                          *
*      i2h -- intel to host                                          *
*                                                                    *
*********************************************************************/

#ifndef __H2II2H_H
#define __H2II2H_H


#if BYTE_ORDER == LITTLE_ENDIAN

static inline uint32 h2i32   (uint32 hostint32)   {return hostint32;}
static inline uint32 h2i16   (uint16 hostint16)   {return hostint16;}
static inline uint32 i2h32   (uint32 intelint32)  {return intelint32;}
static inline uint16 i2h16   (uint16 intelint16)  {return intelint16;}
static inline int32  h2i32s  (int32  hostint32)   {return hostint32;}
static inline int32  h2i16s  (int16  hostint16)   {return hostint16;}
static inline int32  i2h32s  (int32  intelint32)  {return intelint32;}
static inline int16  i2h16s  (int16  intelint16)  {return intelint16;}
static inline void   h2ihdr  (CxHeader   *h __attribute__((unused))) {}
static inline void   i2hhdr  (CxHeader   *h __attribute__((unused))) {}
static inline void   h2ifork (CxDataFork *f __attribute__((unused))) {}
static inline void   i2hfork (CxDataFork *f __attribute__((unused))) {}

static inline void memcpy_h2i32(uint32 *dst, const uint32 *src, int count)
{ memcpy(dst, src, sizeof(int32) * count); }

static inline void memcpy_i2h32(uint32 *dst, const uint32 *src, int count)
{ memcpy(dst, src, sizeof(int32) * count); }

static inline void memcpy_h2i16(uint16 *dst, const uint16 *src, int count)
{ memcpy(dst, src, sizeof(int16) * count); }

static inline void memcpy_i2h16(uint16 *dst, const uint16 *src, int count)
{ memcpy(dst, src, sizeof(int16) * count); }

#elif BYTE_ORDER == BIG_ENDIAN

#define SwapBytes32(x) (			\
            (((x & 0x000000FF) << 24)) |	\
            (((x & 0x0000FF00) <<  8)) |	\
            (((x & 0x00FF0000) >>  8)) |	\
            (((x & 0xFF000000) >> 24) & 0xFF)	\
        )

#define SwapBytes16(x) (			\
            (((x & 0x00FF) << 8)) |		\
            (((x & 0xFF00) >> 8) & 0xFF)	\
        )

static inline uint32 h2i32(uint32 hostint32)  {return SwapBytes32(hostint32);}
static inline uint32 h2i16(uint16 hostint16)  {return SwapBytes16(hostint16);}
static inline uint32 i2h32(uint32 intelint32) {return SwapBytes32(intelint32);}
static inline uint16 i2h16(uint16 intelint16) {return SwapBytes16(intelint16);}
static inline int32  h2i32s(int32 hostint32)  {return SwapBytes32(hostint32);}
static inline int32  h2i16s(int16 hostint16)  {return SwapBytes16(hostint16);}
static inline int32  i2h32s(int32 intelint32) {return SwapBytes32(intelint32);}
static inline int16  i2h16s(int16 intelint16) {return SwapBytes16(intelint16);}

static void h2ihdr(CxHeader *h)
{
    h->ID       = h2i32(h->ID);
    h->Seq      = h2i32(h->Seq);
    h->Type     = h2i32(h->Type);
    h->Res1     = h2i32(h->Res1);
    h->Res2     = h2i32(h->Res2);
    h->Status   = h2i32(h->Status);
    h->NumData  = h2i32(h->NumData);
    h->DataSize = h2i32(h->DataSize);
}

static void i2hhdr(CxHeader *h)
{
    h->ID       = i2h32(h->ID);
    h->Seq      = i2h32(h->Seq);
    h->Type     = i2h32(h->Type);
    h->Res1     = i2h32(h->Res1);
    h->Res2     = i2h32(h->Res2);
    h->Status   = i2h32(h->Status);
    h->NumData  = i2h32(h->NumData);
    h->DataSize = i2h32(h->DataSize);
}

static inline void h2ifork(CxDataFork *f)
{
    f->Command  = h2i32(f->Command);
    f->Flags    = h2i32(f->Flags);
    f->Status   = h2i32(f->Status);
    f->Count    = h2i32(f->Count);
    f->Start    = h2i32(f->Start);
    f->Reserved = h2i32(f->Reserved);
}

static inline void i2hfork(CxDataFork *f)
{
    f->Command  = i2h32(f->Command);
    f->Flags    = i2h32(f->Flags);
    f->Status   = i2h32(f->Status);
    f->Count    = i2h32(f->Count);
    f->Start    = i2h32(f->Start);
    f->Reserved = i2h32(f->Reserved);
}

static inline void memcpy_h2i32(uint32 *dst, const uint32 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = h2i32(*src++);
}

static inline void memcpy_i2h32(uint32 *dst, const uint32 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = i2h32(*src++);
}

static inline void memcpy_h2i16(uint16 *dst, const uint16 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = h2i16(*src++);
}

static inline void memcpy_i2h16(uint16 *dst, const uint16 *src, int count)
{
  int  x;
    
    for (x = 0; x < count; x++)  *dst++ = i2h16(*src++);
}

#else

#error Sorry, the "BYTE_ORDER" is neither "LITTLE_ENDIAN" nor "BIG_ENDIAN"

#endif


#endif /* __H2II2H_H */
