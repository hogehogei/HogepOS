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
#include "Mouse.hpp"
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

// 
// static variables
//
static char s_PixelWriterBuf[sizeof(RGB8BitPerColorPixelWriter)];
static char s_ConsoleBuf[sizeof(Console)];
static char s_MouseCursorBuf[sizeof(MouseCursor)];
static char s_MemoryManagerBuf[sizeof(BitmapMemoryManager)];

static char s_String[128];
static unsigned int s_Count = 0;

alignas(16) uint8_t s_KernelMainStack[2048*1024];

//
// static functions
//
static void SetupMemory();
static void InitMemoryManager( const MemoryMap* memory_map );
static IPixelWriter* GetPixelWriter( const FrameBufferConfig& config );
static void ShowMemoryType( const MemoryMap* memory_map );



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

    pci::InitializePCI();
    InitializeInterrupt();
    InitializeMouse();
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
        case Message::k_InterruptLAPICTimer:
            Printk( "Timer interrupt\n" );
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

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}
