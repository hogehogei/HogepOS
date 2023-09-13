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

struct Context
{
    uint64_t BaseAddr;
};

Context* Initialize( pci::Device device );

template <typename T>
void RegWrite32( const Context& ctx, T value )
{
    volatile uint32_t* addr = reinterpret_cast<volatile uint32_t*>(ctx.BaseAddr + T::Addr);
    *addr = value.Data;
}

template <typename T>
uint32_t RegRead32( const Context& ctx )
{
    volatile uint32_t* addr = reinterpret_cast<volatile uint32_t*>(ctx.BaseAddr + T::Addr);
    return *addr;
}

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
    } __attribute__((packed)) Bits;
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
    } __attribute__((packed)) Bits;
} __attribute__((packed));

void EnableInterrupt( const Context& ctx );
uint32_t ReadInterrupt( const Context& ctx );
void DisableInterrupt( const Context& ctx );

}
}
}