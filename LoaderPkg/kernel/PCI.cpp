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

uint32_t Manager::MakeAddress( uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr ) const
{
    auto shl = []( uint32_t x, unsigned int bits ){
        return x << bits;
    };

    return shl(1, 31) // enable bit
        |  shl(bus, 16)
        |  shl(device   & 0x1Fu, 11)
        |  shl(function & 0x03u, 8)
        |     (reg_addr & 0xFCu); 
}

void Manager::WriteAddress( uint32_t addr ) const
{
    IoOut32( sk_ConfigAddress_Addr, addr );
}

void Manager::WriteData( uint32_t data )
{
    IoOut32( sk_ConfigData_Addr, data );
}

uint32_t Manager::ReadData() const
{
    return IoIn32( sk_ConfigData_Addr );
}

uint16_t Manager::ReadVendorID( uint8_t bus, uint8_t device, uint8_t function ) const
{
    WriteAddress( MakeAddress(bus, device, function, 0x00) );
    return ReadData() & 0xFFFFu;
}

uint8_t Manager::ReadHeaderType( uint8_t bus, uint8_t device, uint8_t function ) const
{
    WriteAddress( MakeAddress(bus, device, function, 0x0C) );
    return (ReadData() >> 16) & 0xFFu;
}

uint32_t Manager::ReadBusNumbers( uint8_t bus, uint8_t device, uint8_t function ) const
{
    WriteAddress( MakeAddress(bus, device, function, 0x18) );
    return ReadData();
}

ClassCode Manager::ReadClassCode( uint8_t bus, uint8_t device, uint8_t function ) const
{
    WriteAddress( MakeAddress(bus, device, function, 0x08) );
    return ClassCode( ReadData() );
}

bool Manager::IsSingleFunctionDevice( uint8_t header_type ) const
{
    return (header_type & 0x80u) == 0;
}

Result Manager::ScanAllBus()
{
    m_NumDevice = 0;

    auto header_type = ReadHeaderType( 0, 0, 0 );
    if( IsSingleFunctionDevice(header_type) ){
        return ScanBus(0);
    }

    for( uint8_t function = 1; function < 8; ++function ){
        if( ReadVendorID(0, 0, function) == 0xFFFFu ){
            continue;
        }

        auto err = ScanBus(function);
        if( err ){
            return err;
        }
    }

    return Result::Success;
}

const Manager::DeviceArray& Manager::GetDevices() const
{
    return m_Devices;
}

int Manager::GetDeviceNum() const
{
    return m_NumDevice;
}

Result Manager::ScanBus( uint8_t bus )
{
    for( uint8_t device = 0; device < 32; ++device ){
        // function id 0 の vendorID が 0xFFFF なら、有効なデバイスが存在しないのでスキップ
        if( ReadVendorID( bus, device, 0 ) == 0xFFFFu ){
            continue;
        }

        auto err = ScanDevice( bus, device );
        if( err ){
            return err;
        }
    }

    return Result::Success;
}

Result Manager::ScanDevice( uint8_t bus, uint8_t device )
{
    for( uint8_t function = 0; function < 8; ++function ){
        if( ReadVendorID( bus, device, function ) == 0xFFFFu ){
            continue;
        }

        auto err = ScanFunction( bus, device, function );
        if( err ){
            return err;
        }
    }

    return Result::Success;
}

Result Manager::ScanFunction( uint8_t bus, uint8_t device, uint8_t function )
{
    auto header_type = ReadHeaderType( bus, device, function );
    auto err = AddDevice( bus, device, function, header_type );
    if( err ){
        return err;
    }

    auto classcode = ReadClassCode( bus, device, function );
    if( classcode.Base() == 0x06u & classcode.Sub() == 0x04u ){
        // standard PCI-PCI bridge
        auto bus_numbers = ReadBusNumbers( bus, device, function );
        uint8_t secondary_bus = (bus_numbers >> 8) & 0xFFu;
        return ScanBus(secondary_bus);
    }

    return Result::Success;
}

Result Manager::AddDevice( uint8_t bus, uint8_t device, uint8_t function, uint8_t header_type )
{
    if( m_NumDevice >= sk_DeviceMax ){
        return Result::Full;
    }

    m_Devices[m_NumDevice] = Device { bus, device, function, header_type };
    ++m_NumDevice;

    return Result::Success;
}

} // namespace pci

