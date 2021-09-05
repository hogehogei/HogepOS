//
// include files
//
#include <array>

#include "Segment.hpp"
#include "asmfunc.h"

//
// constant
//

//
// static variables
//
static std::array<SegmentDescriptor, 3> g_GDT;
//
// static function declaration
// 

//
// funcion definitions
// 

void SetCodeSegment( SegmentDescriptor& desc, 
                     DescriptorType type,
                     uint32_t descriptor_privilege_level, 
                     uint32_t base,
                     uint32_t limit )
{
    desc.Data = 0;

    desc.Bits.BaseLow    = base & 0xFFFFu;
    desc.Bits.BaseMiddle = (base >> 16) & 0xFFu;
    desc.Bits.BaseHigh   = (base >> 24) & 0xFFu;

    desc.Bits.LimitLow   = limit & 0xFFFFu;
    desc.Bits.LimitHigh  = (limit >> 16) & 0xFu;

    desc.Bits.Type = type;
    desc.Bits.SystemSegment            = 1;            // 1: code & data segment
    desc.Bits.DescriptorPrivilegeLevel = descriptor_privilege_level;
    desc.Bits.Present   = 1;
    desc.Bits.Available = 0;
    desc.Bits.LongMode  = 1;
    desc.Bits.DefaultOperationSize = 0;     // should be 0 when long_mode == 1
    desc.Bits.Granularity = 1;
}

void SetDataSegment( SegmentDescriptor& desc, 
                     DescriptorType type,
                     uint32_t descriptor_privilege_level, 
                     uint32_t base,
                     uint32_t limit )
{
    SetCodeSegment( desc, type, descriptor_privilege_level, base, limit );
    desc.Bits.LongMode = 0;
    desc.Bits.DefaultOperationSize = 1;     // 32-bit stack segment
}

void SetupSegments()
{
    g_GDT[0].Data = 0;
    SetCodeSegment( g_GDT[1], DescriptorType::kExecuteRead, 0, 0, 0xFFFFF );
    SetDataSegment( g_GDT[2], DescriptorType::kReadWrite,   0, 0, 0xFFFFF );
    LoadGDT( sizeof(g_GDT) - 1, reinterpret_cast<uintptr_t>(&g_GDT[0]) );
}