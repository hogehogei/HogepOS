#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "../MemoryMap.hpp"
#include "Main.h"
#include "Segment.hpp"
#include "Paging.hpp"
#include "MemoryManager.hpp"
#include "Timer.hpp"
#include "PCI.hpp"
#include "MSI.hpp"
#include "Interrupt.hpp"
#include "MouseCursor.hpp"
#include "PixelWriter.hpp"
#include "Font.hpp"
#include "Console.hpp"

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

static char s_MemoryManagerBuf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager* g_MemManager;

static char s_String[128];
static unsigned int s_Count = 0;
static std::shared_ptr<Window> g_MainWindow;

alignas(16) uint8_t s_KernelMainStack[2048*1024];

//
// static functions
//
static void SetupMemory();
static void InitMemoryManager( const MemoryMap* memory_map );
static IPixelWriter* GetPixelWriter( const FrameBufferConfig& config );
static void ShowMemoryType( const MemoryMap* memory_map );
static void SearchIntelxHC();
static uint64_t Read_xHC_Bar();
static void Init_XHCI_Controller( usb::xhci::Controller& controller );
static void SwitchEhci2Xhci( const pci::Device& xhc_dev );
static void MouseObserver( int8_t displacement_x, int8_t displacement_y );
static void CreateLayer( const FrameBufferConfig& config, FrameBuffer* screen );
static void DrawDesktop( IPixelWriter& writer );
static int Printk( const char* format, ... );

extern "C" void KernelMainNewStack( const FrameBufferConfig* config_in, const MemoryMap* memory_map_in )
{
    FrameBufferConfig config( *config_in );
    MemoryMap memory_map( *memory_map_in );

    SetupMemory();
    InitializeLAPICTimer();

    g_PixelWriter = GetPixelWriter( config );
    g_Console = new(s_ConsoleBuf) Console( {255, 255, 255}, sk_DesktopBGColor );
    g_Cursor  = new(s_MouseCursorBuf) MouseCursor( g_PixelWriter, sk_DesktopBGColor, {300, 200} );
    g_MemManager = new(s_MemoryManagerBuf) BitmapMemoryManager();

    FrameBuffer screen;
    screen.Initialize( config );

    InitMemoryManager( &memory_map );
    InitializeHeap( *g_MemManager );
    SetLogLevel( kInfo );

    g_MousePosition = Vector2<int>( 100, 100 );
    g_ScreenSize = Vector2<int>( config.HorizontalResolution, config.VerticalResolution );
    CreateLayer( config, &screen );

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

    ShowMemoryType( &memory_map );

    while(1){
        ++s_Count;
        sprintf( s_String, "%010u", s_Count );
        FillRectAngle( *g_MainWindow->Writer(), {24, 28}, {8*10, 16}, {0xc6, 0xc6, 0xc6} );
        WriteString( *g_MainWindow->Writer(), 24, 28, s_String, {0, 0, 0} );
        g_LayerManager->Draw( g_MainWindowLayerID );
        __asm__("cli");

        if( g_EventQueue.IsEmpty() ){
            __asm__("sti");
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

static void SetupMemory()
{
    SetupSegments();

    const uint16_t kernel_cs = 1 << 3;
    const uint16_t kernel_ss = 2 << 3;
    SetDSAll( 0 );
    SetCSSS( kernel_cs, kernel_ss );

    SetupIdentityPageTable();
}

static void InitMemoryManager( const MemoryMap* memory_map )
{
    const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map->buffer);   
    uintptr_t available_end = 0;

    for( uintptr_t itr = memory_map_base; 
         itr < memory_map_base + memory_map->map_size;
         itr += memory_map->descriptor_size )
    {
        auto desc = reinterpret_cast<const MemoryDescriptor*>(itr);

        if( available_end < desc->physical_start ){
            g_MemManager->MarkAllocated( 
                FrameID(available_end / k_BytesPerFrame),                    // start frame
                (desc->physical_start - available_end) / k_BytesPerFrame    // frame size
            );
        }

        const auto physical_end = desc->physical_start + desc->number_of_pages * kUEFIPageSize;
        if( IsAvailableUEFIMemoryType(static_cast<MemoryType>(desc->type)) ){
            available_end = physical_end;
        }
        else {
            g_MemManager->MarkAllocated(
                FrameID(desc->physical_start / k_BytesPerFrame),
                desc->number_of_pages * kUEFIPageSize / k_BytesPerFrame
            );
        }
    }

    g_MemManager->SetMemoryRange( FrameID(1), FrameID(available_end / k_BytesPerFrame) );
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

static void ShowMemoryType( const MemoryMap* memory_map )
{
    Printk( "memory map %p\n", memory_map );

    uintptr_t buffer_begin = reinterpret_cast<uintptr_t>(memory_map->buffer);
    uintptr_t buffer_end   = buffer_begin + memory_map->map_size;
    for( uintptr_t itr = buffer_begin; itr < buffer_end; itr += memory_map->descriptor_size )
    {
        auto desc = reinterpret_cast<MemoryDescriptor*>(itr);

        if( IsAvailableUEFIMemoryType(static_cast<MemoryType>(desc->type)) ){
            Printk( "type = %u, phys = %08lx - %08lx, pages = %lu, attr = %016lx\n",
                desc->type,
                desc->physical_start,
                desc->physical_start + desc->number_of_pages * 4096 - 1,
                desc->attribute );
        }
    }
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
    auto newpos = g_MousePosition + Vector2<int>( displacement_x, displacement_y );
    newpos = ElementMin( newpos, g_ScreenSize + Vector2<int>(-1, -1) );
    g_MousePosition = ElementMax( newpos, Vector2<int>(0, 0) );

    g_LayerManager->Move( g_MouseLayerID, g_MousePosition );
    g_LayerManager->Draw( g_MouseLayerID );
}

static void CreateLayer( const FrameBufferConfig& config, FrameBuffer* screen )
{
    const int k_FrameWidth = config.HorizontalResolution;
    const int k_FrameHeight = config.VerticalResolution;

    auto bg_window = std::make_shared<Window>( k_FrameWidth, k_FrameHeight, config.PixelFormat );
    auto bg_writer = bg_window->Writer();

    DrawDesktop( *bg_writer );

    auto mouse_window = std::make_shared<Window>( k_MouseCursorWidth, k_MouseCursorHeight, config.PixelFormat );
    mouse_window->SetTransparentColor( k_MouseTransparentColor );
    DrawMouseCursor( *mouse_window->Writer(), {0, 0} );

    g_MainWindow = std::make_shared<Window>(
        160, 52, config.PixelFormat
    );
    DrawWindow( *g_MainWindow->Writer(), "Hello window" );

    auto console_window = std::make_shared<Window>(
        Console::sk_Columns * 8, Console::sk_Rows * 16, config.PixelFormat 
    );
    g_Console->SetWindow( console_window );

    g_LayerManager = new LayerManager();
    g_LayerManager->SetWriter( screen );

    auto bglayer_id = g_LayerManager->NewLayer()
        .SetWindow( bg_window )
        .Move( {0, 0} )
        .ID();
    auto mouse_layer_id = g_LayerManager->NewLayer()
        .SetWindow( mouse_window )
        .Move( g_MousePosition )
        .ID();
    auto main_window_layer_id = g_LayerManager->NewLayer()
        .SetWindow( g_MainWindow )
        .Move( {300, 100} )
        .ID();
    g_Console->SetLayerID( g_LayerManager->NewLayer()
        .SetWindow( console_window )
        .Move( {0, 0} )
        .ID()
    );

    g_LayerManager->UpDown( bglayer_id, 0 );
    g_LayerManager->UpDown( mouse_layer_id, 1 );
    g_LayerManager->UpDown( main_window_layer_id, 1 );
    g_LayerManager->Draw( bglayer_id );

    g_MouseLayerID = mouse_layer_id;
    g_MainWindowLayerID = main_window_layer_id;
}

static void DrawDesktop( IPixelWriter& writer )
{
    const auto width = writer.Width();
    const auto height = writer.Height();
    FillRectAngle(writer,
                    {0, 0},
                    {width, height - 50},
                    sk_DesktopBGColor );
    FillRectAngle(writer,
                    {0, height - 50},
                    {width, 50},
                    {1, 8, 17});
    FillRectAngle(writer,
                    {0, height - 50},
                    {width / 5, 50},
                    {80, 80, 80});
    FillRectAngle(writer,
                    {10, height - 40},
                    {30, 30},
                    {160, 160, 160});
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
