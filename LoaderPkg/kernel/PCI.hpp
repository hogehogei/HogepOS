#pragma once

// 
// include headers
//
#include <cstdint>

#include "error.hpp"

namespace pci
{
    static constexpr int sk_DeviceMax = 32;         //! PCIの認識できる最大デバイス数

    class ClassCode
    {
    public:
        ClassCode( uint32_t code )
            : m_Code( code )
        {}
        ClassCode() 
            : m_Code( 0 )
        {}

        bool Match( uint8_t base, uint8_t sub, uint8_t interface ) const
        {
            return (base == Base()) && (sub == Sub()) && (interface == Interface());
        }
        
        uint8_t Base() const { return (m_Code >> 24) & 0xFFu; }
        uint8_t Sub() const { return (m_Code >> 16) & 0xFFu; }
        uint8_t Interface() const { return (m_Code >> 8) & 0xFFu; }

    private:
        uint32_t m_Code;
    };

    struct Device
    {
        uint8_t Bus;
        uint8_t Device;
        uint8_t Function;
        uint8_t HeaderType;
        ClassCode ClassCode;
    };



    /**
     * @brief   PCIマネージャ
     *          PCIコンフィギュレーション空間への読み書きを管理する
     */
    class ConfigurationArea
    {
    public:

        using DeviceArray = Device[sk_DeviceMax];

    public:
        //! CONFIG_ADDRESS レジスタのポートアドレス
        static constexpr uint16_t sk_ConfigAddress_Addr = 0x0CF8;
        //! CONFIG_DATA レジスタのポートアドレス
        static constexpr uint16_t sk_ConfigData_Addr    = 0x0CFC;

        ConfigurationArea();
        ~ConfigurationArea();

        /**
         * @brief インスタンス取得
         */
        static ConfigurationArea& Instance();

        /**
         * @brief CONFIG ADDRESS 用の32ビット整数を生成する
         */
        uint32_t MakeAddress( uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr ) const;
        uint32_t MakeAddress( const Device& device, uint8_t reg_addr ) const;
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
         * @brief コンフィギュレーション空間の指定アドレスを読み出す
         **/
        uint32_t ReadConfReg( const Device& device, uint8_t addr ) const;

        /**
         * @brief コンフィギュレーション空間の指定アドレスへ書き込む
         **/
        void WriteConfReg( const Device& device, uint8_t addr, uint32_t value );
        
        /**
         * @brief ベンダーID読み取り
         */
        uint16_t ReadVendorID( uint8_t bus, uint8_t device, uint8_t function ) const;
        uint16_t ReadVendorID( const Device& device ) const;
        /**
         * @brief ヘッダータイプ読み取り
         */
        uint8_t ReadHeaderType( uint8_t bus, uint8_t device, uint8_t function ) const;
        /**
         * @brief バスナンバー読み取り
         */
        uint32_t ReadBusNumbers( uint8_t bus, uint8_t device, uint8_t function ) const;
        /**
         * @brief クラスコード読み取り
         */
        ClassCode ReadClassCode( uint8_t bus, uint8_t device, uint8_t function ) const;
        /**
         * @brief Capabilities Pointer読み取り
         */
        uint8_t CapablitiesPointer( uint8_t bus, uint8_t device, uint8_t funciton ) const;
        /**
         * @brief Read BAR 64bit
         */
        WithError<uint64_t> ReadBAR( uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_num ) const;
        WithError<uint64_t> ReadBAR( const Device& device, uint8_t bar_num ) const;

        /**
         * @brief シングルファンクションデバイスか判定
         */
        bool IsSingleFunctionDevice( uint8_t header_type ) const;

        /**
         * @brief PCIバス全探索
         */
        Error::Code ScanAllBus();

        const DeviceArray& GetDevices() const;
        int GetDeviceNum() const;

    private:

         /**
         * @brief デバイス追加
         */
        Error::Code AddDevice( uint8_t bus, uint8_t device, uint8_t funciton, uint8_t header_type );
        /**
         * @brief バス探索
         */
        Error::Code ScanBus( uint8_t bus );
        /**
         * @brief デバイス探索
         */
        Error::Code ScanDevice( uint8_t bus, uint8_t device );
        /**
         * @brief ファンクション探索
         */
        Error::Code ScanFunction( uint8_t bus, uint8_t device, uint8_t function );

        DeviceArray  m_Devices;            //! 認識したデバイス記録先
        int m_NumDevice;                   //! 認識したデバイス数
    };
}