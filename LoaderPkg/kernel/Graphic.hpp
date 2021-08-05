#pragma once

#include "PixelWriter.hpp"

template <typename T>
class Vector2
{
public:

    T x, y;

    template <typename U>
    Vector2<T>& operator+=( const Vector2<U>& rhs )
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
};

void FillRectAngle( IPixelWriter& writer, const Vector2<int>& pos, const Vector2<int>& size, const PixelColor& color );
void DrawRectAngle( IPixelWriter& writer, const Vector2<int>& pos, const Vector2<int>& size, const PixelColor& color );
void DrawMouseCursor( IPixelWriter& writer, const Vector2<int>& pos );