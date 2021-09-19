//
// include files
//
#include "Timer.hpp"

//
// constant
//

//
// static variables
//
namespace
{
    constexpr uint32_t k_CountMax = 0xFFFFFFFFu;
    volatile uint32_t* lvt_timer = reinterpret_cast<uint32_t*>(0xFEE00320);
    volatile uint32_t* initial_count = reinterpret_cast<uint32_t*>(0xFEE00380);
    volatile uint32_t* current_count = reinterpret_cast<uint32_t*>(0xFEE00390);
    volatile uint32_t* divide_config = reinterpret_cast<uint32_t*>(0xFEE003E0);
}

//
// static function declaration
// 

//
// funcion definitions
// 
void InitializeLAPICTimer()
{
    *divide_config = 0b1011;       // divide 1:1
    *lvt_timer = (0b001 << 16) | 32;    // masked, one-shot
}

void StartLAPICTimer()
{
    *initial_count = k_CountMax;
}

uint32_t LAPICTimerElapsed()
{
    return k_CountMax - *current_count;
}

void StopLAPICTimer()
{
    *initial_count = 0;
}
