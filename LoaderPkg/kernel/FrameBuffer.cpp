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

//
// funcion definitions
// 
Error FrameBuffer::Initialize( const FrameBufferConfig& config )
{
    m_Config = config;

    const auto bits_per_pixel = BitsPerPixel( config.PixelFormat );
    if( bits_per_pixel <= 0 ){
        return MAKE_ERROR( Error::kUnknownPixelFormat );
    }

    if( config.FrameBuffer ){
        m_Buffer.resize(0);
    }
    else {
        m_Buffer.resize(
            ((bits_per_pixel + 7) / 8) * m_Config.HorizontalResolution * m_Config.VerticalResolution
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

Error FrameBuffer::Copy( Vector2<int> pos, const FrameBuffer& src )
{
    if( m_Config.PixelFormat != src.m_Config.PixelFormat ){
        return MAKE_ERROR( Error::kUnknownPixelFormat );
    }

    const auto bits_per_pixel = BitsPerPixel( m_Config.PixelFormat );
    if( bits_per_pixel <= 0 ){
        return MAKE_ERROR( Error::kUnknownPixelFormat );
    }

    const auto dst_width = m_Config.HorizontalResolution;
    const auto dst_height = m_Config.VerticalResolution;
    const auto src_width =  src.m_Config.HorizontalResolution;
    const auto src_height = src.m_Config.VerticalResolution;

    const int copy_start_dst_x = std::max( pos.x, 0 );
    const int copy_start_dst_y = std::max( pos.y, 0 );
    const int copy_end_dst_x = std::min( pos.x + src_width, dst_width );
    const int copy_end_dst_y = std::min( pos.y + src_height, dst_height );

    if( copy_end_dst_x < 0 || copy_end_dst_y < 0 ||
        copy_start_dst_x >= dst_width || copy_start_dst_y >= dst_height )
    {
        return MAKE_ERROR( Error::kInvalidArguments );
    }

    const auto bytes_per_pixel = (bits_per_pixel + 7) / 8;
    const auto bytes_per_copy_line = bytes_per_pixel * (copy_end_dst_x - copy_start_dst_x);

    uint8_t* dst_buf = m_Config.FrameBuffer + 
                       bytes_per_pixel * m_Config.PixelsPerScanLine + copy_start_dst_x;

    const uint8_t* src_buf = src.m_Config.FrameBuffer + 
                             (bytes_per_pixel * src.m_Config.PixelsPerScanLine) - 
                             bytes_per_copy_line;

    for( int dy = 0; dy < copy_end_dst_y - copy_start_dst_y; ++dy ){
        memcpy( dst_buf, src_buf, bytes_per_copy_line );
        dst_buf += bytes_per_pixel * m_Config.PixelsPerScanLine;
        src_buf += bytes_per_pixel * src.m_Config.PixelsPerScanLine;
    }

    return MAKE_ERROR( Error::kSuccess );
}

FrameBufferPixelWriter& FrameBuffer::Writer()
{
    return *m_Writer;
}

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