//
// include files
//
#include "Window.hpp"
#include "logger.hpp"

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

Window::Window( int width, int height, PixelFormat shadow_format )
    : m_Width(width),
      m_Height(height),
      m_Data(),
      m_Writer(*this),
      m_TransparentColor(),
      m_ShadowBuffer()
{
    m_Data.resize( m_Height );

    for( int y = 0; y < m_Height; ++y ){
        m_Data[y].resize( m_Width );
    }

    FrameBufferConfig config;
    config.FrameBuffer = nullptr;
    config.HorizontalResolution = width;
    config.VerticalResolution = height;
    config.PixelFormat = shadow_format;

    auto err = m_ShadowBuffer.Initialize(config);
    if( err ){
        Log( kError, "failed to initialize shadow buffer : %s at %s:%d\n",
            err.Name(), err.File(), err.Line() );
    }
}

void Window::DrawTo( FrameBuffer& dst, Vector2<int> pos )
{
    if( !m_TransparentColor ){
        dst.Copy( pos, m_ShadowBuffer );
    }
    else {
        const auto tc = m_TransparentColor.value();
        auto& writer = dst.Writer();
        for( int y = 0; y < m_Height; ++y ){
            for( int x = 0; x < m_Width; ++x ){
                const auto c = At(x, y);
                if( tc != c ){
                    writer.Write( pos.x + x, pos.y + y, c );
                }
            }
        }
    }
}    

void Window::SetTransparentColor( const std::optional<PixelColor>& c )
{
    m_TransparentColor = c;
}

Window::WindowWriter* Window::Writer()
{
    return &m_Writer;
}

PixelColor& Window::At( int x, int y )
{
    return m_Data[y][x];
}

const PixelColor Window::At( int x, int y ) const
{
    return m_Data[y][x];
}

void Window::Write( Vector2<int> pos, PixelColor c )
{
    m_Data[pos.y][pos.x] = c;
    m_ShadowBuffer.Writer().Write( pos.x, pos.y, c );
}

void Window::Write( int x, int y, PixelColor c )
{
    m_Data[y][x] = c;
    m_ShadowBuffer.Writer().Write( x, y, c );
}


int Window::Width() const
{
    return m_Width;
}

int Window::Height() const
{
    return m_Height;
}

