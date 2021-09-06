#pragma once

#include "PixelWriter.hpp"

template <typename T>
class Vector2
{
public:

    T x, y;

    Vector2( T tx, T ty ) : x(tx), y(ty) {}

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
void EraseMouseCursor( IPixelWriter& writer, const Vector2<int>& pos, const PixelColor& back_color );
void DrawMouseCursor( IPixelWriter& writer, const Vector2<int>& pos );
