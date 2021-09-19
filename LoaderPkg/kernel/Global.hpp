#pragma once 

#ifdef __cplusplus
#include <array>

#include "usb/xhci/xhci.hpp"

#include "PCI.hpp"
#include "Interrupt.hpp"
#include "Event.hpp"
#include "RingBuffer.hpp"
#include "Graphic.hpp"
#include "Layer.hpp"
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
EXTERN const pci::Device* g_xHC_Device;
EXTERN usb::xhci::Controller* g_xHC_Controller;
EXTERN LayerManager* g_LayerManager;
EXTERN int g_MouseLayerID;
EXTERN int g_MainWindowLayerID;

EXTERN Vector2<int> g_MousePosition;
EXTERN Vector2<int> g_ScreenSize;
EXTERN std::array<InterruptDescriptor, 256> g_IDT;
EXTERN RingBuffer<Message, 32> g_EventQueue;
#endif