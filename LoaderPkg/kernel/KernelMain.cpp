#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "Main.h"
#include "PixelWriter.hpp"
#include "Font.hpp"
#include "Console.hpp"
#include "PCI.hpp"
#include "MouseCursor.hpp"

#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"
#include "logger.hpp"

// 
// constant
//
static constexpr PixelColor sk_DesktopBGColor = { 1, 36, 86 };
// 
// static variables
//
static char s_PixelWriterBuf[sizeof(RGB8BitPerColorPixelWriter)];
IPixelWriter* g_PixelWriter;

static char s_ConsoleBuf[sizeof(Console)];
Console* g_Console;

static char s_MouseCursorBuf[sizeof(MouseCursor)];
MouseCursor* g_Cursor;

static const pci::Device* s_xHC_Device;

//
// static functions
//
static IPixelWriter* GetPixelWriter( const FrameBufferConfig& config );
static void SearchIntelxHC();
static uint64_t Read_xHC_Bar();
static void Init_XHCI_Controller( usb::xhci::Controller& controller );
static void SwitchEhci2Xhci( const pci::Device& xhc_dev );
static void MouseObserver( int8_t displacement_x, int8_t displacement_y );
static int Printk( const char* format, ... );

extern "C" void KernelMain( const FrameBufferConfig* config )
{
    g_PixelWriter = GetPixelWriter( *config );
    g_Console = new(s_ConsoleBuf) Console( g_PixelWriter, {255, 255, 255}, sk_DesktopBGColor );
    g_Cursor  = new(s_MouseCursorBuf) MouseCursor( g_PixelWriter, sk_DesktopBGColor, {300, 200} );

    //SetLogLevel( kDebug );

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
    uint64_t xhc_mmio_base = Read_xHC_Bar();

    usb::xhci::Controller xhc { xhc_mmio_base };
    Printk( "xHC MMIO Base : %llx\n", xhc_mmio_base );
    Init_XHCI_Controller( xhc );

    
    usb::HIDMouseDriver::default_observer = MouseObserver;
    for( int i = 1; i <= xhc.MaxPorts(); ++i ){
        auto port = xhc.PortAt(i);
        Printk( "Port %d : IsConnected=%d\n", i, port.IsConnected() );

        if( port.IsConnected() ){
            auto err = ConfigurePort( xhc, port );
            if( err ){
                Printk( "Failed to configure port: %s at %s:%d\n",
                        err.Name(), err.File(), err.Line() );
            }
        }
    }


    while(1){
        auto err = ProcessEvent(xhc);
        if( err ){
            Printk( "Error while ProcessEvent: %s at %s:%d\n",
                    err.Name(), err.File(), err.Line() );
        }
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
static void SearchIntelxHC()
{
    auto& pci_mgr = pci::ConfigurationArea::Instance();
    s_xHC_Device = nullptr;

    const auto& devices = pci_mgr.GetDevices();
    int device_num = pci_mgr.GetDeviceNum();
    for( int i = 0; i < device_num; ++i ){
        const auto& dev = devices[i];
        if( dev.ClassCode.Match( 0x0cu, 0x03u, 0x30u ) ){
            s_xHC_Device = &devices[i];
            if( pci_mgr.ReadVendorID(*s_xHC_Device) == 0x8086 ){
                break;
            }
        }
    }

    if( s_xHC_Device ){
        const auto& classcode = s_xHC_Device->ClassCode;
        Printk( "xHC has been found: %d.%d.%d.\n",
                classcode.Base(), classcode.Sub(), classcode.Interface() );
    }
}

static uint64_t Read_xHC_Bar()
{
    auto xhc_bar = pci::ConfigurationArea::Instance().ReadBAR( *s_xHC_Device, 0 );
    if( !xhc_bar.error ){
        Printk( "Read xHC Bar failed.\n", xhc_bar.error.Name() );
    }
    return xhc_bar.value & ~static_cast<uint64_t>(0xF);
}

static void Init_XHCI_Controller( usb::xhci::Controller& controller )
{
    if( pci::ConfigurationArea::Instance().ReadVendorID( *s_xHC_Device ) ){
        SwitchEhci2Xhci( *s_xHC_Device );
    }

    auto err = controller.Initialize();
    Printk( "xhc.Initialize: %s\n", err.Name() );

    Printk( "xHC starting.\n" );
    controller.Run();
}

static void SwitchEhci2Xhci( const pci::Device& xhc_dev )
{
    auto& pci_mgr = pci::ConfigurationArea::Instance();
    bool intel_ehc_exist = false;

    const auto& devices = pci_mgr.GetDevices();
    int device_num = pci_mgr.GetDeviceNum();
    for( int i = 0; i < device_num; ++i ){
        // classcode = EHCI 
        if( devices[i].ClassCode.Match( 0x0Cu, 0x03u, 0x20u ) ){
            intel_ehc_exist = true;
            break;
        }
    }

    if( !intel_ehc_exist ){
        return;
    }


    uint32_t superspeed_ports = pci_mgr.ReadConfReg( xhc_dev, 0xDC );   // USB3PRM
    pci_mgr.WriteConfReg( xhc_dev, 0xD8, superspeed_ports );            // USB3_PSSEN
    uint32_t ehci2xhci_ports  = pci_mgr.ReadConfReg( xhc_dev, 0xD4 );   // XUSB2PRM
    pci_mgr.WriteConfReg( xhc_dev, 0xD0, ehci2xhci_ports );             // XUSB2PR

    Printk( "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n",
            superspeed_ports, ehci2xhci_ports );
}

static void MouseObserver( int8_t displacement_x, int8_t displacement_y )
{
    g_Cursor->MoveRelative( {displacement_x, displacement_y} );
}

static int Printk( const char* format, ... )
{
    va_list ap;
    int result;
    char s[1024];

    va_start( ap, format );
    result = vsprintf( s, format, ap );
    va_end( ap );

    g_Console->PutString( s );
    return result;
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}
