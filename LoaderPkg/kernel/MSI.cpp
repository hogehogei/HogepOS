//
// include files
//
#include "MSI.hpp"
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

MSICapability::MSICapability( Device dev, uint8_t capablities_pointer )
    : m_Device( dev ),
      m_Pointer( capablities_pointer )
{}

MSICapability::~MSICapability()
{}

bool MSICapability::IsValid() const
{
    return m_Pointer != 0;
}

uint8_t MSICapability::CapabilityID() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_Pointer) );
    return instance.ReadData() & 0xFFu;
}

uint8_t MSICapability::NextPointer() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_Pointer) );
    return (instance.ReadData() >> 8) & 0xFFu;
}

uint16_t MSICapability::MessageControl() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_Pointer) );
    return (instance.ReadData() >> 16) & 0xFFFFu;
}

uint32_t MSICapability::MessageAddress() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_Pointer + 0x04) );
    return instance.ReadData();
}

uint32_t MSICapability::MessageUpperAddress() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_Pointer + 0x08) );
    return instance.ReadData();
}

uint16_t MSICapability::MessageData() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_Pointer + 0x0C) );
    return instance.ReadData() & 0xFFFFu;
}

MSICapability MSICapability::NextCapability() const
{
    return MSICapability( m_Device, NextPointer() );
}

} // namespace pci

