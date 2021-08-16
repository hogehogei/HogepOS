#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "Main.h"
#include "PixelWriter.hpp"
#include "Font.hpp"
#include "Console.hpp"
#include "PCI.hpp"

// 
// static variables
//
char s_PixelWriterBuf[sizeof(RGB8BitPerColorPixelWriter)];
IPixelWriter* s_PixelWriter;

char s_ConsoleBuf[sizeof(Console)];
Console* s_Console;

const pci::Device* s_xHC_Device;

//
// static functions
//
static IPixelWriter* GetPixelWriter( const FrameBufferConfig& config );
static void SearchIntelxHC( void );
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

    pci::ConfigurationArea().Instance().ScanAllBus();
    const auto& devices = pci::ConfigurationArea().Instance().GetDevices();
    int device_num = pci::ConfigurationArea().Instance().GetDeviceNum();

    Printk( "NumDevice : %d\n", device_num );
    for( int i = 0; i < device_num; ++i ){
        const auto& dev = devices[i];
        auto vendor_id  = pci::ConfigurationArea::Instance().ReadVendorID( dev.Bus, dev.Device, dev.Function );
        auto class_code = pci::ConfigurationArea::Instance().ReadClassCode( dev.Bus, dev.Device, dev.Function );
        Printk( "%d.%d.%d: vend %04x, class %08x, head %02x\n",
                dev.Bus, dev.Device, dev.Function,
                vendor_id, class_code, dev.HeaderType );
    }

    SearchIntelxHC();

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
static void SearchIntelxHC( void )
{
    s_xHC_Device = nullptr;
    const auto& devices = pci::ConfigurationArea::Instance().GetDevices();

    for( int i = 0; i < pci::sk_DeviceMax; ++i ){
        const auto& dev = devices[i];
        if( dev.ClassCode.Match( 0x0cu, 0x03u, 0x30u ) ){
            s_xHC_Device = &devices[i];
            if( pci::ConfigurationArea::Instance().ReadVendorID(*s_xHC_Device) == 0x8086 ){
                break;;
            }
        }
    }

    if( s_xHC_Device ){
        const auto& classcode = s_xHC_Device->ClassCode;
        Printk( "xHC has been found: %d.%d.%d.\n",
                classcode.Base(), classcode.Sub(), classcode.Interface() );
    }
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
