#pragma once

// 
// include headers
//
#include <cstdint>
#include "../../asmfunc.h"
#include "../../PCI.hpp"
#include "../../logger.hpp"

namespace driver {
namespace net {
namespace e1000e {

constexpr std::size_t k_RxBufferSize = 2048;
constexpr std::size_t k_RxDescriptorNum = 2400;
constexpr std::size_t k_TxBufferSize = 2048;
constexpr std::size_t k_TxDescriptorNum = 16;

struct RxDescriptor
{
    uint64_t BufferAddr;
    uint16_t Length;
    uint16_t Chksum;
    uint8_t  Status;
    uint8_t  Errors;
    uint16_t Special;
}  __attribute__((packed));
// RxDescriptor ring size(byte) must be multple 128
static_assert( (sizeof(RxDescriptor) * k_RxDescriptorNum) % 128 == 0 );
constexpr uint8_t k_RDESC_STATUS_DD = 0x01;

struct TxDescriptor
{
    uint64_t BufferAddr;
    uint16_t Length;
    uint8_t  Cso;
    uint8_t  Cmd;
    struct {
        uint8_t Sta : 4;
        uint8_t Rsv : 4;
    }  __attribute__((packed));
    uint8_t  Css;
    uint16_t Special;
}  __attribute__((packed));
// TxDescriptor ring size(byte) must be multple 128
static_assert( (sizeof(TxDescriptor) * k_TxDescriptorNum) % 128 == 0 );

/**
 * @brief e1000e ドライバコンテキスト
 * 複数枚の同種NICが搭載できるように、NIC毎に使用する変数をまとめる。
 */
class Context
{
public:
    static Context* Initialize( pci::Device device );

    Context() = default;
    Context( const Context& ) = delete;
    Context& operator=( const Context& ) = delete;

    int32_t Recv( uint8_t* buffer, std::size_t buf_size );
    int32_t Send( uint8_t* buffer, std::size_t send_size );
    uint64_t BaseAddr() const { return m_BaseAddr; }

private:

    void EnableAutoNegotiation();
    void InitializeRx();
    void InitializeTx();
    void InitializeRxDescRing();
    void InitializeTxDescRing();

    uint64_t m_BaseAddr;              //! NICレジスタベースアドレス
    uint32_t m_CurrentRxRingIdx;      //! 現在の受信Ringのドライバ側受信処理済みインデックス
    uint32_t m_CurrentTxRingIdx;      //! 現在の送信Ringのドライバ側受信処理済みインデックス
    RxDescriptor* m_RxDescriptor;     //! 受信ディスクリプタRING
    uint8_t*      m_RxBuffer;         //! 受信パケットバッファ(RxBufferSize * RxDiscriptorNum)
    TxDescriptor* m_TxDescriptor;     //! 送信ディスクリプタRING
    uint8_t*      m_TxBuffer;         //! 送信パケットバッファ(TxBufferSize * TxDiscriptorNum)
};
constexpr std::size_t k_Ether_MTU = 1500;
constexpr std::size_t k_Ether_HeaderSize = 14;
constexpr std::size_t k_Ether_FCS = 4;

/**
 * @brief 32bitレジスタ書き込み
*/
template <typename T>
void RegWrite32( const Context& ctx, T value )
{
    volatile uint32_t* addr = reinterpret_cast<volatile uint32_t*>(ctx.BaseAddr() + T::Addr);
    *addr = value.Data;
}

/**
 * @brief 32bitレジスタ読み込み
*/
template <typename T>
uint32_t RegRead32( const Context& ctx )
{
    volatile uint32_t* addr = reinterpret_cast<volatile uint32_t*>(ctx.BaseAddr() + T::Addr);
    return *addr;
}

union CTRL {
    static constexpr uint32_t Addr = 0x00000000;
    uint32_t Data;
    struct {
        uint32_t FD : 1;
        uint32_t Reserved1 : 2;
        uint32_t LRST  : 1;
        uint32_t Reserved2 : 1;
        uint32_t ASDE : 1;
        uint32_t SLU : 1;
        uint32_t ILOS : 1;
        uint32_t SPEED : 2;
        uint32_t Reserved3 : 1;
        uint32_t FRCSPD : 1;
        uint32_t FRCDPLX : 1;
        uint32_t Reserved4 : 5;
        uint32_t SDP0_DATA : 1;
        uint32_t SDP1_DATA : 1;
        uint32_t ADVD3WUC : 1;
        uint32_t EN_PHY_PWR_MGMT : 1;
        uint32_t SDP0_IODIR : 1;
        uint32_t SDP1_IODIR : 1;
        uint32_t Reserved5 : 2;
        uint32_t RST : 1;
        uint32_t RFCE : 1;
        uint32_t TFCE : 1;
        uint32_t Reserved6 : 1;
        uint32_t VME : 1;
        uint32_t PHY_RST : 1;
    } __attribute__((packed));
} __attribute__((packed));

union STATUS {
    static constexpr uint32_t Addr = 0x00000008;
    uint32_t Data;
    struct {
        uint32_t FD : 1;
        uint32_t LU : 1;
        uint32_t FunctionID  : 2;
        uint32_t TXOFF : 1;
        uint32_t TBIMODE : 1;
        uint32_t SPEED : 2;
        uint32_t ASDV : 2;
        uint32_t Reserved1 : 1;
        uint32_t PCI66 : 1;
        uint32_t BUS64 : 1;
        uint32_t PCIX_MODE : 1;
        uint32_t PCIXSPD : 2;
        uint32_t Reserved2 : 16;
    } __attribute__((packed));
} __attribute__((packed));

union IMS {
    static constexpr uint32_t Addr = 0x000000D0;
    uint32_t Data;
    struct {
        uint32_t TXDW : 1;
        uint32_t TXQE : 1;
        uint32_t LSC  : 1;
        uint32_t Reserved1 : 1;
        uint32_t RXDMT0 : 1;
        uint32_t Reserved2 : 1;
        uint32_t RXO : 1;
        uint32_t RXT0 : 1;
        uint32_t Reserved3 : 1;
        uint32_t MDAC : 1;
        uint32_t Reserved4 : 5;
        uint32_t TXD_LOW : 1;
        uint32_t SRPD : 1;
        uint32_t ACK : 1;
        uint32_t MNG : 1;
        uint32_t Reserved5 : 1;
        uint32_t RxQ0 : 1;
        uint32_t RxQ1 : 1;
        uint32_t TxQ0 : 1;
        uint32_t TxQ1 : 1;
        uint32_t Other : 1;
        uint32_t Reserved6 : 7;
    } __attribute__((packed));
} __attribute__((packed));

union IMC {
    static constexpr uint32_t Addr = 0x000000D8;
    uint32_t Data;
    struct {
        uint32_t TXDW : 1;
        uint32_t TXQE : 1;
        uint32_t LSC  : 1;
        uint32_t Reserved1 : 1;
        uint32_t RXDMT0 : 1;
        uint32_t Reserved2 : 1;
        uint32_t RXO : 1;
        uint32_t RXT0 : 1;
        uint32_t Reserved3 : 1;
        uint32_t MDAC : 1;
        uint32_t Reserved4 : 5;
        uint32_t TXD_LOW : 1;
        uint32_t SRPD : 1;
        uint32_t ACK : 1;
        uint32_t MNG : 1;
        uint32_t Reserved5 : 1;
        uint32_t RxQ0 : 1;
        uint32_t RxQ1 : 1;
        uint32_t TxQ0 : 1;
        uint32_t TxQ1 : 1;
        uint32_t Other : 1;
        uint32_t Reserved6 : 7;
    } __attribute__((packed));
} __attribute__((packed));

template <std::size_t N>
union RAL
{
    static constexpr uint32_t Addr = 0x00005400 + N * 8;
    uint32_t Data;
    struct {
        uint32_t Mac1 : 8;
        uint32_t Mac2 : 8;
        uint32_t Mac3 : 8;
        uint32_t Mac4 : 8;
    } __attribute__((packed));
} __attribute__((packed));

template <std::size_t N>
union RAH
{
    static constexpr uint32_t Addr = 0x00005404 + N * 8;
    uint32_t Data;
    struct {
        uint32_t Mac5 : 8;
        uint32_t Mac6 : 8;
        uint32_t AS : 2;
        uint32_t Reserved : 13;
        uint32_t AV : 1;
    } __attribute__((packed));
} __attribute__((packed));

struct MTA
{
    static constexpr uint32_t Addr = 0x00005200;
    static constexpr uint32_t ResisterNum = 128;
    uint32_t Data;
};

union ITR
{
    static constexpr uint32_t Addr = 0x000000C4;
    uint32_t Data;
    struct {
        uint32_t Interval : 16;
        uint32_t Reserved : 16;
    } __attribute__((packed));
} __attribute__((packed));

struct RDBAL
{
    static constexpr uint32_t Addr = 0x00002800;
    uint32_t Data;
};

struct RDBAH
{
    static constexpr uint32_t Addr = 0x00002804;
    uint32_t Data;
};

struct RDLEN
{
    static constexpr uint32_t Addr = 0x00002808;
    uint32_t Data;
};

struct TDBAL
{
    static constexpr uint32_t Addr = 0x00003800;
    uint32_t Data;
};

struct TDBAH
{
    static constexpr uint32_t Addr = 0x00003804;
    uint32_t Data;
};

struct TDLEN
{
    static constexpr uint32_t Addr = 0x00003808;
    uint32_t Data;
};

union RDH
{
    static constexpr uint32_t Addr = 0x00002810;
    uint32_t Data;
    struct {
        uint32_t Head : 16;
        uint32_t Reserved : 16;
    } __attribute__((packed));
} __attribute__((packed));

union RDT
{
    static constexpr uint32_t Addr = 0x00002818;
    uint32_t Data;
    struct {
        uint32_t Tail : 16;
        uint32_t Reserved : 16;
    } __attribute__((packed));
} __attribute__((packed));

union TDH
{
    static constexpr uint32_t Addr = 0x00003810;
    uint32_t Data;
    struct {
        uint32_t Head : 16;
        uint32_t Reserved : 16;
    } __attribute__((packed));
} __attribute__((packed));

union TDT
{
    static constexpr uint32_t Addr = 0x00003818;
    uint32_t Data;
    struct {
        uint32_t Tail : 16;
        uint32_t Reserved : 16;
    } __attribute__((packed));
} __attribute__((packed));

union RCTL {
    static constexpr uint32_t Addr = 0x00000100;
    uint32_t Data;
    struct {
        uint32_t Reserved1 : 1;
        uint32_t EN : 1;
        uint32_t SBP  : 1;
        uint32_t UPE : 1;
        uint32_t MPE : 1;
        uint32_t LPE : 1;
        uint32_t LBM : 2;
        uint32_t RDMTS : 2;
        uint32_t Reserved2 : 2;
        uint32_t MO : 2;
        uint32_t Reserved3 : 1;
        uint32_t BAM : 1;
        uint32_t BSIZE : 2;
        uint32_t VFE : 1;
        uint32_t CFIEN : 1;
        uint32_t CFI : 1;
        uint32_t Reserved4 : 1;
        uint32_t DPF : 1;
        uint32_t PMCF : 1;
        uint32_t Reserved5 : 1;
        uint32_t BSEX : 1;
        uint32_t SECRC : 1;
        uint32_t Reserved6 : 5;
    } __attribute__((packed));
} __attribute__((packed));

union TCTL {
    static constexpr uint32_t Addr = 0x00000400;
    uint32_t Data;
    struct {
        uint32_t Reserved1 : 1;
        uint32_t EN : 1;
        uint32_t Reserved2  : 1;
        uint32_t PSP : 1;
        uint32_t CT : 8;
        uint32_t COLD : 10;
        uint32_t SWXOFF : 1;
        uint32_t Reserved3 : 1;
        uint32_t RTLC : 1;
        uint32_t NRTU : 1;
        uint32_t Reserved4 : 6;
    } __attribute__((packed));
} __attribute__((packed));
constexpr uint16_t k_TCTL_CollisionDistanceFullDuplex = 0x40;
constexpr uint8_t  k_TCTL_CollisionThreshold = 0x0F;

void EnableRxInterrupt( const Context& ctx );
uint32_t ReadInterrupt( const Context& ctx );
void DisableInterrupt( const Context& ctx );

}
}
}