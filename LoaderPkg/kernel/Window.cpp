//
// include files
//
#include "Window.hpp"
#include "Font.hpp"
#include "logger.hpp"
#include "PixelWriter.hpp"

//
// constant
//

//
// static variables
//
namespace
{
    void DrawTextBox(IPixelWriter& writer, Vector2<int> pos, Vector2<int> size,
                   const PixelColor& background,
                   const PixelColor& border_light,
                   const PixelColor& border_dark) {
        auto fill_rect =
        [&writer](Vector2<int> pos, Vector2<int> size, const PixelColor& c) {
            FillRectAngle(writer, pos, size, c);
        };

        // fill main box
        fill_rect(pos + Vector2<int>{1, 1}, size - Vector2<int>{2, 2}, background);

        // draw border lines
        fill_rect(pos,                            {size.x, 1}, border_dark);
        fill_rect(pos,                            {1, size.y}, border_dark);
        fill_rect(pos + Vector2<int>{0, size.y}, {size.x, 1}, border_light);
        fill_rect(pos + Vector2<int>{size.x, 0}, {1, size.y}, border_light);
    }

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
}

//
// static function declaration
//

//
// funcion definitions
//

Window::Window(int width, int height, PixelFormat shadow_format)
    : m_Width(width),
      m_Height(height),
      m_Data(),
      m_Writer(*this),
      m_TransparentColor(),
      m_ShadowBuffer()
{
    m_Data.resize(m_Height);

    for( int y = 0; y < m_Height; ++y ){
        m_Data[y].resize(m_Width);
    }

    FrameBufferConfig config;
    config.FrameBuffer = nullptr;
    config.HorizontalResolution = width;
    config.VerticalResolution = height;
    config.PixelFormat = shadow_format;

    auto err = m_ShadowBuffer.Initialize(config);
    if( err ){
        Log(kError, "failed to initialize shadow buffer : %s at %s:%d\n",
            err.Name(), err.File(), err.Line());
    }
}

void Window::DrawTo(FrameBuffer &dst, Vector2<int> pos, const RectAngle<int> &area)
{
    if (!m_TransparentColor)
    {
        RectAngle<int> window_area{pos, Size()};
        RectAngle<int> intersection = area.Intersection(window_area);
        // Copy の第3引数にはバッファの左上座標を基準とする座標を渡す必要がある
        // pos は バッファの左上座標を基準とするウィンドウ位置が入っているので
        // intersecton - pos とすれば、交差した矩形の左上がバッファの左上座標からどこにあるか
        // 計算できることになる
        dst.Copy(intersection.pos, m_ShadowBuffer, {intersection.pos - pos, intersection.size});
    }
    else
    {
        const auto tc = m_TransparentColor.value();
        auto &dst_w = dst.Writer();
        for (int y = std::max(0, 0 - pos.y); 
             y < std::min(Height(), dst_w.Height() - pos.y); ++y)
        {
            for (int x = std::max(0, 0 - pos.x); 
                 x < std::min(Width(), dst_w.Width() - pos.x); ++x)
            {
                const auto c = At(x, y);
                if (tc != c)
                {
                    dst_w.Write(pos.x + x, pos.y + y, c);
                }
            }
        }
    }
}

void Window::Move( Vector2<int> dst_pos, const RectAngle<int>& src )
{
    m_ShadowBuffer.Move( dst_pos, src );
}


void Window::SetTransparentColor(const std::optional<PixelColor> &c)
{
    m_TransparentColor = c;
}

Window::WindowWriter *Window::Writer()
{
    return &m_Writer;
}

PixelColor &Window::At(int x, int y)
{
    return m_Data[y][x];
}

const PixelColor Window::At(int x, int y) const
{
    return m_Data[y][x];
}

void Window::Write(Vector2<int> pos, PixelColor c)
{
    m_Data[pos.y][pos.x] = c;
    m_ShadowBuffer.Writer().Write(pos.x, pos.y, c);
}

void Window::Write(int x, int y, PixelColor c)
{
    m_Data[y][x] = c;
    m_ShadowBuffer.Writer().Write(x, y, c);
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
    return Vector2<int>{m_Width, m_Height};
}

TopLevelWindow::TopLevelWindow(int width, int height, PixelFormat shadow_format, const std::string &title)
    : Window(width, height, shadow_format),
      m_Title(title),
      m_InnerWriter(*this)
{
    DrawWindowTitle(*Writer(), m_Title.c_str(), false);
}

void TopLevelWindow::Activate()
{
    Window::Activate();
    DrawWindowTitle(*Writer(), m_Title.c_str(), true);
}

void TopLevelWindow::Deactivate()
{
    Window::Deactivate();
    DrawWindowTitle(*Writer(), m_Title.c_str(), false);
}

Vector2<int> TopLevelWindow::InnerSize() const
{
    return Size() - k_TopLeftMargin - k_BottomRightMargin;
}

void DrawWindow(IPixelWriter &writer, const char *title)
{
    auto fill_rect = [&writer](Vector2<int> pos, Vector2<int> size, uint32_t c)
    {
        FillRectAngle(writer, pos, size, ToColor(c));
    };
    const auto win_w = writer.Width();
    const auto win_h = writer.Height();

    fill_rect({0, 0}, {win_w, 1}, 0xc6c6c6);
    fill_rect({1, 1}, {win_w - 2, 1}, 0xffffff);
    fill_rect({0, 0}, {1, win_h}, 0xc6c6c6);
    fill_rect({1, 1}, {1, win_h - 2}, 0xffffff);
    fill_rect({win_w - 2, 1}, {1, win_h - 2}, 0x848484);
    fill_rect({win_w - 1, 0}, {1, win_h}, 0x000000);
    fill_rect({2, 2}, {win_w - 4, win_h - 4}, 0xc6c6c6);
    fill_rect({3, 3}, {win_w - 6, 18}, 0x000084);
    fill_rect({1, win_h - 2}, {win_w - 2, 1}, 0x848484);
    fill_rect({0, win_h - 1}, {win_w, 1}, 0x000000);

    DrawWindowTitle( writer, title, false );
}

void DrawWindowTitle( IPixelWriter &writer, const char *title, bool active )
{
    const auto win_w = writer.Width();
    uint32_t bgcolor = 0x848484;
    if( active ){
        bgcolor = 0x000084;
    }

    FillRectAngle(writer, {3, 3}, {win_w - 6, 18}, ToColor(bgcolor));
    WriteString(writer, 24, 4, title, ToColor(0xffffff));

    for( int y = 0; y < kCloseButtonHeight; ++y ){
        for( int x = 0; x < kCloseButtonWidth; ++x ){
            PixelColor c = ToColor(0xffffff);
            if( close_button[y][x] == '@' ){
                c = ToColor(0x000000);
            }
            else if( close_button[y][x] == '$' ){
                c = ToColor(0x848484);
            }
            else if( close_button[y][x] == ':' ){
                c = ToColor(0xc6c6c6);
            }
            writer.Write(win_w - 5 - kCloseButtonWidth + x, 5 + y, c);
        }
    }
}

void DrawTextBox( IPixelWriter& writer, Vector2<int> pos, Vector2<int> size ) 
{
    DrawTextBox(writer, pos, size,
                ToColor(0xffffff), ToColor(0xc6c6c6), ToColor(0x848484));
}

void DrawTerminal( IPixelWriter& writer, Vector2<int> pos, Vector2<int> size ) 
{
    DrawTextBox(writer, pos, size,
                ToColor(0x000000), ToColor(0xc6c6c6), ToColor(0x848484));
}