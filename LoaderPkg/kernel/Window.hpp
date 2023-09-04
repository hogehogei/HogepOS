#pragma once

#include <optional>
#include <vector>
#include <string>

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
    virtual ~Window() = default;

    Window( const Window& ) = delete;
    Window& operator=( const Window& ) = delete;

    /**
     * @brief 与えられた FrameBuffer にこのウィンドウの表示領域を描写する
     * @param dst    描写先
     * @param pos    dst の左上を基準とした描写位置
     */
    void DrawTo( FrameBuffer& dst, Vector2<int> pos, const RectAngle<int>& area );

    /** @brief このウィンドウの平面描画領域内で，矩形領域を移動する。
     *
     * @param src_pos   移動元矩形の原点
     * @param src_size  移動元矩形の大きさ
     * @param dst_pos   移動先の原点
     */
    void Move( Vector2<int> dst_pos, const RectAngle<int>& src );

    //! @brief 透過色を設定する
    void SetTransparentColor( const std::optional<PixelColor>& c );
    //! @brief このインスタンスに紐づいた WindowWriter を取得する。
    virtual Window::WindowWriter* Writer();

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

    virtual void Activate() {}
    virtual void Deactivate() {}

private:

    int m_Width, m_Height;
    std::vector<std::vector<PixelColor>> m_Data;
    WindowWriter m_Writer;
    std::optional<PixelColor> m_TransparentColor;

    FrameBuffer m_ShadowBuffer;
};

class TopLevelWindow : public Window
{
public:
    static constexpr Vector2<int> k_TopLeftMargin{ 4, 24 };
    static constexpr Vector2<int> k_BottomRightMargin{ 4, 4 };
    static constexpr int k_MarginX = k_TopLeftMargin.x + k_BottomRightMargin.x;
    static constexpr int k_MarginY = k_TopLeftMargin.y + k_BottomRightMargin.y;

    class InnerAreaWriter : public IPixelWriter {
    public:
        InnerAreaWriter( TopLevelWindow& window ) : m_Window(window) {}
        virtual void Write( uint32_t x, uint32_t y, const PixelColor& c ) override {
            m_Window.Write( x + k_TopLeftMargin.x, y + k_TopLeftMargin.y, c );
        }
        virtual int Width() const override {
            return m_Window.Width() - k_TopLeftMargin.x - k_BottomRightMargin.x;
        }
        virtual int Height() const override {
            return m_Window.Height() - k_TopLeftMargin.y - k_BottomRightMargin.y;
        }
    private:
        TopLevelWindow& m_Window;
    };

    TopLevelWindow( int width, int height, PixelFormat shadow_format, const std::string& title );
    virtual void Activate() override;
    virtual void Deactivate() override;

    InnerAreaWriter* InnerWriter() { return &m_InnerWriter; }
    Vector2<int> InnerSize() const;

private:

    std::string m_Title;
    InnerAreaWriter m_InnerWriter;
};

void DrawWindow( IPixelWriter& writer, const char* title );
void DrawWindowTitle( IPixelWriter &writer, const char *title, bool active );
void DrawTextBox( IPixelWriter& writer, Vector2<int> pos, Vector2<int> size ) ;
void DrawTerminal( IPixelWriter& writer, Vector2<int> pos, Vector2<int> size ) ;