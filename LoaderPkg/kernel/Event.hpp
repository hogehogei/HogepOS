#pragma once 

struct Message
{
    enum Type {
        k_InterruptXHCI,
        k_InterruptLAPICTimer,
    } type;

    Message() = default;
    ~Message() = default;
    
    Message( Message::Type value )
     : type(value) {}
};