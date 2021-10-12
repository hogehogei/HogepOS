
//
// include files
//
#include "Mouse.hpp"
#include "MouseCursor.hpp"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"

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
void InitializeMouse()
{
    usb::HIDMouseDriver::default_observer = MouseObserver;
}
