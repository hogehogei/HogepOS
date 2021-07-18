#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "Main.h"
#include "PixelWriter.hpp"
#include "Font.hpp"
#include "Console.hpp"

// 
// static variables
//
char s_PixelWriterBuf[sizeof(RGB8BitPerColorPixelWriter)];
IPixelWriter* s_PixelWriter;

char s_ConsoleBuf[sizeof(Console)];
Console* s_Console;

//
// static functions
//
static IPixelWriter* GetPixelWriter( const FrameBufferConfig& config );
static int Printk( const char* format, ... );

void* operator new( size_t size, void* buf )
{
    return buf;
}

void operator delete( void* buf ) noexcept
{}

extern "C" void KernelMain( const FrameBufferConfig* config )
{
    s_PixelWriter = GetPixelWriter( *config );
    s_Console = new(s_ConsoleBuf) Console( s_PixelWriter, {0, 0, 0}, {255, 255, 255} );

    for( int i = 0; i < 27; ++i ){
        Printk( "printk: %d\n", i );
    }

    while(1){
        __asm__ ("hlt");
    }
}

static IPixelWriter* GetPixelWriter( const FrameBufferConfig& config )
{
    IPixelWriter* writer = nullptr;

    switch( config.PixelFormat )
    {
    case kPixelRGBReserved8BitPerColor:
        writer = new (s_PixelWriterBuf) RGB8BitPerColorPixelWriter(config);
        break;
    case kPixelBGRReserved8BitPerColor:
        writer = new (s_PixelWriterBuf) BGR8BitPerColorPixelWriter(config);
        break;
    default:
        break;
    }

    return writer;
}

static int Printk( const char* format, ... )
{
    va_list ap;
    int result;
    char s[1024];

    va_start( ap, format );
    result = vsprintf( s, format, ap );
    va_end( ap );

    s_Console->PutString( s );
    return result;
}
