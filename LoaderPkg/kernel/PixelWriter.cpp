#include "PixelWriter.hpp"

static uint8_t* PixelAt( const FrameBufferConfig& config, uint32_t x, uint32_t y )
{
    return config.FrameBuffer + (4 * (config.PixelsPerScanLine * y + x));
}

RGB8BitPerColorPixelWriter::RGB8BitPerColorPixelWriter( const FrameBufferConfig& config )
    : m_Config( config )
{}

void RGB8BitPerColorPixelWriter::Write( uint32_t x, uint32_t y, const PixelColor& c )
{
    if( x < m_Config.HorizontalResolution && y < m_Config.VerticalResolution ){
        uint8_t* p = PixelAt( m_Config, x, y );
        p[0] = c.Red;
        p[1] = c.Green;
        p[2] = c.Blue;
    }
}



BGR8BitPerColorPixelWriter::BGR8BitPerColorPixelWriter( const FrameBufferConfig& config )
    : m_Config( config )
{}

void BGR8BitPerColorPixelWriter::Write( uint32_t x, uint32_t y, const PixelColor& c )
{
    if( x < m_Config.HorizontalResolution && y < m_Config.VerticalResolution ){
        uint8_t* p = PixelAt( m_Config, x, y );
        p[0] = c.Blue;
        p[1] = c.Green;
        p[2] = c.Red;
    }
}