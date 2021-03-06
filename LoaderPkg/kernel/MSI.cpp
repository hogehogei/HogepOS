//
// include files
//
#include "MSI.hpp"
#include "asmfunc.h"
#include "logger.hpp"

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

MSICapability::MSICapability( pci::Device dev )
    : m_Device( dev ),
      m_CapabilitiesPointer( 0 )
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, k_CapabilitiesFirstPtr) );
    m_CapabilitiesPointer = instance.ReadData() & 0xFFu;
}

MSICapability::MSICapability( pci::Device dev, uint8_t next_capability_pointer )
    : m_Device( dev ),
      m_CapabilitiesPointer( next_capability_pointer )
{}

MSICapability::~MSICapability()
{}

uint8_t MSICapability::CapabilitiesPointer() const
{
    return m_CapabilitiesPointer;
}

const pci::Device& MSICapability::Device() const
{
    return m_Device;
}

uint8_t MSICapability::CapabilityID() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer) );
    return instance.ReadData() & 0xFFu;
}

uint8_t MSICapability::NextPointer() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer) );
    Log( kDebug, "NextPointer: (%02x)\n", (instance.ReadData() >> 8) & 0xFFu );
    return (instance.ReadData() >> 8) & 0xFFu;
}

bool MSICapability::IsLastEntry() const
{
    return NextPointer() == 0;
}

pci::MessageControl MSICapability::MessageControl() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer) );
    return pci::MessageControl( (instance.ReadData() >> 16) & 0xFFFFu );
}

void MSICapability::MessageControl( const pci::MessageControl& msgctrl )
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer) );
    
    uint32_t header = instance.ReadData() & 0xFFFFu;
    header |= static_cast<uint32_t>(msgctrl.Data) << 16;

    instance.WriteData( header );
}

pci::MessageAddress MSICapability::MessageAddress() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer + 0x04) );
    return instance.ReadData();
}

void MSICapability::MessageAddress( const pci::MessageAddress& msgaddr )
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer + 0x04) );

    instance.WriteData( msgaddr.Data );
}

uint32_t MSICapability::MessageUpperAddress() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer + 0x08) );
    return instance.ReadData();
}

void MSICapability::MessageUpperAddress( const uint32_t msgaddr )
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer + 0x08) );
    instance.WriteData( msgaddr );
}

pci::MessageData MSICapability::MessageData() const
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    
    pci::MessageControl msgctrl = MessageControl();
    uint8_t msgdata_addr = 8;
    if( msgctrl.Bits.Addr64_Capable == 1 ){
        msgdata_addr = 12;
    }

    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer + msgdata_addr) );
    return instance.ReadData();
}

void MSICapability::MessageData( const pci::MessageData& msgdata )
{
    ConfigurationArea& instance = ConfigurationArea::Instance();

    pci::MessageControl msgctrl = MessageControl();
    uint8_t msgdata_addr = 8;
    if( msgctrl.Bits.Addr64_Capable == 1 ){
        msgdata_addr = 12;
    }

    instance.WriteAddress( instance.MakeAddress(m_Device, m_CapabilitiesPointer + msgdata_addr) );

    instance.WriteData( msgdata.Data );
}

MSICapability MSICapability::NextCapability() const
{
    return MSICapability( m_Device, NextPointer() );
}

Error ConfigureMSIFixedDestination(
        const Device& dev, 
        uint8_t apic_id,
        MSITriggerMode trigger_mode, 
        MSIDeliveryMode delivery_mode,
        uint8_t vector, 
        uint8_t num_vector_exponent )
{
    MSICapability cap( dev );

    while( !cap.IsLastEntry() ){
        cap = cap.NextCapability();
    }

    if( cap.CapabilityID() == k_CapabilityID_MSIX ){
        MAKE_ERROR( Error::kNotImplemented );
    }

    MessageControl msgctrl( cap.MessageControl() );
    msgctrl.Bits.Enable = 1;
    msgctrl.Bits.MultipleMsgEnable = std::min<uint8_t>( msgctrl.Bits.MultipleMsgCapable, num_vector_exponent );
    msgctrl.Bits.VectorMaskingCapable = 1;

    MessageAddress msgaddr( 0xFEE00000 );
    msgaddr.Bits.DestinationID   = apic_id;
    msgaddr.Bits.DestinationMode = 0;
    msgaddr.Bits.RedirectionHint = 0;
    
    MessageData msgdata = {0};
    if (trigger_mode == MSITriggerMode::k_Level) {
        msgdata.Bits.Level = 1;
        msgdata.Bits.TriggerMode = 1;
    }
    msgdata.Bits.DeliveryMode = static_cast<uint32_t>(delivery_mode);
    msgdata.Bits.Vector       = vector;

    cap.MessageControl( msgctrl );
    cap.MessageAddress( msgaddr );
    cap.MessageUpperAddress( 0 );
    cap.MessageData( msgdata );

    ShowMSIRegister( cap );

    return MAKE_ERROR( Error::kSuccess );
}

void ShowMSIRegister( const MSICapability& cap )
{
    ConfigurationArea& instance = ConfigurationArea::Instance();
    uint8_t capablities_pointer = cap.CapabilitiesPointer();
    
    Log( kDebug, "MSI capabilities pointer: (%x)\n", capablities_pointer );
    for( int i = 0; i < 4; ++i ){
        instance.WriteAddress( instance.MakeAddress(cap.Device(), capablities_pointer + (i*4)) );
        Log( kDebug, "bit(%02d - %02d): %08X\n", i * 32, i * 32 + 31, instance.ReadData() );
    }
}

} // namespace pci

