#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "Main.h"
#include "PixelWriter.hpp"
#include "Font.hpp"

// 
// static variables
//
char sPixelWriterBuf[sizeof(RGB8BitPerColorPixelWriter)];
IPixelWriter* sPixelWriter;

//
// static functions
//
static IPixelWriter* GetPixelWriter( const FrameBufferConfig& config );

void* operator new( size_t size, void* buf )
{
    return buf;
}

void operator delete( void* buf ) noexcept
{}

extern "C" void KernelMain( const FrameBufferConfig* config )
{
    char stringbuf[128];
    sPixelWriter = GetPixelWriter( *config );

    for( int y = 0; y < 100; ++y ){
        for( int x = 0; x < 255; ++x ){
            sPixelWriter->Write( x, y, {255, 255, 255} );
        }
    }

    sprintf( stringbuf, "Hello world! %d", 999 );
    WriteString( *sPixelWriter, 10, 0, stringbuf, {0, 0, 0} );

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
        writer = new (sPixelWriterBuf) RGB8BitPerColorPixelWriter(config);
        break;
    case kPixelBGRReserved8BitPerColor:
        writer = new (sPixelWriterBuf) BGR8BitPerColorPixelWriter(config);
        break;
    default:
        break;
    }

    return writer;
}