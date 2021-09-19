//
// include files
//
#include "FrameBuffer.hpp"


//
// constant
//

//
// static variables
//

//
// static function declaration
// 
namespace {
    int BitsPerPixel( PixelFormat format );
    int BytesPerPixel( PixelFormat format );
    uint8_t* FrameAddrAt( Vector2<int> pos, const FrameBufferConfig& config );
    int BytesPerScanLine( const FrameBufferConfig& config );
    Vector2<int> FrameBufferSize( const FrameBufferConfig& config );
}
//
// funcion definitions
// 

namespace {
    
    int BitsPerPixel( PixelFormat format )
    {
        switch( format ){
        case kPixelRGBReserved8BitPerColor:
            return 32;
        case kPixelBGRReserved8BitPerColor:
            return 32;
        default:
            return -1;
        }
        return -1;
    }
    
    int BytesPerPixel( PixelFormat format )
    {
        switch (format) {
        case kPixelRGBReserved8BitPerColor: 
            return 4;
        case kPixelBGRReserved8BitPerColor: 
            return 4;
        default: 
            return -1;
        }
        return -1;
    }

    uint8_t* FrameAddrAt( Vector2<int> pos, const FrameBufferConfig& config ) 
    {
        return config.FrameBuffer + BytesPerPixel(config.PixelFormat) *
        (config.PixelsPerScanLine * pos.y + pos.x);
    }

    int BytesPerScanLine( const FrameBufferConfig& config ) 
    {
        return BytesPerPixel(config.PixelFormat) * config.PixelsPerScanLine;
    }

    Vector2<int> FrameBufferSize( const FrameBufferConfig& config ) 
    {
        return {static_cast<int>(config.HorizontalResolution),
                static_cast<int>(config.VerticalResolution)};
    }
}

Error FrameBuffer::Initialize( const FrameBufferConfig& config )
{
    m_Config = config;

    const auto bytes_per_pixel = BytesPerPixel( config.PixelFormat );
    if( bytes_per_pixel <= 0 ){
        return MAKE_ERROR( Error::kUnknownPixelFormat );
    }

    if( config.FrameBuffer ){
        m_Buffer.resize(0);
    }
    else {
        m_Buffer.resize(
            bytes_per_pixel * m_Config.HorizontalResolution * m_Config.VerticalResolution
        );
        m_Config.FrameBuffer = m_Buffer.data();
        m_Config.PixelsPerScanLine = m_Config.HorizontalResolution;
    }

    switch( m_Config.PixelFormat ){
    case kPixelRGBReserved8BitPerColor:
        m_Writer = std::make_unique<RGB8BitPerColorPixelWriter>( m_Config );
        break;
    case kPixelBGRReserved8BitPerColor:
        m_Writer = std::make_unique<BGR8BitPerColorPixelWriter>( m_Config );
        break;
    default:
        return MAKE_ERROR( Error::kUnknownPixelFormat );
    }

    return MAKE_ERROR( Error::kSuccess );
}

Error FrameBuffer::Copy( Vector2<int> dst_pos, const FrameBuffer& src, const RectAngle<int>& src_area )
{
    if( m_Config.PixelFormat != src.m_Config.PixelFormat ){
        return MAKE_ERROR( Error::kUnknownPixelFormat );
    }

    const auto bytes_per_pixel = BytesPerPixel( m_Config.PixelFormat );
    if( bytes_per_pixel <= 0 ){
        return MAKE_ERROR( Error::kUnknownPixelFormat );
    }

    const RectAngle<int> dst_outline( {0, 0}, FrameBufferSize(m_Config) );
    const RectAngle<int> src_area_shifted( dst_pos, src_area.size );
    const RectAngle<int> src_outline( dst_pos - src_area.pos, FrameBufferSize(src.m_Config) );

    const auto copy_area = dst_outline.Intersection( src_outline.Intersection( src_area_shifted ) );
    const auto src_start_pos = ElementMin( src_area.pos, FrameBufferSize(src.m_Config) - Vector2<int>(1, 1) );
 
    uint8_t* dst_buf = FrameAddrAt( copy_area.pos, m_Config );
    const uint8_t* src_buf = FrameAddrAt( src_start_pos, src.m_Config );

    for( int y = 0; y < copy_area.size.y; ++y ){
        memcpy( dst_buf, src_buf, bytes_per_pixel * copy_area.size.x );
        dst_buf += BytesPerScanLine( m_Config );
        src_buf += BytesPerScanLine( src.m_Config );
    }

    return MAKE_ERROR( Error::kSuccess );
}

FrameBufferPixelWriter& FrameBuffer::Writer()
{
    return *m_Writer;
}

const FrameBufferConfig& FrameBuffer::Config() const
{
    return m_Config;
}
