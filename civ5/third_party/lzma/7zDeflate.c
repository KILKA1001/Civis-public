#include "7zDeflate.h"
#include "zlib.h"
#include <string.h>

static voidpf alloc_zlib(voidpf opaque, uInt count, uInt size)
{
  const size_t bytes = (size_t)count * size;
  if (count && bytes / count != size)
    return Z_NULL;
  return ISzAlloc_Alloc((ISzAllocPtr)opaque, bytes);
}

static void free_zlib(voidpf opaque, voidpf address)
{
  ISzAlloc_Free((ISzAllocPtr)opaque, address);
}

SRes CivisDecode7zDeflate(UInt64 inSize, ILookInStreamPtr inStream,
                         Byte *outBuffer, size_t outSize, ISzAllocPtr alloc)
{
  z_stream stream;
  size_t written = 0;
  SRes result = SZ_ERROR_DATA;
  Byte overflow = 0;
  int status;
  memset(&stream, 0, sizeof(stream));
  stream.zalloc = alloc_zlib;
  stream.zfree = free_zlib;
  stream.opaque = (voidpf)alloc;
  status = inflateInit2(&stream, -MAX_WBITS);
  if (status != Z_OK)
    return status == Z_MEM_ERROR ? SZ_ERROR_MEM : SZ_ERROR_DATA;

  for (;;)
  {
    const void *input = NULL;
    const size_t remaining = outSize - written;
    size_t available = inSize < (1 << 20) ? (size_t)inSize : (1 << 20);
    uInt input_count;
    uInt output_count;
    uInt consumed;
    uInt produced;
    SRes read_status;

    if (available != 0)
    {
      read_status = ILookInStream_Look(inStream, &input, &available);
      if (read_status != SZ_OK)
      {
        result = read_status;
        break;
      }
      if (available == 0)
        break;
    }
    input_count = (uInt)available;
    output_count = (uInt)(remaining < (1 << 20) ? remaining : (1 << 20));
    stream.next_in = (Bytef *)input;
    stream.avail_in = input_count;
    stream.next_out = output_count ? outBuffer + written : &overflow;
    stream.avail_out = output_count ? output_count : 1;
    status = inflate(&stream, Z_NO_FLUSH);
    consumed = input_count - stream.avail_in;
    produced = (output_count ? output_count : 1) - stream.avail_out;
    if (consumed != 0)
    {
      read_status = ILookInStream_Skip(inStream, consumed);
      if (read_status != SZ_OK)
      {
        result = read_status;
        break;
      }
      inSize -= consumed;
    }
    if (!output_count && produced)
      break;
    written += produced;
    if (status == Z_STREAM_END)
    {
      if (inSize == 0 && written == outSize)
        result = SZ_OK;
      break;
    }
    if (status == Z_MEM_ERROR)
    {
      result = SZ_ERROR_MEM;
      break;
    }
    if (status != Z_OK || (consumed == 0 && produced == 0))
      break;
  }
  inflateEnd(&stream);
  return result;
}
