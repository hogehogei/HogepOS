#pragma once

#include <stdint.h>

typedef enum 
{
    kPixelRGBReserved8BitPerColor,
    kPixelBGRReserved8BitPerColor,
    kUnknownFormat,
} PixelFormat;

typedef struct 
{
    uint8_t*    FrameBuffer;
    uint32_t    PixelsPerScanLine;
    uint32_t    HorizontalResolution;
    uint32_t    VerticalResolution;
    PixelFormat PixelFormat;
} FrameBufferConfig;
