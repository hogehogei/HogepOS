#pragma once

#include <cstdint>
#include <cstddef>

#include "error.hpp"

namespace fat {

struct BPB {
    uint8_t JumpBoot[3];
    char OemName[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectorCount;
    uint8_t NumFats;
    uint16_t RootEntryCount;
    uint16_t TotalSectors_16;
    uint8_t Media;
    uint16_t FatSize_16;
    uint16_t SectorsPerTrack;
    uint16_t NumHeads;
    uint32_t HiddenSectors;
    uint32_t TotalSectors_32;
    uint32_t FatSize_32;
    uint16_t ExtFlags;
    uint16_t FsVersion;
    uint32_t RootCluster;
    uint16_t FsInfo;
    uint16_t BackupBootSector;
    uint8_t Reserved[12];
    uint8_t DriveNumber;
    uint8_t Reserved1;
    uint8_t BootSignature;
    uint32_t VolumeId;
    char VolumeLabel[11];
    char FsType[8];
} __attribute__((packed));

enum class Attribute : uint8_t {
    kReadOnly  = 0x01,
    kHidden    = 0x02,
    kSystem    = 0x04,
    kVolumeID  = 0x08,
    kDirectory = 0x10,
    kArchive   = 0x20,
    kLongName  = 0x0f,
};

struct DirectoryEntry {
    unsigned char Name[11];
    Attribute Attr;
    uint8_t Ntres;
    uint8_t CreateTimeTenth;
    uint16_t CreateTime;
    uint16_t CreateDate;
    uint16_t LastAccessDate;
    uint16_t FirstClusterHigh;
    uint16_t WriteTime;
    uint16_t WriteDate;
    uint16_t FirstClusterLow;
    uint32_t FileSize;

    uint32_t FirstCluster() const {
        return FirstClusterLow |
        (static_cast<uint32_t>(FirstClusterHigh) << 16);
    }
} __attribute__((packed));


class VolumeOperator
{
public:

    VolumeOperator( void* volume_image );
    ~VolumeOperator() = default;
    VolumeOperator( VolumeOperator& ) = delete;
    VolumeOperator& operator=( VolumeOperator& ) = delete;

    template <typename T>
    T* GetSectorByCluster( uint32_t cluster ) const
    {
        return reinterpret_cast<T*>(getClusterAddr(cluster));
    }
    
    const BPB* GetBPB() const;
    uint32_t GetEntriesPerCluster() const;

private:

    uintptr_t getClusterAddr( uint32_t cluster ) const;

    BPB* m_BPB;
};

void ReadName( const DirectoryEntry& entry, char* base, char* ext );

}    // namespace fat