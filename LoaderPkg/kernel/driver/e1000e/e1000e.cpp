// 
// include headers
//
#include <cstdint>
#include "e1000e.hpp"

namespace {
// 
// static variables
//
constexpr int k_ContextNum = 2;
driver::net::e1000e::Context g_Context[k_ContextNum];
int g_AvailableContextPtr = 0;

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

Context* Initialize( pci::Device device )
{
    if( g_AvailableContextPtr >= k_ContextNum ){
        return nullptr;
    } 

    Context ctx;
    if( !GetBaseAddress(device, ctx.BaseAddr) ){
        return nullptr;
    }
    
    g_Context[g_AvailableContextPtr] = ctx;
    Context* result = &g_Context[g_AvailableContextPtr];
    ++g_AvailableContextPtr;
    
    return result;
}

void EnableInterrupt( const Context& ctx )
{
    // とりあえず適当に設定
    IMS t;
    t.Data = 0x0000BEEF;
    RegWrite32<IMS>( ctx, t );
}

uint32_t ReadInterrupt( const Context& ctx )
{
    return RegRead32<IMS>( ctx );
}

void DisableInterrupt( const Context& ctx )
{
    // とりあえず適当に設定
    IMC t;
    t.Data = 0xFFFFFFFF;
    RegWrite32<IMC>( ctx, t );
}



}
}
}