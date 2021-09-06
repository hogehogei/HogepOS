
//
// include files
//
#include "Graphic.hpp"
#include "MouseCursor.hpp"

//
// constant
//

//
// static variables
//
const char sk_MouseCursorShape[k_MouseCursorHeight][k_MouseCursorWidth + 1] = {
  "@              ",
  "@@             ",
  "@.@            ",
  "@..@           ",
  "@...@          ",
  "@....@         ",
  "@.....@        ",
  "@......@       ",
  "@.......@      ",
  "@........@     ",
  "@.........@    ",
  "@..........@   ",
  "@...........@  ",
  "@............@ ",
  "@......@@@@@@@@",
  "@......@       ",
  "@....@@.@      ",
  "@...@ @.@      ",
  "@..@   @.@     ",
  "@.@    @.@     ",
  "@@      @.@    ",
  "@       @.@    ",
  "         @.@   ",
  "         @@@   ",
};

//
// static function declaration
// 

//
// funcion definitions
// 
void FillRectAngle( IPixelWriter& writer, const Vector2<int>& pos, const Vector2<int>& size, const PixelColor& color )
{
    for( int dy = 0; dy < size.y; ++dy ){
        for( int dx = 0; dx < size.x; ++dx ){
            writer.Write( pos.x + dx, pos.y + dy, color );
        }
    }
}

void DrawRectAngle( IPixelWriter& writer, const Vector2<int>& pos, const Vector2<int>& size, const PixelColor& color )
{
    for( int dy = 0; dy < size.y; ++dy ){
        writer.Write( pos.x             , pos.y + dy, color );
        writer.Write( pos.x + size.x - 1, pos.y + dy, color );
    }
    for( int dx = 0; dx < size.x; ++dx ){
        writer.Write( pos.x + dx, pos.y             , color );
        writer.Write( pos.x + dx, pos.y + size.y - 1, color );
    }
}

void EraseMouseCursor( IPixelWriter& writer, const Vector2<int>& pos, const PixelColor& back_color )
{
    for( int dy = 0; dy < k_MouseCursorHeight; ++dy ){
        for( int dx = 0; dx < k_MouseCursorWidth; ++dx ){
            writer.Write( pos.x + dx, pos.y + dy, back_color );
        }
    }
}

void DrawMouseCursor( IPixelWriter& writer, const Vector2<int>& pos )
{
    for( int dy = 0; dy < k_MouseCursorHeight; ++dy ){
        for( int dx = 0; dx < k_MouseCursorWidth; ++dx ){
            if( sk_MouseCursorShape[dy][dx] == '@' ){
                writer.Write( pos.x + dx, pos.y + dy, {0, 0, 0} );
            }
            else if( sk_MouseCursorShape[dy][dx] == '.' ){
                writer.Write( pos.x + dx, pos.y + dy, {255, 255, 255} );
            }
            else {
                writer.Write( pos.x + dx, pos.y + dy, k_MouseTransparentColor );
            }
        }
    }
}