
#include "logger.hpp"

#include "Interrupt.hpp"
#include "Global.hpp"


__attribute__((interrupt))
void IntHandlerXHCI( InterruptFrame* frame )
{
    Log( kDebug, "IntHandler invoked\n" );
    while( g_xHC_Controller->PrimaryEventRing()->HasFront() ){
        auto err = ProcessEvent( *g_xHC_Controller );
        if( err ){
            Log( kDebug, "Error while ProcessEvent: %s at %s:%d\n",
                 err.Name(), err.File(), err.Line() );
        }
    }

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