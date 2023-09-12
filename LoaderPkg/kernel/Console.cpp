#include <cstring>

#include "Console.hpp"
#include "Global.hpp"
#include "Font.hpp"

Console::Console( const PixelColor& fg_color, const PixelColor& bg_color )
    : m_Window( nullptr ),
      m_LayerID( 0 ),
      m_ForeGroundColor( fg_color ),
      m_BackGroundColor( bg_color ),
      m_CursorRow( 0 ),
      m_CursorColumn( 0 ),
      m_Buffer{},
      m_ConsoleLines()
{
    m_ConsoleLines.Push( m_Buffer[m_CursorRow] );
}

void Console::SetWindow( std::shared_ptr<Window> window )
{
    m_Window = window;
    Refresh();
}

std::shared_ptr<Window> Console::GetWindow()
{
    return m_Window;
}

void Console::SetLayerID( unsigned int layer_id )
{
    m_LayerID = layer_id;
}

unsigned int Console::LayerID() const
{
    return m_LayerID;
}
      
void Console::PutString( const char* s )
{
    char* curr_line = CurrentLine();
    while( *s ){
        if( *s == '\n' ){
            NewLine();
        }
        else if( m_CursorColumn < sk_Columns ){
            WriteAscii( *m_Window->Writer(), 8 * m_CursorColumn, 16 * m_CursorRow, *s, m_ForeGroundColor );
            curr_line[m_CursorColumn] = *s;
            ++m_CursorColumn;
        }
        ++s;
    }

    if( g_LayerManager ){
        g_LayerManager->Draw( m_LayerID );
    }
}

void Console::ClearConsole()
{
    for( int y = 0; y < 16 * sk_Rows; ++y ){
        for( int x = 0; x < 8 * sk_Columns; ++x ){
            m_Window->Writer()->Write( x, y, m_BackGroundColor );
        }
    }
    g_LayerManager->Draw( m_LayerID );
}

char* Console::CurrentLine()
{
    return m_ConsoleLines.Back();
}

void Console::NewLine()
{
    m_CursorColumn = 0;

    if( m_CursorRow < (sk_Rows - 1) ){
        ++m_CursorRow;
        m_ConsoleLines.Push( m_Buffer[m_CursorRow] );
    }
    else {
        // コンソールクリア
        ClearConsole();
        // 一番最初の行を取り出し、最後尾に追加する
        char* line = m_ConsoleLines.Front();
        m_ConsoleLines.Pop();
        memset( line, 0, sk_ColumnBufferLen );
        m_ConsoleLines.Push( line );
        // コンソール再描写
        Refresh();
    }
}

void Console::Refresh()
{
    std::size_t size = m_ConsoleLines.Size();
    for( int row = 0; row < size; ++row ){
        const char* line = m_ConsoleLines[row];
        WriteString( *m_Window->Writer(), 0, 16 * row, line, m_ForeGroundColor );
    }

    g_LayerManager->Draw( m_LayerID );
}