#pragma once

#include <algorithm>
#include "PixelWriter.hpp"

template <typename T>
struct Vector2
{
    T x, y;

    template <typename U>
    Vector2<T>& operator+=( const Vector2<U>& rhs )
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    template <typename U>
    const Vector2<T> operator+( const Vector2<U>& rhs ) const
    {
        return Vector2<T>{ x + rhs.x, y + rhs.y };
    }

    template <typename U>
    const Vector2<T> operator-( const Vector2<U>& rhs ) const
    {
        return Vector2<T>{ x - rhs.x, y - rhs.y };
    }
};

template <typename T>
Vector2<T> ElementMin( const Vector2<T>& a, const Vector2<T>& b )
{
    Vector2<T> result;
    result.x = std::min( a.x, b.x );
    result.y = std::min( a.y, b.y );
    return result;
}

template <typename T>
Vector2<T> ElementMax( const Vector2<T>& a, const Vector2<T>& b )
{
    Vector2<T> result;
    result.x = std::max( a.x, b.x );
    result.y = std::max( a.y, b.y );
    return result; 
}

template <typename T>
class RectAngle
{
public:

    RectAngle() = default;
    RectAngle( const Vector2<T>& p, const Vector2<T>& s )
     : pos(p), size(s) {}

    RectAngle<T> Intersection( const RectAngle<T>& area ) const
    {
        const auto& rhs = pos;
        const auto& lhs = area.pos;
        const auto lhs_end = pos + size;
        const auto rhs_end = area.pos + area.size;

        if( lhs_end.x < rhs.x || lhs_end.y < rhs.y ||
            rhs_end.x < lhs.x || rhs_end.y < lhs.y )
        {
            return {{0, 0}, {0, 0}};
        }

        auto new_pos = ElementMax( lhs, rhs );
        auto new_size = ElementMin( lhs_end, rhs_end ) - new_pos;

        return {new_pos, new_size};
    }

    Vector2<T> pos;
    Vector2<T> size;
};

constexpr PixelColor sk_DesktopBGColor = { 1, 36, 86 };

void FillRectAngle( IPixelWriter& writer, const Vector2<int>& pos, const Vector2<int>& size, const PixelColor& color );
void DrawRectAngle( IPixelWriter& writer, const Vector2<int>& pos, const Vector2<int>& size, const PixelColor& color );
void EraseMouseCursor( IPixelWriter& writer, const Vector2<int>& pos, const PixelColor& back_color );
void DrawMouseCursor( IPixelWriter& writer, const Vector2<int>& pos );
void DrawDesktop( IPixelWriter& writer );