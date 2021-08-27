#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "Main.h"
#include "PixelWriter.hpp"
#include "Font.hpp"
#include "Console.hpp"
#include "PCI.hpp"
#include "MSI.hpp"
#include "Interrupt.hpp"
#include "MouseCursor.hpp"

#include "asmfunc.h"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"
#include "logger.hpp"

#define  GLOBAL_VARIABLE_DEFINITION
#include "Global.hpp"

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

    SetLogLevel( kInfo );

    pci::ConfigurationArea().Instance().ScanAllBus();
    const auto& devices = pci::ConfigurationArea().Instance().GetDevices();
    int device_num = pci::ConfigurationArea().Instance().GetDeviceNum();

   Log( kDebug, "NumDevice : %d\n", device_num );
    for( int i = 0; i < device_num; ++i ){
        const auto& dev = devices[i];
        auto vendor_id  = pci::ConfigurationArea::Instance().ReadVendorID( dev.Bus, dev.Device, dev.Function );
        auto class_code = pci::ConfigurationArea::Instance().ReadClassCode( dev.Bus, dev.Device, dev.Function );
        Log( kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n",
             dev.Bus, dev.Device, dev.Function,
             vendor_id, class_code, dev.HeaderType );
    }
    SearchIntelxHC();
    
    const uint16_t cs = GetCS();
    SetIDTEntry( g_IDT[InterruptVector::kXHCI],
                 MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                 reinterpret_cast<uint64_t>(IntHandlerXHCI),
                 cs );
    Log( kDebug, "LoadIDT: size(%d), addr(%p)\n", sizeof(g_IDT) - 1, reinterpret_cast<uintptr_t>(&g_IDT[0]) );
    PrintIDTEntry( InterruptVector::kXHCI );
    LoadIDT( sizeof(g_IDT) - 1, reinterpret_cast<uintptr_t>(&g_IDT[0]) );

    const uint8_t bsp_local_apic_id = *(reinterpret_cast<const uint32_t*>(0xFEE00020)) >> 24;
    Log( kDebug, "BSP local apic id: (%d)\n", bsp_local_apic_id );
    pci::ConfigureMSIFixedDestination(
        *g_xHC_Device,
        bsp_local_apic_id,
        pci::MSITriggerMode::k_Level,
        pci::MSIDeliveryMode::k_Fixed,
        InterruptVector::kXHCI,
        0
    );

    uint64_t xhc_mmio_base = Read_xHC_Bar();

    usb::xhci::Controller xhc { xhc_mmio_base };
    g_xHC_Controller = &xhc;
    Log( kDebug, "xHC MMIO Base : %llx\n", xhc_mmio_base );
    Init_XHCI_Controller( xhc );

    usb::HIDMouseDriver::default_observer = MouseObserver;
    for( int i = 1; i <= xhc.MaxPorts(); ++i ){
        auto port = xhc.PortAt(i);
        Log( kDebug, "Port %d : IsConnected=%d\n", i, port.IsConnected() );

        if( port.IsConnected() ){
            auto err = ConfigurePort( xhc, port );
            if( err ){
                Log( kDebug, "Failed to configure port: %s at %s:%d\n",
                     err.Name(), err.File(), err.Line() );
            }
        }
    }

    while(1){
        __asm("cli");
        if( g_EventQueue.IsEmpty() ){
            __asm__("sti\n\thlt");
            continue;
        }

        auto msg = g_EventQueue.Front();
        g_EventQueue.Pop();
        __asm__("sti");

        switch( msg.type ){
        case Message::k_InterruptXHCI:

            while( g_xHC_Controller->PrimaryEventRing()->HasFront() ){
                auto err = ProcessEvent( *g_xHC_Controller );
                if( err ){
                    Log( kDebug, "Error while ProcessEvent: %s at %s:%d\n",
                        err.Name(), err.File(), err.Line() );
                }
            }
            break;
        default:
            Log( kError, "Unknown message type: %d\n", msg.type );
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
    g_xHC_Device = nullptr;

    const auto& devices = pci_mgr.GetDevices();
    int device_num = pci_mgr.GetDeviceNum();
    for( int i = 0; i < device_num; ++i ){
        const auto& dev = devices[i];
        if( dev.ClassCode.Match( 0x0cu, 0x03u, 0x30u ) ){
            g_xHC_Device = &devices[i];
            if( pci_mgr.ReadVendorID(*g_xHC_Device) == 0x8086 ){
                break;
            }
        }
    }

    if( g_xHC_Device ){
        const auto& classcode = g_xHC_Device->ClassCode;
        Log( kDebug, "xHC has been found: %d.%d.%d.\n",
             classcode.Base(), classcode.Sub(), classcode.Interface() );
    }
}

static uint64_t Read_xHC_Bar()
{
    auto xhc_bar = pci::ConfigurationArea::Instance().ReadBAR( *g_xHC_Device, 0 );
    if( !xhc_bar.error ){
        Log( kDebug, "Read xHC Bar failed.\n", xhc_bar.error.Name() );
    }
    return xhc_bar.value & ~static_cast<uint64_t>(0xF);
}

static void Init_XHCI_Controller( usb::xhci::Controller& controller )
{
    if( pci::ConfigurationArea::Instance().ReadVendorID( *g_xHC_Device ) ){
        SwitchEhci2Xhci( *g_xHC_Device );
    }

    auto err = controller.Initialize();
    Log( kDebug, "xhc.Initialize: %s\n", err.Name() );

    Log( kDebug, "xHC starting.\n" );
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

    Log( kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n",
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
