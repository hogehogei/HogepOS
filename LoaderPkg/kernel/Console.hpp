#pragma once

#include <cstdint>
#include "RingBuffer.hpp"
#include "PixelWriter.hpp"

class Console
{
public:

    static constexpr int sk_Rows    = 25;
    static constexpr int sk_Columns = 80;
    
    Console( IPixelWriter* writer, const PixelColor& fg_color, const PixelColor& bg_color );
    ~Console() = default;

    void SetWriter( IPixelWriter* writer );
    void PutString( const char* s );

private:

    static constexpr int sk_ColumnBufferLen = sk_Columns + 1;

    char* CurrentLine();
    void NewLine();
    void ClearConsole();
    void Refresh();

    IPixelWriter* m_PixelWriter;
    const PixelColor m_ForeGroundColor;
    const PixelColor m_BackGroundColor;

    int m_CursorRow;
    int m_CursorColumn;
    char m_Buffer[sk_Rows][sk_ColumnBufferLen];
    RingBuffer<char*, sk_Rows> m_ConsoleLines;
};
