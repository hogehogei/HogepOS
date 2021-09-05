#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>

#include "error.hpp"

namespace
{
    constexpr unsigned long long operator""_KiB( unsigned long long kib )
    {
        return kib * 1024;
    }
    constexpr unsigned long long operator""_MiB( unsigned long long mib )
    {
        return mib * 1024_KiB;
    }
    constexpr unsigned long long operator""_GiB( unsigned long long gib )
    {
        return gib * 1024_MiB;
    }
}

// @brief 物理メモリフレーム1つの大きさ(Byte)
inline static constexpr uint64_t k_BytesPerFrame = 4_KiB;

class FrameID
{
public:
    explicit FrameID( std::size_t id )
        : m_ID( id ) {}

    std::size_t ID() const
    {
        return m_ID;
    }
    void* Frame() const
    {
        return reinterpret_cast<void*>( m_ID * k_BytesPerFrame );
    }

private:

    std::size_t m_ID;
};

// @brief 未定義のページフレーム番号 無効なページフレームを表す
static inline const FrameID k_NullFrame { std::numeric_limits<std::size_t>::max() };

class MemoryFrame
{
public:

    MemoryFrame( FrameID id, std::size_t size )
        : m_StartFrameID(id),
          m_NumFrames(size)
    {}

    const FrameID& GetFrameID() const 
    {
        return m_StartFrameID;
    }
    std::size_t Size() const
    {
        return m_NumFrames;
    }

private:

    FrameID m_StartFrameID;
    std::size_t m_NumFrames;
};

class BitmapMemoryManager
{
public:
    // @brief  このメモリ管理クラスで扱える最大の物理メモリ量（バイト）
    static constexpr uint64_t k_MaxPhysicalMemoryBytes = 128_GiB;
    // @brief  k_MaxPhysicalMemoryBytes までの物理メモリを扱うための必要なフレーム数
    static constexpr uint64_t k_FrameCount = k_MaxPhysicalMemoryBytes / k_BytesPerFrame;

    // @brief  ビットマップ配列の要素型
    using MapLineType = unsigned long;
    // @brief  ビットマップ配列の１つの要素のビット数 == フレーム数
    static constexpr std::size_t k_BitsPerMapLine = 8 * sizeof(MapLineType);

public:

    // @brief  インスタンスを初期化
    BitmapMemoryManager();

    // @brief  要求されたフレーム数の領域を確保して、先頭のフレームIDを返す
    WithError<MemoryFrame> Allocate( std::size_t num_frames );
    
    Error Free( MemoryFrame allocated_frame );
    void MarkAllocated( FrameID start_frame, std::size_t num_frames );

    /** @brief  このメモリマネージャで扱うメモリ範囲を設定する
     *  この呼び出し以降、Allocate によるメモリ割り当ては設定された範囲内でのみ行われる
     *
     *  @param range_begin    メモリ範囲の始点
     *  @param range_end      メモリ範囲の終点、最終フレームの次のフレーム
     */
    void SetMemoryRange( FrameID range_begin, FrameID range_end );

private:

    bool GetBit( FrameID frame ) const;
    void SetBit( FrameID frame, bool allocated );

    std::array<MapLineType, k_FrameCount / k_BitsPerMapLine> m_AllocMap;
    
    // @brief  扱うメモリ範囲の始点
    FrameID m_RangeBegin;
    // @brief  扱うメモリ範囲の終点、最終フレームの次のフレーム
    FrameID m_RangeEnd;
};




