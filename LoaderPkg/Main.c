#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h> 
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include  <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>

#include "Main.h"
#include "PixelFormat.h"
#include "MemoryMap.hpp"
#include "ELF.hpp"



// 
// definition
//
#define     KERNEL_BASE_ADDRESS    ((EFI_PHYSICAL_ADDRESS)0x100000)
// 
// static variables
//
CHAR8 memmap_buffer[1024 * 4 * 4];    // pagesize 4KiB, 16KiB buffer
FrameBufferConfig config;

//
// static functions
//
static EFI_STATUS GetMemoryMap( MemoryMap* map );
static EFI_STATUS SaveMemoryMap( MemoryMap* map, EFI_FILE_PROTOCOL* file );
static const CHAR16* GetMemoryTypeUnicode( EFI_MEMORY_TYPE type );
static EFI_STATUS OpenRootDir( EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root );
static EFI_STATUS OpenGOP( EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL** gop );
static const CHAR16* GetPixelFormatUnicode( EFI_GRAPHICS_PIXEL_FORMAT fmt );
static EFI_PHYSICAL_ADDRESS ReadKernel( EFI_FILE_PROTOCOL* root_dir );
static void CalcLoadAddressRange( Elf64_Ehdr* ehdr, UINT64* first, UINT64* last );
static void CopyLoadSegments( Elf64_Ehdr* ehdr );
static void StopBootServices( EFI_HANDLE image_handle, MemoryMap memmap );

static void Halt();
static void HaltOnError( EFI_STATUS status, const UINT16* message );

EFI_STATUS EFIAPI UefiMain( EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table )
{
    EFI_STATUS status;
    MemoryMap memmap = {
        sizeof(memmap_buffer),
        memmap_buffer,
        0, 0, 0, 0
    };
    GetMemoryMap( &memmap );

    EFI_FILE_PROTOCOL* root_dir;
    status = OpenRootDir( image_handle, &root_dir );
    HaltOnError( status, L"Failed to open root directory." );

    EFI_FILE_PROTOCOL* memmap_file;
    status = root_dir->Open( root_dir, &memmap_file, L"\\memmap", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0 );
    HaltOnError( status, L"Failed to open memmap file." );
    status = SaveMemoryMap( &memmap, memmap_file );
    HaltOnError( status, L"Failed to save memroy map." );
    memmap_file->Close( memmap_file );

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    status = OpenGOP( image_handle, &gop );
    HaltOnError( status, L"Failed to open GOP." );

    EFI_PHYSICAL_ADDRESS kernel_base_addr = ReadKernel( root_dir );

    // FrameBuffer 設定
    config.FrameBuffer          = (UINT8*)gop->Mode->FrameBufferBase;
    config.PixelsPerScanLine    = gop->Mode->Info->PixelsPerScanLine;
    config.HorizontalResolution = gop->Mode->Info->HorizontalResolution;
    config.VerticalResolution   = gop->Mode->Info->VerticalResolution;
    
    switch( gop->Mode->Info->PixelFormat )
    {
    case PixelRedGreenBlueReserved8BitPerColor:
        config.PixelFormat = kPixelRGBReserved8BitPerColor;
        break;
    case PixelBlueGreenRedReserved8BitPerColor:
        config.PixelFormat = kPixelBGRReserved8BitPerColor;
        break;
    default:
        config.PixelFormat = kUnknownFormat;
        Print( L"Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat );
        Halt();
        break;
    }

    Print( L"Jump to entry point.\n" );
    StopBootServices( image_handle, memmap );

    UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);
    typedef void EntryPointType( const FrameBufferConfig*, const MemoryMap* );
    EntryPointType* entry_point = (EntryPointType*)entry_addr;

    entry_point( &config, &memmap );

    while( 1 );
    return EFI_SUCCESS;
}

static EFI_STATUS GetMemoryMap( MemoryMap* map )
{
    if( map->buffer == NULL ){
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size = map->buffer_size;
    return gBS->GetMemoryMap( 
        &map->map_size,
        (EFI_MEMORY_DESCRIPTOR*)map->buffer,
        &map->map_key,
        &map->descriptor_size,
        &map->descriptor_version );
}

static EFI_STATUS SaveMemoryMap( MemoryMap* map, EFI_FILE_PROTOCOL* file )
{
    CHAR8 buf[256];
    UINTN len;
    EFI_STATUS status;
    
    CHAR8* header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen( header );

    status = file->Write( file, &len, header );
    if( EFI_ERROR(status) ){
        return status;
    }

    Print( L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer, map->map_size );
    EFI_PHYSICAL_ADDRESS itr;
    int i;

    for( itr = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
         itr < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
         itr += map->descriptor_size, ++i )
    {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)itr;
        len = AsciiSPrint( 
            buf, sizeof(buf),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            i, 
            desc->Type,
            GetMemoryTypeUnicode(desc->Type),
            desc->PhysicalStart,
            desc->NumberOfPages,
            desc->Attribute & 0xFFFFFlu
        );

        status = file->Write( file, &len, buf );
        if( EFI_ERROR(status) ){
            return status;
        }
    }

    return EFI_SUCCESS;
}

static const CHAR16* GetMemoryTypeUnicode( EFI_MEMORY_TYPE type ) 
{
    switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
    }
}

static EFI_STATUS OpenRootDir( EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root )
{
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    EFI_STATUS status;

    status = gBS->OpenProtocol(
                image_handle,
                &gEfiLoadedImageProtocolGuid,
                (VOID**)&loaded_image,
                image_handle,
                NULL,
                EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
            );
    if( EFI_ERROR(status) ){
        return status;
    }

    status = gBS->OpenProtocol(
                loaded_image->DeviceHandle,
                &gEfiSimpleFileSystemProtocolGuid,
                (VOID**)&fs,
                image_handle,
                NULL,
                EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
            );
    if( EFI_ERROR(status) ){
        return status;
    }

    status = fs->OpenVolume( fs, root );
    if( EFI_ERROR(status) ){
        return status;
    }

    return EFI_SUCCESS;
}

static EFI_STATUS OpenGOP( EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL** gop )
{
    UINTN num_gop_handles = 0;
    EFI_HANDLE* gop_handles = NULL;
    EFI_STATUS status;

    status = gBS->LocateHandleBuffer( 
                ByProtocol, 
                &gEfiGraphicsOutputProtocolGuid,
                NULL,
                &num_gop_handles,
                &gop_handles
             );
    if( EFI_ERROR(status) ){
        return status;
    }
    status = gBS->OpenProtocol( 
                gop_handles[0],
                &gEfiGraphicsOutputProtocolGuid,
                (VOID**)gop,
                image_handle,
                NULL,
                EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
            );
    if( EFI_ERROR(status) ){
        return status;
    }

    FreePool( gop_handles );

    Print( L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
        (*gop)->Mode->Info->HorizontalResolution,
        (*gop)->Mode->Info->VerticalResolution,
        GetPixelFormatUnicode((*gop)->Mode->Info->PixelFormat),
        (*gop)->Mode->Info->PixelsPerScanLine );
    Print( L"FrameBuffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
        (*gop)->Mode->FrameBufferBase,
        (*gop)->Mode->FrameBufferBase + (*gop)->Mode->FrameBufferSize,
        (*gop)->Mode->FrameBufferSize );
    
    return EFI_SUCCESS;
}

static const CHAR16* GetPixelFormatUnicode( EFI_GRAPHICS_PIXEL_FORMAT fmt )
{
    switch( fmt )
    {
        case PixelRedGreenBlueReserved8BitPerColor:
            return L"PixelRedGreenBlueReserved8BitPerColor";
        case PixelBlueGreenRedReserved8BitPerColor:
            return L"PixelBlueGreenRedReserved8BitPerColor";
        case PixelBitMask:
            return L"PixelBitMask";
        case PixelBltOnly:
            return L"PixelBltOnly";
        case PixelFormatMax:
            return L"PixelFormatMax";
        default:
            return L"InvalidPixelFormat";
    }
}
static EFI_PHYSICAL_ADDRESS ReadKernel( EFI_FILE_PROTOCOL* root_dir )
{
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* kernel_file;

    status = root_dir->Open( root_dir, &kernel_file, L"\\kernel.elf", EFI_FILE_MODE_READ, 0 );
    HaltOnError( status, L"Failed to open kernel file." );

    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];
    status = kernel_file->GetInfo( kernel_file, 
                                   &gEfiFileInfoGuid,
                                   &file_info_size, 
                                   file_info_buffer );
    HaltOnError( status, L"Failed to get file info." );
    
    // まずは一時領域にカーネルファイルを読み込む
    EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

    VOID* kernel_buffer;
    status = gBS->AllocatePool( EfiLoaderData, kernel_file_size, &kernel_buffer );
    HaltOnError( status, L"Failed to allocate pool" );
    status = kernel_file->Read( kernel_file, &kernel_file_size, kernel_buffer );
    HaltOnError( status, L"Failed to read kernel file." );

    // カーネルコピー先領域確保
    Elf64_Ehdr* kernel_ehdr = (Elf64_Ehdr*)kernel_buffer;
    UINT64 kernel_first_addr;
    UINT64 kernel_last_addr;
    CalcLoadAddressRange( kernel_ehdr, &kernel_first_addr, &kernel_last_addr );

    UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xFFF) / 0x1000;
    status = gBS->AllocatePages(
                AllocateAddress, 
                EfiLoaderData,
                num_pages,
                &kernel_first_addr
             );

    // Loadセグメントをメモリにコピー
    CopyLoadSegments( kernel_ehdr );
    Print( L"Kernel: 0x%0lx - 0x%0lx\n", kernel_first_addr, kernel_last_addr );

    // 一時領域開放
    status = gBS->FreePool( kernel_buffer );
    HaltOnError( status, L"Failed to free pool" );

    return kernel_first_addr;
}

static void CalcLoadAddressRange( Elf64_Ehdr* ehdr, UINT64* first, UINT64* last )
{
    Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
    *first = MAX_UINT64;
    *last  = 0;

    for( Elf64_Half i = 0; i < ehdr->e_phnum; ++i ){
        if( phdr[i].p_type != PT_LOAD ){
            continue;
        }

        *first = MIN(*first, phdr[i].p_vaddr );
        *last  = MAX(*last,  phdr[i].p_vaddr + phdr[i].p_memsz );
    }
}

static void CopyLoadSegments( Elf64_Ehdr* ehdr )
{
    Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
    
    for( Elf64_Half i = 0; i < ehdr->e_phnum; ++i ){
        if( phdr[i].p_type != PT_LOAD ){
            continue;
        }

        UINT64 segment_in_file = (UINT64)ehdr + phdr[i].p_offset;
        CopyMem( (VOID*)phdr[i].p_vaddr, (VOID*)segment_in_file, phdr[i].p_filesz );

        UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
        SetMem( (VOID*)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0 );
    }
}


static void StopBootServices( EFI_HANDLE image_handle, MemoryMap memmap )
{
    EFI_STATUS status;
    status = gBS->ExitBootServices( image_handle, memmap.map_key );
    
    if( EFI_ERROR(status) ){
        status = GetMemoryMap( &memmap );
        HaltOnError( status, L"Failed to get memory map." );

        status = gBS->ExitBootServices( image_handle, memmap.map_key );
        HaltOnError( status, L"Could not exit boot service." );
    }
}

static void Halt()
{
    while(1){
        __asm__("hlt");
    }
}

static void HaltOnError( EFI_STATUS status, const UINT16* message )
{
    if( EFI_ERROR(status) ){
        Print( message );
        Print( L"Error status : %r", status );
        Halt();
    }
}