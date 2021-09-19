#pragma once

#include <optional>
#include <vector>

#include "PixelWriter.hpp"
#include "Graphic.hpp"
#include "FrameBuffer.hpp"

class Window
{
public:
    /** 
     * @brief WindowWriter は Window と関連付けられた PixelWriter を提供する
     */
    class WindowWriter : public IPixelWriter
    {
    public:
        WindowWriter( Window& window ) : m_Window(window) {}

        //! @brief 指定位置に色を描く
        virtual void Write( uint32_t x, uint32_t y, const PixelColor& c ) override 
        {
            m_Window.Write( x, y, c );
        }
        virtual int Width() const override { return m_Window.Width(); }
        virtual int Height() const override { return m_Window.Height(); }

    private:

        Window& m_Window;
    };

    //! @brief 指定されたピクセル数の平面描写領域を作成する
    Window( int width, int height, PixelFormat shadow_format );
    ~Window() = default;

    Window( const Window& ) = delete;
    Window& operator=( const Window& ) = delete;

    /**
     * @brief 与えられた FrameBuffer にこのウィンドウの表示領域を描写する
     * @param dst    描写先
     * @param pos    dst の左上を基準とした描写位置
     */
    void DrawTo( FrameBuffer& dst, Vector2<int> pos, const RectAngle<int>& area );

    //! @brief 透過色を設定する
    void SetTransparentColor( const std::optional<PixelColor>& c );
    //! @brief このインスタンスに紐づいた WindowWriter を取得する。
    Window::WindowWriter* Writer();

    //! @brief 指定した位置のピクセルを返す
    PixelColor& At( int x, int y );
    //! @brief 指定した位置のピクセルを返す
    const PixelColor At( int x, int y ) const;

    //! @brief 指定した位置にピクセルを書き込む
    void Write( Vector2<int> pos, PixelColor c );
    void Write( int x, int y, PixelColor c );

    //! @brief 平面描写領域の横幅をピクセル単位で返す
    int Width() const;
    //! @brief 平面描写領域の高さをピクセル単位で返す
    int Height() const;
    //! @brief 平面描写領域のサイズを返す
    Vector2<int> Size() const;

private:

    int m_Width, m_Height;
    std::vector<std::vector<PixelColor>> m_Data;
    WindowWriter m_Writer;
    std::optional<PixelColor> m_TransparentColor;

    FrameBuffer m_ShadowBuffer;
};

void DrawWindow( IPixelWriter& writer, const char* title );