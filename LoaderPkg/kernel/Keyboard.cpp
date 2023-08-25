
//
// include files
//
#include "Keyboard.hpp"
#include "Event.hpp"
#include "Global.hpp"
#include "Task.hpp"

#include <memory>
#include "usb/classdriver/keyboard.hpp"

//
// constant
//
namespace {
const char s_KeycodeMap[256] = {
  0,    0,    0,    0,    'a',  'b',  'c',  'd', // 0
  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l', // 8
  'm',  'n',  'o',  'p',  'q',  'r',  's',  't', // 16
  'u',  'v',  'w',  'x',  'y',  'z',  '1',  '2', // 24
  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0', // 32
  '\n', '\b', 0x08, '\t', ' ',  '-',  '=',  '[', // 40
  ']', '\\',  '#',  ';', '\'',  '`',  ',',  '.', // 48
  '/',  0,    0,    0,    0,    0,    0,    0,   // 56
  0,    0,    0,    0,    0,    0,    0,    0,   // 64
  0,    0,    0,    0,    0,    0,    0,    0,   // 72
  0,    0,    0,    0,    '/',  '*',  '-',  '+', // 80
  '\n', '1',  '2',  '3',  '4',  '5',  '6',  '7', // 88
  '8',  '9',  '0',  '.', '\\',  0,    0,    '=', // 96
};

const char s_KeycodeMapShifted[256] = {
  0,    0,    0,    0,    'A',  'B',  'C',  'D', // 0
  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L', // 8
  'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T', // 16
  'U',  'V',  'W',  'X',  'Y',  'Z',  '!',  '@', // 24
  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')', // 32
  '\n', '\b', 0x08, '\t', ' ',  '_',  '+',  '{', // 40
  '}',  '|',  '~',  ':',  '"',  '~',  '<',  '>', // 48
  '?',  0,    0,    0,    0,    0,    0,    0,   // 56
  0,    0,    0,    0,    0,    0,    0,    0,   // 64
  0,    0,    0,    0,    0,    0,    0,    0,   // 72
  0,    0,    0,    0,    '/',  '*',  '-',  '+', // 80
  '\n', '1',  '2',  '3',  '4',  '5',  '6',  '7', // 88
  '8',  '9',  '0',  '.', '\\',  0,    0,    '=', // 96
};
}


//
// static variables
//

//
// static function declaration
// 


//
// funcion definitions
// 

namespace Keyboard
{

Key::Key( uint8_t modifier, uint8_t keycode )
    : m_Modifier( modifier ),
      m_KeyCode( keycode )
{}

bool Key::IsPressedShift() const
{
    return (m_Modifier & (k_LShiftBitMask | k_RShiftBitMask)) != 0;
}

char Key::Ascii() const
{
    if( IsPressedShift() ){
        return s_KeycodeMapShifted[m_KeyCode];
    }

    return s_KeycodeMap[m_KeyCode];
}

uint8_t Key::Modifier() const
{
    return m_Modifier;
}

uint8_t Key::KeyCode() const
{
    return m_KeyCode;
}

void InitializeKeyboard()
{
    usb::HIDKeyboardDriver::default_observer = 
        [] (uint8_t modifier, uint8_t keycode) {

            Message msg( Message::k_KeyPush, TaskManager::k_MainTaskID );
            msg.Arg.Keyboard.Key = Key( modifier, keycode );
            TaskManager::Instance().SendMessage( TaskManager::k_MainTaskID, msg );
        };
}
}
