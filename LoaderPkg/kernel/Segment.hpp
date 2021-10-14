#pragma once

#include <cstdint>
#include "DescriptorX86.hpp"

union SegmentDescriptor
{
    uint64_t Data;
    struct {
        uint64_t LimitLow : 16;
        uint64_t BaseLow  : 16;
        uint64_t BaseMiddle : 8;
        DescriptorType Type : 4;
        uint64_t SystemSegment : 1;
        uint64_t DescriptorPrivilegeLevel : 2;
        uint64_t Present : 1;
        uint64_t LimitHigh : 4;
        uint64_t Available : 1;
        uint64_t LongMode : 1;
        uint64_t DefaultOperationSize : 1;
        uint64_t Granularity : 1;
        uint64_t BaseHigh : 8;
    } __attribute__((packed)) Bits;
} __attribute__((packed));

constexpr uint16_t k_KernelCS = 1 << 3;
constexpr uint16_t k_KernelSS = 2 << 3;
constexpr uint16_t k_KernelDS = 0;

void SetCodeSegment( SegmentDescriptor& desc, 
                     DescriptorType type,
                     uint32_t descriptor_privilege_level, 
                     uint32_t base,
                     uint32_t limit );
void SetDataSegment( SegmentDescriptor& desc, 
                     DescriptorType type,
                     uint32_t descriptor_privilege_level, 
                     uint32_t base,
                     uint32_t limit );

void SetupSegments();