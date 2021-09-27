#pragma once

#include "Graphic.hpp"
#include "PixelWriter.hpp"

class MouseCursor
{
public:
    MouseCursor( IPixelWriter* writer, PixelColor back_color, Vector2<int> initial_position );

    void MoveRelative( Vector2<int> displacement );

private:

    IPixelWriter*  m_PixelWriter;
    PixelColor     m_BackColor;
    Vector2<int>   m_Position;
};

inline static constexpr int k_MouseCursorWidth  = 15;
inline static constexpr int k_MouseCursorHeight = 24;
inline static constexpr PixelColor k_MouseTransparentColor = { 0, 0, 1 };

void MouseObserver( uint8_t buttons, int8_t displacement_x, int8_t displacement_y );
