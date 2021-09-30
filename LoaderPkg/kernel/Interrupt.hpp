#pragma once

// 
// include headers
//
#include <cstdint>
#include "DescriptorX86.hpp"

//
// constants
//
static constexpr volatile uint32_t sk_EndOfInterruptRegister = 0xFEE000B0;

//
// typedef structures
//

union InterruptDescriptorAttribute
{
    uint16_t Data;
    struct {
        uint16_t InterruptStackTable: 3;
        uint16_t : 5;
        DescriptorType Type : 4;
        uint16_t : 1;
        uint16_t DiscriptorPrivilegeLevel : 2;
        uint16_t Present : 1;
    } __attribute__((packed)) Bits;
} __attribute__((packed));

struct InterruptDescriptor
{
    uint16_t OffsetLow;
    uint16_t SegmentSelector;
    InterruptDescriptorAttribute Attr;
    uint16_t OffsetMiddle;
    uint32_t OffsetHigh;
    uint32_t Reserved;
} __attribute__((packed));

class InterruptVector {
public:
    enum Number {
        kXHCI = 0x40,
        kAPICTimer = 0x41,
    };
};

struct InterruptFrame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

// 
// functions
//
constexpr InterruptDescriptorAttribute MakeIDTAttr(
    DescriptorType type,
    uint8_t descriptor_privilege_level,
    bool present = true,
    uint8_t interrupt_stack_table = 0 ) 
{
    InterruptDescriptorAttribute attr{0};

    attr.Bits.InterruptStackTable = interrupt_stack_table;
    attr.Bits.Type = type;
    attr.Bits.DiscriptorPrivilegeLevel = descriptor_privilege_level;
    attr.Bits.Present = present;
    return attr;
}

void InitializeInterrupt();

__attribute__((interrupt))
void IntHandlerXHCI( InterruptFrame* frame );

__attribute__((interrupt))
void IntHandlerLAPICTimer( InterruptFrame* frame );

void NotifyEndOfInterrupt();
void SetIDTEntry( InterruptDescriptor& desc, 
                  InterruptDescriptorAttribute attr,
                  uint64_t offset,
                  uint16_t segment_selector );
                  
void PrintIDTEntry( uint32_t idt_entry );
