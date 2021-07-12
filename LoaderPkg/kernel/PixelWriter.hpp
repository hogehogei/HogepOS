#pragma once

#include <cstdint>
#include "Main.h"

struct PixelColor
{
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
};

class IPixelWriter
{
public:

    virtual ~IPixelWriter() = default;
    virtual void Write( uint32_t x, uint32_t y, const PixelColor& c ) = 0;

private:
};

class RGB8BitPerColorPixelWriter : public IPixelWriter
{
public:
    RGB8BitPerColorPixelWriter( const FrameBufferConfig& config );
    ~RGB8BitPerColorPixelWriter() = default;

    virtual void Write( uint32_t x, uint32_t y, const PixelColor& c ) override;

private:

    FrameBufferConfig m_Config;
};

class BGR8BitPerColorPixelWriter : public IPixelWriter
{
public:
    BGR8BitPerColorPixelWriter( const FrameBufferConfig& config );
    ~BGR8BitPerColorPixelWriter() = default;

    virtual void Write( uint32_t x, uint32_t y, const PixelColor& c ) override;

private:

    FrameBufferConfig m_Config;
};
