#pragma once

// 
// include headers
//
#include <cstdint>

#include "PCI.hpp"

namespace pci
{
    constexpr uint8_t k_CapabilitiesFirstPtr = 0x34;
    constexpr uint8_t k_CapabilityID_MSI = 5;
    constexpr uint8_t k_CapabilityID_MSIX = 17;

    enum class MSITriggerMode {
        k_Edge = 0,
        k_Level = 1
    };

    enum class MSIDeliveryMode {
        k_Fixed          = 0b000,
        k_LowestPriority = 0b001,
        k_SMI            = 0b010,
        k_NMI            = 0b100,
        k_INIT           = 0b101,
        k_ExtINT         = 0b111,
    };

    union MessageControl {
        MessageControl( uint16_t data )
         : Data(data) {}

        uint16_t Data;
        struct {
            uint16_t Enable : 1;
            uint16_t MultipleMsgCapable : 3;
            uint16_t MultipleMsgEnable  : 3;
            uint16_t Addr64_Capable : 1;
            uint16_t VectorMaskingCapable : 1;
            uint16_t : 7;
        } __attribute__((packed)) Bits;
    } __attribute__((packed));

    union MessageAddress {
        MessageAddress( uint32_t data )
         : Data(data) {}

        uint32_t Data;
        struct {
            uint32_t : 2;
            uint32_t DestinationMode : 1;
            uint32_t RedirectionHint : 1;
            uint32_t : 8;
            uint32_t DestinationID : 8;
            uint32_t : 12;
        } __attribute__((packed)) Bits;
    } __attribute__((packed));

    union MessageData {
        MessageData( uint32_t data )
         : Data(data) {}

        uint32_t Data;
        struct {
            uint32_t Vector : 8;
            uint32_t DeliveryMode : 3;
            uint32_t : 3;
            uint32_t Level : 1;
            uint32_t TriggerMode : 1;
            uint32_t : 16;
        } __attribute__((packed)) Bits;
    } __attribute__((packed));

    class MSICapability
    {
    public:
        MSICapability( Device dev );
        MSICapability( Device dev, uint8_t next_capability_pointer );
        ~MSICapability();

        uint8_t CapabilitiesPointer() const;
        const pci::Device& Device() const;

        uint8_t CapabilityID() const;
        uint8_t NextPointer() const;
        bool IsLastEntry() const;
        pci::MessageControl ReadMessageControl() const;
        void WriteMessageControl( const pci::MessageControl& msgctrl );
        pci::MessageAddress ReadMessageAddress() const;
        void WriteMessageAddress( const pci::MessageAddress& msgaddr );
        uint32_t ReadMessageUpperAddress() const;
        void WriteMessageUpperAddress( const uint32_t msgaddr );
        pci::MessageData ReadMessageData() const;
        void WriteMessageData( const pci::MessageData& msgdata );

        MSICapability NextCapability() const;

    private:

        pci::Device  m_Device;
        uint8_t      m_CapabilitiesPointer;
    };

    Error ConfigureMSIFixedDestination(
        const Device& dev, 
        uint8_t apic_id,
        MSITriggerMode trigger_mode, 
        MSIDeliveryMode delivery_mode,
        uint8_t vector, 
        uint8_t num_vector_exponent );

    void ShowMSIConfigurationSpace( const Device& dev );
    void ShowMSIRegister( const MSICapability& cap );
}