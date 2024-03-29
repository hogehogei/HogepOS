#pragma once

#include <cstdint>
#include "PixelFormat.h"

struct PixelColor
{
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;

    bool operator==( const PixelColor& rhs ) const {
        return Red == rhs.Red && Green == rhs.Green && Blue == rhs.Blue;
    }
    bool operator!=( const PixelColor& rhs ) const {
        return !(*this == rhs);
    }
};

class IPixelWriter
{
public:

    virtual ~IPixelWriter() = default;
    virtual void Write( uint32_t x, uint32_t y, const PixelColor& c ) = 0;
    virtual int Width() const = 0;
    virtual int Height() const = 0;

private:
};

class FrameBufferPixelWriter : public IPixelWriter
{
public:
    FrameBufferPixelWriter( const FrameBufferConfig& config ) : 
        m_Config( config ) {}

    virtual int Width() const { return m_Config.HorizontalResolution; }
    virtual int Height() const { return m_Config.VerticalResolution; }

protected:

    FrameBufferConfig m_Config;
};

class RGB8BitPerColorPixelWriter : public FrameBufferPixelWriter
{
public:
    RGB8BitPerColorPixelWriter( const FrameBufferConfig& config );
    ~RGB8BitPerColorPixelWriter() = default;

    virtual void Write( uint32_t x, uint32_t y, const PixelColor& c ) override;
};

class BGR8BitPerColorPixelWriter : public FrameBufferPixelWriter
{
public:
    BGR8BitPerColorPixelWriter( const FrameBufferConfig& config );
    ~BGR8BitPerColorPixelWriter() = default;

    virtual void Write( uint32_t x, uint32_t y, const PixelColor& c ) override;
};

constexpr PixelColor ToColor(uint32_t c)
{
    return {
        static_cast<uint8_t>((c >> 16) & 0xff),
        static_cast<uint8_t>((c >> 8) & 0xff),
        static_cast<uint8_t>(c & 0xff)};
}
