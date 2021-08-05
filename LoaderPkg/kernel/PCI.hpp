#pragma once

// 
// include headers
//
#include <cstdint>

#include "Error.hpp"

namespace pci
{
    struct Device
    {
        uint8_t Bus;
        uint8_t Device;
        uint8_t Function;
        uint8_t HeaderType;
    };

    class ClassCode
    {
    public:
        ClassCode( uint32_t code )
            : m_Code( code )
        {}
        
        uint8_t Base() const { return (m_Code >> 24) & 0x0Fu; }
        uint8_t Sub() const { return (m_Code >> 16) & 0x0Fu; }

    private:
        uint32_t m_Code;
    };

    /**
     * @brief   PCIマネージャ
     *          PCIコンフィギュレーション空間への読み書きを管理する
     */
    class Manager
    {
    public:
        static constexpr int sk_DeviceMax = 32;         //! PCIの認識できる最大デバイス数
        using DeviceArray = Device[sk_DeviceMax];

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
         * @brief CONFIG ADDRESS 用の32ビット整数を生成する
         */
        uint32_t MakeAddress( uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr ) const;
        /**
         * @brief CONFIG_ADDRESS へアドレス書き込み
         */
        void WriteAddress( uint32_t addr ) const;
        /** 
         * @brief CONFIG_DATA へデータ書き込み
         */
        void WriteData( uint32_t data );
        /**
         * @brief CONFIG_DATA からデータ読み取り
         */
        uint32_t ReadData() const;
        /**
         * @brief ベンダーID読み取り
         */
        uint16_t ReadVendorID( uint8_t bus, uint8_t device, uint8_t function ) const;
        /**
         * @brief ヘッダータイプ読み取り
         */
        uint8_t ReadHeaderType( uint8_t bus, uint8_t device, uint8_t function ) const;
        /**
         * @brief バスナンバー読み取り
         */
        uint32_t ReadBusNumbers( uint8_t bus, uint8_t device, uint8_t function ) const;
        /**
         * @brief ヘッダータイプ読み取り
         */
        ClassCode ReadClassCode( uint8_t bus, uint8_t device, uint8_t function ) const;
        /**
         * @brief シングルファンクションデバイスか判定
         */
        bool IsSingleFunctionDevice( uint8_t header_type ) const;

        /**
         * @brief PCIバス全探索
         */
        Result ScanAllBus();

        const DeviceArray& GetDevices() const;
        int GetDeviceNum() const;

    private:

         /**
         * @brief デバイス追加
         */
        Result AddDevice( uint8_t bus, uint8_t device, uint8_t funciton, uint8_t header_type );
        /**
         * @brief バス探索
         */
        Result ScanBus( uint8_t bus );
        /**
         * @brief デバイス探索
         */
        Result ScanDevice( uint8_t bus, uint8_t device );
        /**
         * @brief ファンクション探索
         */
        Result ScanFunction( uint8_t bus, uint8_t device, uint8_t function );

        DeviceArray  m_Devices;            //! 認識したデバイス記録先
        int m_NumDevice;                   //! 認識したデバイス数
    };
}