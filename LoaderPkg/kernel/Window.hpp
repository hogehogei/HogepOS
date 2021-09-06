#pragma once

#include <optional>
#include <vector>

#include "PixelWriter.hpp"
#include "Graphic.hpp"

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
            m_Window.At( x, y ) = c;
        }
        virtual int Width() const override { return m_Window.Width(); }
        virtual int Height() const override { return m_Window.Height(); }

    private:

        Window& m_Window;
    };

    //! @brief 指定されたピクセル数の平面描写領域を作成する
    Window( int width, int height );
    ~Window() = default;

    Window( const Window& ) = delete;
    Window& operator=( const Window& ) = delete;

    /**
     * @brief 与えられた PixelWriter にこのウィンドウの表示領域を描写する
     * @param writer 描写先
     * @param pos    writer の左上を基準とした描写位置
     */
    void DrawTo( IPixelWriter& writer, Vector2<int> pos );

    //! @brief 透過色を設定する
    void SetTransparentColor( const std::optional<PixelColor>& c );
    //! @brief このインスタンスに紐づいた WindowWriter を取得する。
    Window::WindowWriter* Writer();

    //! @brief 指定した位置のピクセルを返す
    PixelColor& At( int x, int y );
    //! @brief 指定した位置のピクセルを返す
    const PixelColor At( int x, int y ) const;

    //! @brief 平面描写領域の横幅をピクセル単位で返す
    int Width() const;
    //! @brief 平面描写領域の高さをピクセル単位で返す
    int Height() const;

private:

    int m_Width, m_Height;
    std::vector<std::vector<PixelColor>> m_Data;
    WindowWriter m_Writer;
    std::optional<PixelColor> m_TransparentColor;
};