//
// include files
//
#include <cstdint>
#include <array>

#include "Paging.hpp"
#include "asmfunc.h"

//
// constant
//

//
// static variables
//
static const uint64_t k_PageSize4K = 4096;
static const uint64_t k_PageSize2M = k_PageSize4K * 512;
static const uint64_t k_PageSize1G = k_PageSize2M * 512;

alignas(k_PageSize4K) std::array<uint64_t, 512> g_PML4_Table;
alignas(k_PageSize4K) std::array<uint64_t, 512> g_PDP_Table;
alignas(k_PageSize4K) 
    std::array<std::array<uint64_t, 512>, k_PageDirectoryCount> g_PageDirectory;


//
// static function declaration
// 

//
// funcion definitions
// 
void SetupIdentityPageTable()
{
    g_PML4_Table[0] = reinterpret_cast<uint64_t>(&g_PDP_Table[0]) | 0x003;

    for( int pdptbl_idx = 0; pdptbl_idx < g_PageDirectory.size(); ++pdptbl_idx ){
        g_PDP_Table[pdptbl_idx] = reinterpret_cast<uint64_t>(&g_PageDirectory[pdptbl_idx]) | 0x003;

        for( int pd_idx = 0; pd_idx < 512; ++pd_idx ){
            g_PageDirectory[pdptbl_idx][pd_idx] = pdptbl_idx * k_PageSize1G + pd_idx * k_PageSize2M | 0x083;
        }
    }

    SetCR3( reinterpret_cast<uint64_t>(&g_PML4_Table[0]) );
}