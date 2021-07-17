#include <cstdint>
#include "Font.hpp"
#include "PixelWriter.hpp"


extern const uint8_t _binary_hankaku_font_bin_start[];
extern const uint8_t _binary_hankaku_font_bin_end[];
extern const uint8_t _binary_hankaku_font_bin_size[];

const uint8_t* GetFont( char c )
{
    uint32_t index = 16 * static_cast<uint32_t>( c );
    if( index >= reinterpret_cast<uintptr_t>(_binary_hankaku_font_bin_size) ){
        return nullptr;
    }

    return _binary_hankaku_font_bin_start + index;
}

void WriteAscii( IPixelWriter& writer, int x, int y, char c, const PixelColor& color )
{
    const uint8_t* font = GetFont( c );
    if( font == nullptr ){
        return;
    }

    for( int dy = 0; dy < 16; ++dy ){
        for( int dx = 0; dx < 8; ++dx ){
            if( font[dy] & (static_cast<uint32_t>(0x80) >> dx) ){
                writer.Write( x + dx, y + dy, color );
            }
        }
    }
}

void WriteString( IPixelWriter& writer, int x, int y, const char* s, const PixelColor& color )
{
    for( int i = 0; s[i] != '\0'; ++i ){
        WriteAscii( writer, x + 8 * i, y, s[i], color );
    }
}