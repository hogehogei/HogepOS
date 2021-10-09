
//
// include files
//
#include "Interrupt.hpp"
#include "Global.hpp"
#include "Event.hpp"
#include "PCI.hpp"
#include "MSI.hpp"
#include "Timer.hpp"

#include "logger.hpp"
#include "asmfunc.h"


void InitializeInterrupt()
{
    const uint16_t cs = GetCS();
    SetIDTEntry( g_IDT[InterruptVector::kXHCI],
                 MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                 reinterpret_cast<uint64_t>(IntHandlerXHCI),
                 cs );
    SetIDTEntry( g_IDT[InterruptVector::kAPICTimer],
                 MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                 reinterpret_cast<uint64_t>(IntHandlerLAPICTimer),
                 cs );
    PrintIDTEntry( InterruptVector::kXHCI );
    LoadIDT( sizeof(g_IDT) - 1, reinterpret_cast<uintptr_t>(&g_IDT[0]) );

    const uint8_t bsp_local_apic_id = *(reinterpret_cast<const uint32_t*>(0xFEE00020)) >> 24;
    pci::ConfigureMSIFixedDestination(
        *g_xHC_Device,
        bsp_local_apic_id,
        pci::MSITriggerMode::k_Level,
        pci::MSIDeliveryMode::k_Fixed,
        InterruptVector::kXHCI,
        0
    );
}

__attribute__((interrupt))
void IntHandlerXHCI( InterruptFrame* frame )
{
    g_EventQueue.Push( Message(Message::k_InterruptXHCI) );
    NotifyEndOfInterrupt();
}

__attribute__((interrupt))
void IntHandlerLAPICTimer( InterruptFrame* frame )
{
    LAPICTimerOnInterrupt();
    NotifyEndOfInterrupt();
}

void NotifyEndOfInterrupt()
{
    *(reinterpret_cast<volatile uint32_t*>(sk_EndOfInterruptRegister)) = 0;
}

void SetIDTEntry( InterruptDescriptor& desc, 
                  InterruptDescriptorAttribute attr,
                  uint64_t offset,
                  uint16_t segment_selector )
{
    desc.Attr = attr;
    desc.OffsetLow       = offset & 0xFFFFu;
    desc.OffsetMiddle    = (offset >> 16) & 0xFFFFu;
    desc.OffsetHigh      = offset >> 32;
    desc.SegmentSelector = segment_selector; 
}

void PrintIDTEntry( uint32_t idt_entry )
{
    Log( kDebug, "IDT Offset Low: (%04X)\n", g_IDT[idt_entry].OffsetLow );
    Log( kDebug, "IDT SegmentSelector: (%04X)\n", g_IDT[idt_entry].SegmentSelector );
    Log( kDebug, "IDT InterruptAttributeDescriptor: (%04X)\n", g_IDT[idt_entry].Attr.Data );
    Log( kDebug, "IDT Offset Middle: (%04X)\n", g_IDT[idt_entry].OffsetMiddle );
    Log( kDebug, "IDT Offset High: (%08X)\n", g_IDT[idt_entry].OffsetHigh );
}