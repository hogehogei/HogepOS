//
// include files
//
#include "FAT.hpp"
#include "asmfunc.h"
#include "logger.hpp"

namespace fat
{
//
// constant
//

//
// static variables
//

//
// static function declaration
// 

//
// funcion definitions
// 

VolumeOperator::VolumeOperator( void* volume_image )
{   
    m_BPB = reinterpret_cast<fat::BPB*>(volume_image);
}

uintptr_t VolumeOperator::getClusterAddr( uint32_t cluster ) const
{
    if( cluster < 2 ){
        return 0;
    }

    uint64_t sector_num = 
        m_BPB->ReservedSectorCount + 
        m_BPB->NumFats * m_BPB->FatSize_32 + 
        (cluster - 2) * m_BPB->SectorsPerCluster;
    uintptr_t offset = sector_num * m_BPB->BytesPerSector;

    return reinterpret_cast<uintptr_t>(m_BPB) + offset;
}

const BPB* VolumeOperator::GetBPB() const
{
    return m_BPB;
}

uint32_t VolumeOperator::GetEntriesPerCluster() const
{
    return m_BPB->BytesPerSector / sizeof(fat::DirectoryEntry) * m_BPB->SectorsPerCluster;
}

void ReadName( const DirectoryEntry& entry, char* base, char* ext )
{
    memcpy( base, &entry.Name[0], 8 );
    base[8] = '\0';

    for( int i = 7; i >= 0 && base[i] == 0x20; --i ){
        base[i] = '\0';
    }

    memcpy( ext, &entry.Name[8], 8 );
    ext[3] = '\0';
    for( int i = 2; i >= 0 && base[i] == 0x20; --i ){
        ext[i] = '\0';
    }
}

} // namespace fat

