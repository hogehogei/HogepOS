#pragma once

#include <cstdint>

namespace Keyboard
{

constexpr int k_LControlBitMask = 0b00000001u;
constexpr int k_LShiftBitMask   = 0b00000010u;
constexpr int k_LAltBitMask     = 0b00000100u;
constexpr int k_LGUIBitMask     = 0b00001000u;
constexpr int k_RControlBitMask = 0b00010000u;
constexpr int k_RShiftBitMask   = 0b00100000u;
constexpr int k_RAltBitMask     = 0b01000000u;
constexpr int k_RGUIBitMask     = 0b10000000u;

struct Key
{
    Key() = default;
    Key( uint8_t modifier, uint8_t keycode );

    bool IsPressedShift() const;
    char Ascii() const;

    uint8_t Modifier() const;
    uint8_t KeyCode() const;

private:

    uint8_t m_Modifier;
    uint8_t m_KeyCode;
};

void InitializeKeyboard();
}



