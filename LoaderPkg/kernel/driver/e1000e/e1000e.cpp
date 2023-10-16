// 
// include headers
//
#include <cstdint>
#include <algorithm>
#include "e1000e.hpp"
#include "Global.hpp"
#include "MSI.hpp"
#include "PCI.hpp"

namespace {
// 
// static variables
//
alignas(16) driver::net::e1000e::RxDescriptor g_RxDescriptor[driver::net::e1000e::k_RxDescriptorNum];
alignas(16) uint8_t g_RxBuffer[driver::net::e1000e::k_RxBufferSize * driver::net::e1000e::k_RxDescriptorNum];

alignas(16) driver::net::e1000e::TxDescriptor g_TxDescriptor[driver::net::e1000e::k_TxDescriptorNum];
alignas(16) uint8_t g_TxBuffer[driver::net::e1000e::k_TxBufferSize * driver::net::e1000e::k_TxDescriptorNum];

//
// static functions
//
bool GetBaseAddress( const pci::Device& device, uint64_t& addr )
{
    uint64_t baseaddr = 0;

    // PCIのBARを読み出し、ベースアドレスを取得
    pci::DumpBARArray bar_array;
    size_t bar_dump_len = 0;
    pci::ConfigurationArea& pciconf = pci::ConfigurationArea::Instance();
    if( pciconf.DumpBAR(device, bar_array, bar_dump_len) ){
        for( size_t i = 0; i < bar_dump_len; ++i ){
            const auto& bar = bar_array[i];

            if( bar.AddrType == pci::BAR::ADDR_TYPE_MEMORY ){
                if( bar.Addr != 0 ){
                    baseaddr = bar.Addr;
                    break;
                }
            }
        }
    }
    else {
        return false;
    }

    addr = baseaddr;

    return baseaddr != 0;
}

}

namespace driver {
namespace net {
namespace e1000e {

Context* Context::Initialize( pci::Device device )
{
    Context* ctx = new Context();
    if( ctx == nullptr ){
        return nullptr;
    }
    if( !GetBaseAddress(device, ctx->m_BaseAddr) ){
        return nullptr;
    }

    Log( kInfo, "e1000e base address: %016lX\n", ctx->m_BaseAddr );

    // TODO: 複数同じイーサネットアダプタが存在する場合を考慮していない。
    // MSI割り込み設定
    Log( kInfo, "Setting ethernet msi interrupts.\n" );
    const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;
    Error err = pci::ConfigureMSIFixedDestination(
        device, bsp_local_apic_id,
        pci::MSITriggerMode::k_Level, pci::MSIDeliveryMode::k_Fixed,
        InterruptVector::kE1000E, 0
    );
    if( err ){
        return nullptr;
    }

    ctx->m_Device = device;

    DisableInterrupt(*ctx);
    ctx->EnableAutoNegotiation();   // AutoNegotiation 有効化

    CTRL ctrl { RegRead32<CTRL>(*ctx) };
    ctrl.Reserved2 = 1;             // set to 1b, according to datasheet
    ctrl.RST = 0;                   // link reset = Normal
    ctrl.PHY_RST = 0;               // PHY reset = Normal
    RegWrite32<CTRL>( *ctx, ctrl );

    ctx->InitializeRx();            // 受信処理初期化
    ctx->InitializeTx();            // 送信処理初期化

    // IMS 受信割り込み設定
    EnableRxInterrupt( *ctx );

    return ctx;
}

int32_t Context::Recv( uint8_t* buffer, std::size_t buf_size )
{
    RxDescriptor *currRxDesc = &m_RxDescriptor[m_CurrentRxRingIdx];

    // 受信完了したか？
    if( (currRxDesc->Status & k_RDESC_STATUS_DD) != k_RDESC_STATUS_DD ){
        return 0;
    }

    std::size_t rxlen = std::min<std::size_t>( buf_size, currRxDesc->Length );
    if( rxlen <= 0 ){
        return 0;
    }

    // 受信バッファから引数のバッファにコピー
    memcpy( buffer, reinterpret_cast<void*>(currRxDesc->BufferAddr), rxlen );
    currRxDesc->Status = 0;
    
    // RDTを次に進める
    RDT rdt { m_CurrentRxRingIdx };
    RegWrite32<RDT>( *this, rdt );
    m_CurrentRxRingIdx = (m_CurrentRxRingIdx + 1) % k_RxDescriptorNum;

    return rxlen;
}

int32_t Context::Send( uint8_t* buffer, std::size_t send_size )
{
    std::size_t next = (m_CurrentTxRingIdx + 1) % k_TxDescriptorNum;
    volatile TxDescriptor *currTxDesc = &m_TxDescriptor[m_CurrentTxRingIdx];
    volatile TxDescriptor *nextTxDesc = &m_TxDescriptor[next];
    // 次のTxDescriptorが送信中の場合は、送信完了を待つ
    while( nextTxDesc->Sta == 0 ) ;

    std::size_t txlen = std::min<std::size_t>( send_size, (k_Ether_MTU + k_Ether_HeaderSize + k_Ether_FCS) );
    if( txlen <= 0 ){
        return 0;
    }

    uint8_t* tx_buffer = &m_TxBuffer[m_CurrentTxRingIdx * k_TxBufferSize];
    memcpy( tx_buffer, reinterpret_cast<void*>(buffer), txlen );

    currTxDesc->BufferAddr = reinterpret_cast<uint64_t>(tx_buffer);
    currTxDesc->Length = txlen;
    currTxDesc->Sta = 0;

    // TDTを次に進める
    m_CurrentTxRingIdx = next;
    TDT tdt { m_CurrentTxRingIdx };
    RegWrite32<TDT>( *this, tdt );

    return txlen;
}

void Context::EnableAutoNegotiation()
{
    CTRL ctrl { RegRead32<CTRL>(*this) };
    ctrl.FRCSPD = 0;    // Force speed disable, PHY or ASD to set the eth controller speed
    ctrl.FRCDPLX = 0;   // Force duplex disable,  PHY or ASD to set the eth controller speed
    ctrl.ASDE = 0;      // Auto-Speed detection enable, must be set 0 in 82574
    ctrl.SLU = 1;       // Set link up
    RegWrite32<CTRL>( *this, ctrl );

    // ctrl.RFCE, ctrl.TFCE は flow control resolution が完了してからMiiレジスタから読み取り設定する?
    // とりあえず起動時は初期値として設定しない
}

void Context::InitializeRx()
{
    // 受信用MACアドレスを設定
    // 3b:4d:3a:1d:b4:b5
    RAL<0> ral { 0x3A1DB4B5 };
    RegWrite32<RAL<0>>( *this, ral );
    RAH<0> rah { 0x00003B4D };
    // address valid : When set, the address is valid and is compared against the incoming packet.
    rah.AV = 1;
    RegWrite32<RAH<0>>( *this, rah );

    // Multicast Table Array初期化
    for( int i = 0; i < MTA::ResisterNum; ++i ){
        volatile uint32_t* addr = reinterpret_cast<volatile uint32_t*>(MTA::Addr + i*4);
        *addr = 0;
    }

    // ITR設定
    ITR itr { 0x00000000 };
    itr.Interval = 1000;
    RegWrite32<ITR>( *this, itr );

    // Receive Descriptor 設定(16byte align)
    InitializeRxDescRing();
    uint64_t addr = reinterpret_cast<uint64_t>(&m_RxDescriptor[0]);
    RDBAL rdbal { static_cast<uint32_t>(addr & 0x00000000FFFFFFFFull) };
    RDBAH rdbah { static_cast<uint32_t>(addr >> 32) };
    RDLEN rdlen { sizeof(RxDescriptor) * k_RxDescriptorNum };
    RegWrite32<RDBAL>( *this, rdbal );
    RegWrite32<RDBAH>( *this, rdbah );
    RegWrite32<RDLEN>( *this, rdlen );
    // RDH, RDT設定
    m_CurrentRxRingIdx = 0;
    RDH rdh { m_CurrentRxRingIdx };
    RDT rdt { k_RxDescriptorNum - 1 };
    RegWrite32<RDH>( *this, rdh );
    RegWrite32<RDT>( *this, rdt );

    // RCTL設定、受信有効化
    RCTL rctl { 0 };
    rctl.SBP = 1;           // Store bad packets
    rctl.UPE = 1;           // Unicast promiscuous enable, not filtered
    rctl.MPE = 1;           // Multicast promiscuous enable, not filtered
    rctl.DTYP = 0;          // legacy descriptor type
    rctl.BAM = 1;           // Broadcast Accept mode, accept broadcast packets
    rctl.BSIZE = 0;         // Receive buffer size = 2048bytes(00b)
    rctl.EN = 1;            // Receiver Enable
    RegWrite32<RCTL>( *this, rctl );
}

void Context::InitializeTx()
{
    // Transmitter Descriptor 設定(16byte align)
    InitializeTxDescRing();
    uint64_t addr = reinterpret_cast<uint64_t>(&m_TxDescriptor[0]);
    TDBAL tdbal { static_cast<uint32_t>(addr & 0x00000000FFFFFFFFull) };
    TDBAH tdbah { static_cast<uint32_t>(addr >> 32) };
    TDLEN tdlen { sizeof(TxDescriptor) * k_TxDescriptorNum };
    RegWrite32<TDBAL>( *this, tdbal );
    RegWrite32<TDBAH>( *this, tdbah );
    RegWrite32<TDLEN>( *this, tdlen );
    // TDH, TDT設定
    m_CurrentRxRingIdx = 0;
    TDH tdh { m_CurrentTxRingIdx };
    TDT tdt { k_TxDescriptorNum - 1 };
    RegWrite32<TDH>( *this, tdh );
    RegWrite32<TDT>( *this, tdt );

    // TCTL設定、送信有効化
    TCTL tctl { 0 };
    tctl.COLD = k_TCTL_CollisionDistanceFullDuplex;       // Collsion distance, datasheet reference value on full-duplex.
    tctl.CT = k_TCTL_CollisionThreshold;                  // Collsion threshold,  datasheet reference value on full-duplex.
    tctl.PSP = 1;                                         // Pad short packet enable
    tctl.EN = 1;                                          // Transmit Enable
    RegWrite32<TCTL>( *this, tctl );
}

void Context::InitializeRxDescRing()
{
    // メモリ確保
    // todo : static 変数からではなく new して確保すること
    m_RxDescriptor = &g_RxDescriptor[0];
    m_RxBuffer = &g_RxBuffer[0];
    for( std::size_t i = 0; i < k_RxDescriptorNum; ++i ){
        uint64_t bufhead =  k_RxBufferSize * i;
        m_RxDescriptor[i].BufferAddr = reinterpret_cast<uint64_t>(&m_RxBuffer[bufhead]);
        m_RxDescriptor[i].Status = 0;
        m_RxDescriptor[i].Errors = 0;
    }
}

void Context::InitializeTxDescRing()
{
    // メモリ確保
    // todo : static 変数からではなく new して確保すること
    m_TxDescriptor = &g_TxDescriptor[0];
    m_TxBuffer = &g_TxBuffer[0];
    for( std::size_t i = 0; i < k_TxDescriptorNum; ++i ){
        m_TxDescriptor[i].BufferAddr = 0;
        m_TxDescriptor[i].Length = 0;
        m_TxDescriptor[i].Cso = 0;
        m_TxDescriptor[i].Cmd = 0;
        m_TxDescriptor[i].Sta = 0;
        m_TxDescriptor[i].Rsv = 0;
        m_TxDescriptor[i].Css = 0;
        m_TxDescriptor[i].Special = 0;
    }
}

void EnableRxInterrupt( const Context& ctx )
{
    // clear ICR
    ICR icr_w = {0xFFFFFFFF};
    RegWrite32<ICR>( *g_e1000e_Ctx, icr_w );

    IMS ims { RegRead32<IMS>(ctx) };
    ims.RXT0 = 1;
    ims.RXO = 1;
    ims.RXDMT0 = 1;
    ims.LSC = 1;
    RegWrite32<IMS>( ctx, ims );
}

uint32_t ReadInterrupt( const Context& ctx )
{
    return RegRead32<IMS>( ctx );
}

void DisableInterrupt( const Context& ctx )
{
    IMC t;
    t.Data = 0xFFFFFFFF;
    RegWrite32<IMC>( ctx, t );
}

void InterruptHandler()
{
    // TODO: 複数同じイーサネットアダプタが存在する場合を考慮していない。
    if( g_e1000e_Ctx == nullptr ){
        return;
    }
    uint32_t icr = RegRead32<ICR>(*g_e1000e_Ctx);

    ICR icr_w = {0xFFFFFFFF};
    RegWrite32<ICR>( *g_e1000e_Ctx, icr_w );

    ++g_e1000eRxIntCnt;
}

}
}
}