//
// include files
//
#include "Window.hpp"


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

Window::Window( int width, int height )
    : m_Width(width),
      m_Height(height),
      m_Data(),
      m_Writer(*this),
      m_TransparentColor()
{
    m_Data.resize( m_Height );

    for( int y = 0; y < m_Height; ++y ){
        m_Data[y].resize( m_Width );
    }
}

void Window::DrawTo( IPixelWriter& writer, Vector2<int> pos )
{
    if( !m_TransparentColor ){
        for( int y = 0; y < m_Height; ++y ){
            for( int x = 0; x < m_Width; ++x ){
                writer.Write( pos.x + x, pos.y + y, At(x, y) );
            }
        }
    }
    else {
        const auto tc = m_TransparentColor.value();
        for( int y = 0; y < m_Height; ++y ){
            for( int x = 0; x < m_Width; ++x ){
                const auto c = At(x, y);
                if( tc != c ){
                    writer.Write( pos.x + x, pos.y + y, At(x, y) );
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

int Window::Width() const
{
    return m_Width;
}

int Window::Height() const
{
    return m_Height;
}

