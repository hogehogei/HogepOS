#pragma once

// 
// include headers
//
#include <cstdint>

namespace acpi
{

struct RSDP
{
    char     Signature[8];
    uint8_t  Checksum;
    char     OemID[6];
    uint8_t  Revision;
    uint32_t RSDT_Address;
    uint32_t Length;
    uint64_t XSDT_Address;
    uint8_t  ExtendedChecksum;
    char     Reserved[3];

    bool IsValid() const;
}  __attribute__((packed));

struct DescriptionHeader 
{
    char     Signature[4];
    uint32_t Length;
    uint8_t  Revision;
    uint8_t  Checksum;
    char     OemID[6];
    char     OemTableID[8];
    uint32_t OemRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;

    bool IsValid( const char* expected_signature ) const;
} __attribute__((packed));

struct XSDT
{
    DescriptionHeader Header;

    const DescriptionHeader& operator[]( size_t i ) const;
    size_t Count() const;
} __attribute__((packed));

struct FADT 
{
    DescriptionHeader Header;

    char Reserved1[76 - sizeof(Header)];
    uint32_t PmTmrBlk;
    char Reserved2[112 - 80];
    uint32_t Flags;
    char Reserved3[276 - 116];
} __attribute__((packed));

void Initialize( const RSDP& rsdp );
void WaitMillSeconds( uint32_t msec );

}   // end of namespace acpi