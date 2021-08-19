
//
// include files
//
#include "MouseCursor.hpp"

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

MouseCursor::MouseCursor( IPixelWriter* writer, PixelColor back_color, Vector2<int> initial_position )
    : m_PixelWriter( writer ),
      m_BackColor( back_color ),
      m_Position( initial_position )
{
    DrawMouseCursor( *m_PixelWriter, m_Position );
}

void MouseCursor::MoveRelative( Vector2<int> displacement )
{
    EraseMouseCursor( *m_PixelWriter, m_Position, m_BackColor );
    m_Position += displacement;
    DrawMouseCursor( *m_PixelWriter, m_Position );
}