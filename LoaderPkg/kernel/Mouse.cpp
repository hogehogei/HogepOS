
//
// include files
//
#include "Mouse.hpp"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"

#include "Global.hpp"
#include "PCI.hpp"
#include "logger.hpp"

//
// constant
//


//
// static variables
//

//
// static function declaration
// 
static uint64_t Read_xHC_Bar();
static void Init_XHCI_Controller( usb::xhci::Controller& controller );
static void SwitchEhci2Xhci( const pci::Device& xhc_dev );

//
// funcion definitions
// 
void InitializeMouse()
{
    uint64_t xhc_mmio_base = Read_xHC_Bar();

    g_xHC_Controller = new usb::xhci::Controller( xhc_mmio_base );
    Log( kDebug, "xHC MMIO Base : %llx\n", xhc_mmio_base );
    Init_XHCI_Controller( *g_xHC_Controller );

    usb::HIDMouseDriver::default_observer = MouseObserver;
    for( int i = 1; i <= g_xHC_Controller->MaxPorts(); ++i ){
        auto port = g_xHC_Controller->PortAt(i);
        Log( kDebug, "Port %d : IsConnected=%d\n", i, port.IsConnected() );

        if( port.IsConnected() ){
            auto err = ConfigurePort( *g_xHC_Controller, port );
            if( err ){
                Log( kDebug, "Failed to configure port: %s at %s:%d\n",
                     err.Name(), err.File(), err.Line() );
            }
        }
    }
}

static uint64_t Read_xHC_Bar()
{
    auto xhc_bar = pci::ConfigurationArea::Instance().ReadBAR( *g_xHC_Device, 0 );
    if( !xhc_bar.error ){
        Log( kDebug, "Read xHC Bar failed.\n", xhc_bar.error.Name() );
    }
    return xhc_bar.value & ~static_cast<uint64_t>(0xF);
}

static void Init_XHCI_Controller( usb::xhci::Controller& controller )
{
    if( pci::ConfigurationArea::Instance().ReadVendorID( *g_xHC_Device ) ){
        SwitchEhci2Xhci( *g_xHC_Device );
    }

    auto err = controller.Initialize();
    Log( kDebug, "xhc.Initialize: %s\n", err.Name() );

    Log( kDebug, "xHC starting.\n" );
    controller.Run();
}

static void SwitchEhci2Xhci( const pci::Device& xhc_dev )
{
    auto& pci_mgr = pci::ConfigurationArea::Instance();
    bool intel_ehc_exist = false;

    const auto& devices = pci_mgr.GetDevices();
    int device_num = pci_mgr.GetDeviceNum();
    for( int i = 0; i < device_num; ++i ){
        // classcode = EHCI 
        if( devices[i].ClassCode.Match( 0x0Cu, 0x03u, 0x20u ) ){
            intel_ehc_exist = true;
            break;
        }
    }

    if( !intel_ehc_exist ){
        return;
    }


    uint32_t superspeed_ports = pci_mgr.ReadConfReg( xhc_dev, 0xDC );   // USB3PRM
    pci_mgr.WriteConfReg( xhc_dev, 0xD8, superspeed_ports );            // USB3_PSSEN
    uint32_t ehci2xhci_ports  = pci_mgr.ReadConfReg( xhc_dev, 0xD4 );   // XUSB2PRM
    pci_mgr.WriteConfReg( xhc_dev, 0xD0, ehci2xhci_ports );             // XUSB2PR

    Log( kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n",
         superspeed_ports, ehci2xhci_ports );
}