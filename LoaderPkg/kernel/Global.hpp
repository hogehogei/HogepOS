#pragma once 

#ifdef __cplusplus
#include <array>

#include "usb/xhci/xhci.hpp"
#include "driver/e1000e/e1000e.hpp"

#include "PCI.hpp"
#include "ACPI.hpp"
#include "Interrupt.hpp"
#include "Event.hpp"
#include "RingBuffer.hpp"
#include "MemoryManager.hpp"
#include "PixelWriter.hpp"
#include "Graphic.hpp"
#include "Layer.hpp"
#include "Console.hpp"
#include "MouseCursor.hpp"

#endif

#ifdef    GLOBAL_VARIABLE_DEFINITION
#define   EXTERN 
#else
#define   EXTERN    extern
#endif


#ifdef __cplusplus
extern "C"
{
#endif
// using newlib_support.c / MemoryManager.hpp
EXTERN caddr_t g_ProgramBreak;
EXTERN caddr_t g_ProgramBreakEnd;
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
EXTERN void* g_VolumeImage;
EXTERN IPixelWriter* g_PixelWriter;
EXTERN Console* g_Console;
EXTERN MouseCursor* g_Cursor;
EXTERN BitmapMemoryManager* g_MemManager;

EXTERN usb::xhci::Controller* g_xHC_Controller;
EXTERN driver::net::e1000e::Context* g_e1000e_Ctx;
EXTERN uint32_t g_e1000eRxIntCnt;
EXTERN LayerManager* g_LayerManager;
EXTERN ActiveLayer* g_ActiveLayer;
EXTERN int g_MouseLayerID;
EXTERN std::shared_ptr<Window> g_MainWindow;
EXTERN int g_MainWindowLayerID;
EXTERN std::shared_ptr<TopLevelWindow> g_TextBoxWindow;
EXTERN int g_TextBoxWindowID;

EXTERN FrameBuffer g_MainScreen;
EXTERN Vector2<int> g_MousePosition;
EXTERN Vector2<int> g_ScreenSize;
EXTERN std::array<InterruptDescriptor, 256> g_IDT;

EXTERN const acpi::FADT* g_FADT;
EXTERN unsigned long g_LApicTimerFreq;
#endif