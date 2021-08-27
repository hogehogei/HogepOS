
#include "logger.hpp"

#include "Interrupt.hpp"
#include "Global.hpp"
#include "Event.hpp"


__attribute__((interrupt))
void IntHandlerXHCI( InterruptFrame* frame )
{
    g_EventQueue.Push( Message(Message::k_InterruptXHCI) );
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