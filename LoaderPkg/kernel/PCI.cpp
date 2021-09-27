//
// include files
//
#include "PCI.hpp"
#include "Global.hpp"
#include "asmfunc.h"
#include "error.hpp"
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
static void SearchIntelxHC();

//
// funcion definitions
// 

ConfigurationArea::ConfigurationArea()
    : m_Devices {},
      m_NumDevice( 0 )
{}

ConfigurationArea::~ConfigurationArea()
{}

ConfigurationArea& ConfigurationArea::Instance()
{
    static ConfigurationArea s_Instance;
    return s_Instance;
}

uint32_t ConfigurationArea::MakeAddress( uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr ) const
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

uint32_t ConfigurationArea::MakeAddress( const Device& device, uint8_t reg_addr ) const
{
    return MakeAddress( device.Bus, device.Device, device.Function, reg_addr );
}

void ConfigurationArea::WriteAddress( uint32_t addr ) const
{
    IoOut32( sk_ConfigAddress_Addr, addr );
}

void ConfigurationArea::WriteData( uint32_t data )
{
    IoOut32( sk_ConfigData_Addr, data );
}

uint32_t ConfigurationArea::ReadData() const
{
    return IoIn32( sk_ConfigData_Addr );
}

uint32_t ConfigurationArea::ReadConfReg( const Device& device, uint8_t addr ) const
{
    WriteAddress( MakeAddress(device.Bus, device.Device, device.Function, addr) );
    return ReadData();
}

void ConfigurationArea::WriteConfReg( const Device& device, uint8_t addr, uint32_t value )
{
    WriteAddress( MakeAddress(device.Bus, device.Device, device.Function, addr) );
    WriteData( value );
}

uint16_t ConfigurationArea::ReadVendorID( uint8_t bus, uint8_t device, uint8_t function ) const
{
    WriteAddress( MakeAddress(bus, device, function, 0x00) );
    return ReadData() & 0xFFFFu;
}

uint16_t ConfigurationArea::ReadVendorID( const Device& device ) const
{
    return ReadVendorID( device.Bus, device.Device, device.Function );
}

uint8_t ConfigurationArea::ReadHeaderType( uint8_t bus, uint8_t device, uint8_t function ) const
{
    WriteAddress( MakeAddress(bus, device, function, 0x0C) );
    return (ReadData() >> 16) & 0xFFu;
}

uint32_t ConfigurationArea::ReadBusNumbers( uint8_t bus, uint8_t device, uint8_t function ) const
{
    WriteAddress( MakeAddress(bus, device, function, 0x18) );
    return ReadData();
}

ClassCode ConfigurationArea::ReadClassCode( uint8_t bus, uint8_t device, uint8_t function ) const
{
    WriteAddress( MakeAddress(bus, device, function, 0x08) );
    return ClassCode( ReadData() );
}

uint8_t ConfigurationArea::CapablitiesPointer( uint8_t bus, uint8_t device, uint8_t function ) const
{
    WriteAddress( MakeAddress(bus, device, function, 0x34) );
    return ReadData() & 0xFFu;
}

WithError<uint64_t> ConfigurationArea::ReadBAR( uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_num ) const
{
    if( bar_num >= 6 ){
        return { 0, MAKE_ERROR(Error::kIndexOutOfRange) };
    }
    uint8_t  addr = 0x10 + (bar_num*0x04);
    uint32_t bar_lower = ReadConfReg( {bus, device, function}, addr );

    // 32bitアドレスかどうか確認
    if( (bar_lower & 0x06) == 0 ){
        return { bar_lower, MAKE_ERROR(Error::kSuccess) };
    }

    // 64bit アドレスなのにBAR5は指定できない
    if( bar_num >= 5 ){
        return { 0, MAKE_ERROR(Error::kIndexOutOfRange) };
    }

    uint64_t bar_upper = ReadConfReg( {bus, device, function}, addr + 4 );
    return { (bar_upper << 32) | bar_lower, MAKE_ERROR(Error::kSuccess) };
}

WithError<uint64_t> ConfigurationArea::ReadBAR( const Device& device, uint8_t bar_num ) const
{
    return ReadBAR( device.Bus, device.Device, device.Function, bar_num );
}

bool ConfigurationArea::IsSingleFunctionDevice( uint8_t header_type ) const
{
    return (header_type & 0x80u) == 0;
}

Error::Code ConfigurationArea::ScanAllBus()
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

    return Error::Code::kSuccess;
}

const ConfigurationArea::DeviceArray& ConfigurationArea::GetDevices() const
{
    return m_Devices;
}

int ConfigurationArea::GetDeviceNum() const
{
    return m_NumDevice;
}

Error::Code ConfigurationArea::ScanBus( uint8_t bus )
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

    return Error::Code::kSuccess;
}

Error::Code ConfigurationArea::ScanDevice( uint8_t bus, uint8_t device )
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

    return Error::Code::kSuccess;
}

Error::Code ConfigurationArea::ScanFunction( uint8_t bus, uint8_t device, uint8_t function )
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

    return Error::Code::kSuccess;
}

Error::Code ConfigurationArea::AddDevice( uint8_t bus, uint8_t device, uint8_t function, uint8_t header_type )
{
    if( m_NumDevice >= sk_DeviceMax ){
        return Error::Code::kFull;
    }

    
    m_Devices[m_NumDevice] = Device { bus, device, function, header_type, 
                                      ReadClassCode(bus, device, function) };
    ++m_NumDevice;

    return Error::Code::kSuccess;
}

static void SearchIntelxHC()
{
    auto& pci_mgr = pci::ConfigurationArea::Instance();
    g_xHC_Device = nullptr;

    const auto& devices = pci_mgr.GetDevices();
    int device_num = pci_mgr.GetDeviceNum();
    for( int i = 0; i < device_num; ++i ){
        const auto& dev = devices[i];
        if( dev.ClassCode.Match( 0x0cu, 0x03u, 0x30u ) ){
            g_xHC_Device = &devices[i];
            if( pci_mgr.ReadVendorID(*g_xHC_Device) == 0x8086 ){
                break;
            }
        }
    }

    if( g_xHC_Device ){
        const auto& classcode = g_xHC_Device->ClassCode;
        Log( kDebug, "xHC has been found: %d.%d.%d.\n",
             classcode.Base(), classcode.Sub(), classcode.Interface() );
    }
}

void InitializePCI()
{
    pci::ConfigurationArea().Instance().ScanAllBus();
    const auto& devices = pci::ConfigurationArea().Instance().GetDevices();
    int device_num = pci::ConfigurationArea().Instance().GetDeviceNum();

   Log( kDebug, "NumDevice : %d\n", device_num );
    for( int i = 0; i < device_num; ++i ){
        const auto& dev = devices[i];
        auto vendor_id  = pci::ConfigurationArea::Instance().ReadVendorID( dev.Bus, dev.Device, dev.Function );
        auto class_code = pci::ConfigurationArea::Instance().ReadClassCode( dev.Bus, dev.Device, dev.Function );
        Log( kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n",
             dev.Bus, dev.Device, dev.Function,
             vendor_id, class_code, dev.HeaderType );
    }
    SearchIntelxHC();
}

} // namespace pci

