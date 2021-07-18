#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>

template <typename T, std::size_t N>
class RingBuffer
{
public:
    RingBuffer() {}
    ~RingBuffer() {}

    bool Push( const T& value )
    {
        uint32_t end_next = (m_End + 1) % sk_BufferSize;
        if( end_next == m_Begin ){
            return false;
        }

        m_Buffer[m_End] = value;
        m_End = end_next;

        return true;
    }

    bool IsEmpty()
    {
        return m_Begin == m_End;
    }

    T& Front()
    {
        return const_cast<T&>( static_cast<const RingBuffer&>(*this).Front() );

    }

    const T& Front() const
    {
        return m_Buffer[m_Begin];
    }

    void Pop()
    {
        if( !IsEmpty() ){
            m_Begin = (m_Begin + 1) % sk_BufferSize;
        }
    }

    T& Back()
    {
        return const_cast<T&>( static_cast<const RingBuffer&>(*this).Back() );

    }

    const T& Back() const
    {
        uint32_t prev = m_End == 0 ? sk_BufferSize - 1 : m_End - 1;
        return m_Buffer[prev];
    }

    std::size_t Size() const
    {
        if( m_Begin <= m_End ){
            return m_End - m_Begin;
        }
        
        return (sk_BufferSize - m_Begin) + m_End;
    }

    const T& operator[]( std::size_t idx ) const
    {
        std::size_t it = (idx + m_Begin) % sk_BufferSize;
        return m_Buffer[it];
    }

    T& operator[]( std::size_t idx )
    {
        return const_cast<T&>( static_cast<const RingBuffer&>(*this)[idx] );
    }

private:

    static_assert( N >= 0 && N < (std::numeric_limits<std::size_t>::max() / 2) - 1, "Invalid Buffer size" );
    static constexpr std::size_t sk_BufferSize = N + 1;

    T m_Buffer[sk_BufferSize];
    std::size_t m_Begin;
    std::size_t m_End;
};