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

uintptr_t VolumeOperator::getClusterAddr( uint32_t cluster )
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

uint32_t VolumeOperator::GetEntriesPerCluster() const
{
    
}

} // namespace fat

