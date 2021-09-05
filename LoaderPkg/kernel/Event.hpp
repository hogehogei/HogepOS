#pragma once 

struct Message
{
    enum Type {
        k_InterruptXHCI,
    } type;

    Message() = default;
    ~Message() = default;
    
    Message( Message::Type value )
     : type(value) {}
};