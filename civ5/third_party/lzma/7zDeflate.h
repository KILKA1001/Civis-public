#ifndef CIVIS_7Z_DEFLATE_H
#define CIVIS_7Z_DEFLATE_H

#include "7zTypes.h"

SRes CivisDecode7zDeflate(UInt64 inSize, ILookInStreamPtr inStream,
                         Byte *outBuffer, size_t outSize, ISzAllocPtr alloc);

#endif
