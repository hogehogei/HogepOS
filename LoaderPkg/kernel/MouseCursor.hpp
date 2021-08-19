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