#pragma once

// 
// include headers
//
#include <cstdint>

#include "PCI.hpp"

namespace pci
{
    class MSICapability
    {
    public:
        MSICapability( Device dev, uint8_t capablities_pointer );
        ~MSICapability();

        bool IsValid() const;
        uint8_t CapabilityID() const;
        uint8_t NextPointer() const;
        uint16_t MessageControl() const;
        uint32_t MessageAddress() const;
        uint32_t MessageUpperAddress() const;
        uint16_t MessageData() const;

        MSICapability NextCapability() const;

    private:

        Device  m_Device;
        uint8_t m_Pointer;
    };
}