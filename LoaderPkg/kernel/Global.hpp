#pragma once 

#include <array>

#include "usb/xhci/xhci.hpp"

#include "PCI.hpp"
#include "Interrupt.hpp"
#include "Event.hpp"

#ifdef    GLOBAL_VARIABLE_DEFINITION
#define   EXTERN 
#else
#define   EXTERN    extern
#endif

EXTERN const pci::Device* g_xHC_Device;
EXTERN usb::xhci::Controller* g_xHC_Controller;

EXTERN std::array<InterruptDescriptor, 256> g_IDT;
EXTERN RingBuffer<Message, 32> g_EventQueue;