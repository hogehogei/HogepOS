//
// include files
//
#include "Window.hpp"
#include "Font.hpp"
#include "logger.hpp"

//
// constant
//

//
// static variables
//
namespace {
    const int kCloseButtonWidth = 16;
    const int kCloseButtonHeight = 14;
    const char close_button[kCloseButtonHeight][kCloseButtonWidth + 1] = {
        "...............@",
        ".:::::::::::::$@",
        ".:::::::::::::$@",
        ".:::@@::::@@::$@",
        ".::::@@::@@:::$@",
        ".:::::@@@@::::$@",
        ".::::::@@:::::$@",
        ".:::::@@@@::::$@",
        ".::::@@::@@:::$@",
        ".:::@@::::@@::$@",
        ".:::::::::::::$@",
        ".:::::::::::::$@",
        ".$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@",
    };

    constexpr PixelColor ToColor(uint32_t c) {
        return {
        static_cast<uint8_t>((c >> 16) & 0xff),
        static_cast<uint8_t>((c >> 8) & 0xff),
        static_cast<uint8_t>(c & 0xff)
        };
    }
}


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

void Window::DrawTo( FrameBuffer& dst, Vector2<int> pos, const RectAngle<int>& area )
{
    if( !m_TransparentColor ){
        RectAngle<int> window_area{ pos, Size() };
        RectAngle<int> intersection = area.Intersection( window_area );
        // Copy の第3引数にはバッファの左上座標を基準とする座標を渡す必要がある
        // pos は バッファの左上座標を基準とするウィンドウ位置が入っているので
        // intersecton - pos とすれば、交差した矩形の左上がバッファの左上座標からどこにあるか
        // 計算できることになる
        dst.Copy( intersection.pos, m_ShadowBuffer, {intersection.pos - pos, intersection.size} );
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

Vector2<int> Window::Size() const
{
    return Vector2<int>( m_Width, m_Height );
}

void DrawWindow( IPixelWriter& writer, const char* title ) {
    auto fill_rect = [&writer](Vector2<int> pos, Vector2<int> size, uint32_t c) {
        FillRectAngle(writer, pos, size, ToColor(c));
    };
    const auto win_w = writer.Width();
    const auto win_h = writer.Height();

    fill_rect({0, 0},         {win_w, 1},             0xc6c6c6);
    fill_rect({1, 1},         {win_w - 2, 1},         0xffffff);
    fill_rect({0, 0},         {1, win_h},             0xc6c6c6);
    fill_rect({1, 1},         {1, win_h - 2},         0xffffff);
    fill_rect({win_w - 2, 1}, {1, win_h - 2},         0x848484);
    fill_rect({win_w - 1, 0}, {1, win_h},             0x000000);
    fill_rect({2, 2},         {win_w - 4, win_h - 4}, 0xc6c6c6);
    fill_rect({3, 3},         {win_w - 6, 18},        0x000084);
    fill_rect({1, win_h - 2}, {win_w - 2, 1},         0x848484);
    fill_rect({0, win_h - 1}, {win_w, 1},             0x000000);

    WriteString(writer, 24, 4, title, ToColor(0xffffff));

    for (int y = 0; y < kCloseButtonHeight; ++y) {
        for (int x = 0; x < kCloseButtonWidth; ++x) {
            PixelColor c = ToColor(0xffffff);
            if (close_button[y][x] == '@') {
                c = ToColor(0x000000);
            } else if (close_button[y][x] == '$') {
                c = ToColor(0x848484);
            } else if (close_button[y][x] == ':') {
                c = ToColor(0xc6c6c6);
            }
            writer.Write( win_w - 5 - kCloseButtonWidth + x, 5 + y, c);
        }
    }
}