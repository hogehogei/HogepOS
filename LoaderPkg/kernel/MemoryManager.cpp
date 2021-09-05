//
// include files
//
#include "MemoryManager.hpp"

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

    // @brief  インスタンスを初期化
BitmapMemoryManager::BitmapMemoryManager()
    : m_AllocMap(),
      m_RangeBegin( FrameID(0) ),
      m_RangeEnd( FrameID(0) )
{}

    // @brief  要求されたフレーム数の領域を確保して、先頭のフレームIDを返す
WithError<MemoryFrame> BitmapMemoryManager::Allocate( std::size_t num_frames )
{
    std::size_t start_frame_id = m_RangeBegin.ID();

    while(1){
        std::size_t i = 0;

        for( ; i < num_frames; ++i ){
            if( start_frame_id + i >= m_RangeEnd.ID() ){
                return { MemoryFrame(k_NullFrame, 0), MAKE_ERROR(Error::kNoEnoughMemory) };
            }
            if( GetBit(FrameID(start_frame_id + i)) ){
                // すでに割り当て済みなのでそれ以降は調べない
                break;
            }
        }

        if( i == num_frames ){
            // num_frames 分の連続した空き領域が見つかった
            MarkAllocated( FrameID(start_frame_id), num_frames );
            return { 
                MemoryFrame(FrameID(start_frame_id), num_frames), 
                MAKE_ERROR(Error::kSuccess) 
            };
        }

        // 次のフレームから再検索
        start_frame_id += i + 1;
    }
}
    
Error BitmapMemoryManager::Free( MemoryFrame allocated_frame )
{
    for( std::size_t i = 0; i < allocated_frame.Size(); ++i ){
        SetBit( FrameID(allocated_frame.GetFrameID().ID() + i), false );
    }

    return MAKE_ERROR(Error::kSuccess);
}

void BitmapMemoryManager::MarkAllocated( FrameID start_frame, std::size_t num_frames )
{
    for( std::size_t i = 0; i < num_frames; ++i ){
        SetBit( FrameID(start_frame.ID() + i ), true );
    }
}

void BitmapMemoryManager::SetMemoryRange( FrameID range_begin, FrameID range_end )
{
    m_RangeBegin = range_begin;
    m_RangeEnd   = range_end;
}

bool BitmapMemoryManager::GetBit( FrameID frame ) const
{
    std::size_t byte_idx = frame.ID() / k_BitsPerMapLine;
    std::size_t bit_idx  = frame.ID() % k_BitsPerMapLine;
    MapLineType bit_pos  = static_cast<MapLineType>(1) << bit_idx;

    return (m_AllocMap[byte_idx] & bit_pos) != 0;
}

void BitmapMemoryManager::SetBit( FrameID frame, bool allocated )
{
    std::size_t byte_idx = frame.ID() / k_BitsPerMapLine;
    std::size_t bit_idx  = frame.ID() % k_BitsPerMapLine;
    MapLineType bit_pos  = static_cast<MapLineType>(1) << bit_idx;

    if( allocated ){
        m_AllocMap[byte_idx] |= bit_pos;
    }
    else {
        m_AllocMap[byte_idx] &= ~bit_pos;
    }
}
