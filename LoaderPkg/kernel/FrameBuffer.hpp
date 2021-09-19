#pragma once

#include <vector>
#include <memory>

#include "error.hpp"
#include "PixelFormat.h"
#include "Graphic.hpp"

/**
 * @brief 描画高速化のためのFrameBufferクラス
 *        VRAMを模擬してピクセルフォーマットを持つ
 */
class FrameBuffer
{
public:

    FrameBuffer() = default;
    FrameBuffer( FrameBuffer& ) = delete;
    FrameBuffer& operator=( FrameBuffer& ) = delete;

    //! @brief フレームバッファ初期化
    Error Initialize( const FrameBufferConfig& config );
    /**
     * @brief データコピー
     * @param [in] pos コピー先位置
     * @param [in] src コピー元バッファ
     * @param [in] area コピー元バッファの左上を基準とするコピー領域
     */
    Error Copy( Vector2<int> dst_pos, const FrameBuffer& src, const RectAngle<int>& src_area );

    FrameBufferPixelWriter& Writer();
    const FrameBufferConfig& Config() const;

private:

    FrameBufferConfig    m_Config;
    std::vector<uint8_t> m_Buffer;
    std::unique_ptr<FrameBufferPixelWriter> m_Writer;
};

//int BitsPerPixel( PixelFormat format );
