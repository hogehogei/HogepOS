//
// include files
//
#include <cstring>
#include <cstdlib>
#include "ACPI.hpp"
#include "Global.hpp"

#include "asmfunc.h"
#include "logger.hpp"

namespace acpi
{
//
// constant
//
static constexpr uint64_t k_PmTimerFreq = 3579545;   // 3.579545Mhz


//
// static variables
//

//
// static function declaration
// 
namespace 
{
template <typename T>
uint8_t SumBytes( const T* data, size_t bytes ){
    return SumBytes( reinterpret_cast<const uint8_t*>(data), bytes );
}

template <>
uint8_t SumBytes<uint8_t>( const uint8_t* data, size_t bytes )
{
    uint8_t sum = 0;
    for( size_t i = 0; i < bytes; ++i ){
        sum += data[i];
    }
    
    return sum;
}
}

//
// funcion definitions
// 
bool RSDP::IsValid() const
{
    if( strncmp( this->Signature, "RSD PTR ", 8 ) != 0 ){
        Log( kDebug, "Invalid signature: %.8s\n", this->Signature );
        return false;
    }
    if( this->Revision != 2 ){
        Log( kDebug, "ACPI version must be 2: %d\n", this->Revision );
        return false;
    }
    
    auto sum = SumBytes( this, 20 ); 
    if( sum != 0 ){
        Log( kDebug, "Sum of 20Bytes must be 0: %d\n", sum );
        return false;
    }

    sum = SumBytes( this, 36 );
    if( sum != 0 ){
        Log( kDebug, "Sum of 36Bytes must be 0: %d\n", sum );
        return false;
    }

    return true;
}

bool DescriptionHeader::IsValid( const char* expected_signature ) const
{
    if( strncmp( this->Signature, expected_signature, 4 ) != 0 ){
        Log( kDebug, "Invalid signature: %.4s\n", this->Signature );
        return false;
    }

    auto sum = SumBytes( this, this->Length );
    if( sum != 0 ){
        Log( kDebug, "sum of %u bytes must be 0: %d\n", this->Length, sum );
        return false;
    }

    return true;
}

const DescriptionHeader& XSDT::operator[]( size_t i ) const
{
    auto entries = reinterpret_cast<const uint64_t*>( &this->Header + 1 );
    return *reinterpret_cast<const DescriptionHeader*>(entries[i]);
}

size_t XSDT::Count() const
{
    return (this->Header.Length - sizeof(DescriptionHeader)) / sizeof(uint64_t);
}

void Initialize( const RSDP& rsdp )
{
    if( !rsdp.IsValid() ){
        Log( kError, "RSDP is not valid\n" );
        exit( 1 );
    }

    const XSDT& xsdt = *reinterpret_cast<const XSDT*>( rsdp.XSDT_Address );
    if( !xsdt.Header.IsValid("XSDT") ){
        Log( kError, "XSDT is not valid.\n" );
        exit(1);
    }

    g_FADT = nullptr;
    for( int i = 0; i < xsdt.Count(); ++i ){
        const auto& entry = xsdt[i];
        // FACP is the signature of FADT
        if( entry.IsValid("FACP") ){
            g_FADT = reinterpret_cast<const FADT*>(&entry);
            break;
        }
    }

    if( g_FADT == nullptr ){
        Log( kError, "FADT is not found.\n" );
        exit(1);
    }
}

void WaitMillSeconds( uint32_t msec )
{
    const bool pm_timer_32 = (g_FADT->Flags >> 8) & 1;
    const uint32_t start = IoIn32(g_FADT->PmTmrBlk);
    uint32_t end = start + (k_PmTimerFreq * msec / 1000);

    if( !pm_timer_32 ){
        end &= 0x00FFFFFFu;
    }

    if( end < start ){
        while( IoIn32( g_FADT->PmTmrBlk ) >= start ) ; 
    }

    while( IoIn32( g_FADT->PmTmrBlk ) < end );
}

} // namespace acpi

