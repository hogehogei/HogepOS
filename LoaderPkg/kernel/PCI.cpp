//
// include files
//
#include "PCI.hpp"
#include "asmfunc.h"

namespace pci
{
//
// constant
//

//
// static variables
//

//
// static function declaration
// 

//
// funcion definitions
// 

Manager& Manager::Instance()
{
    static Manager s_Instance;
    return s_Instance;
}

void Manager::WriteAddress( uint32_t addr )
{
    IoOut32( sk_ConfigAddress_Addr, addr );
}

void Manager::WriteData( uint32_t data )
{
    IoOut32( sk_ConfigData_Addr, data );
}

uint32_t Manager::ReadData()
{
    return IoIn32( sk_ConfigData_Addr );
}

} // namespace pci

