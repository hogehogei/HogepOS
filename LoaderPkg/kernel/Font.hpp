#pragma once

#include "PixelWriter.hpp"

const uint8_t* GetFont( char c );

void WriteAscii( IPixelWriter& writer, int x, int y, char c, const PixelColor& color );
void WriteString( IPixelWriter& writer, int x, int y, const char* s, const PixelColor& color );