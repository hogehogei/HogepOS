#pragma once

#include <cstdint>
#include "RingBuffer.hpp"
#include "PixelWriter.hpp"
#include "Window.hpp"

class Console
{
public:

    static constexpr int sk_Rows    = 25;
    static constexpr int sk_Columns = 80;
    
    Console( const PixelColor& fg_color, const PixelColor& bg_color );
    ~Console() = default;

    void SetWindow( std::shared_ptr<Window> window );
    void SetLayerID( unsigned int layer_id );
    unsigned int LayerID() const;
    void PutString( const char* s );

private:

    static constexpr int sk_ColumnBufferLen = sk_Columns + 1;

    char* CurrentLine();
    void NewLine();
    void ClearConsole();
    void Refresh();

    std::shared_ptr<Window> m_Window;
    unsigned int m_LayerID;
    const PixelColor m_ForeGroundColor;
    const PixelColor m_BackGroundColor;

    int m_CursorRow;
    int m_CursorColumn;
    char m_Buffer[sk_Rows][sk_ColumnBufferLen];
    RingBuffer<char*, sk_Rows> m_ConsoleLines;
};
