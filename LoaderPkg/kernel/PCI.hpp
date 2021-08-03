#pragma once

// 
// include headers
//
#include <cstdint>
#include <array>

namespace pci
{
    struct Device
    {
        uint8_t Bus;
        uint8_t Device;
        uint8_t Function;
        uint8_t HeaderType;
    };

    /**
     * @brief   PCIマネージャ
     *          PCIコンフィギュレーション空間への読み書きを管理する
     */
    class Manager
    {
    public:
        //! CONFIG_ADDRESS レジスタのポートアドレス
        static constexpr uint16_t sk_ConfigAddress_Addr = 0x0CF8;
        //! CONFIG_DATA レジスタのポートアドレス
        static constexpr uint16_t sk_ConfigData_Addr    = 0x0CFC;

        /**
         * @brief インスタンス取得
         */
        static Manager& Instance();

        /**
         * @brief CONFIG_ADDRESS へアドレス書き込み
         */
        void WriteAddress( uint32_t addr );
        /** 
         * @brief CONFIG_DATA へデータ書き込み
         */
        void WriteData( uint32_t data );
        /**
         * @brief CONFIG_DATA からデータ読み取り
         */
        uint32_t ReadData();
    
    private:

        static constexpr int sk_DeviceMax = 32;         //! PCIの認識できる最大デバイス数

        std::array<Device, sk_DeviceMax> m_Devices;     //! 認識したデバイス記録先
        int m_NumDevice;                                //! 認識したデバイス数
    };
}