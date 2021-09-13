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
     */
    Error Copy( Vector2<int> pos, const FrameBuffer& src );

    FrameBufferPixelWriter& Writer();

private:

    FrameBufferConfig    m_Config;
    std::vector<uint8_t> m_Buffer;
    std::unique_ptr<FrameBufferPixelWriter> m_Writer;
};

int BitsPerPixel( PixelFormat format );
