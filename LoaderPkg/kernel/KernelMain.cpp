#include <cstdint>
#include <cstddef>
#include "Main.h"
#include "PixelWriter.hpp"

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
    sPixelWriter = GetPixelWriter( *config );

    for( int y = 0; y < 100; ++y ){
        for( int x = 0; x < 255; ++x ){
            sPixelWriter->Write( x, y, {255, 0, 0} );
        }
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