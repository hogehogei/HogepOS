//
// include files
//
#include "Layer.hpp"


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

    //! @brief 指定されたIDを持つレイヤーを生成する
Layer::Layer( LayerID id )
    : m_ID(id),
      m_Pos(0, 0),
      m_Window()
{}

LayerID Layer::ID() const
{
    return m_ID;
}

Layer& Layer::SetWindow( const std::shared_ptr<Window>& window )
{
    m_Window = window;
    return *this;
}

std::shared_ptr<Window> Layer::GetWindow() const
{
    return m_Window;
}

Layer& Layer::Move( Vector2<int> pos )
{
    m_Pos = pos;
    return *this;
}

Layer& Layer::MoveRelative( Vector2<int> pos_diff )
{
    m_Pos += pos_diff;
    return *this;
}

void Layer::DrawTo( FrameBuffer& fb ) const
{
    if( m_Window ){
        m_Window->DrawTo( fb, m_Pos );
    }
}


LayerManager::LayerManager()
    : m_Screen( nullptr ),
      m_Layers(),
      m_LayerStack(),
      m_LatestID( 0 )
{}

void LayerManager::SetWriter( FrameBuffer* screen )
{
    m_Screen = screen;
}

Layer& LayerManager::NewLayer()
{
    ++m_LatestID;
    return *(m_Layers.emplace_back( std::make_unique<Layer>(m_LatestID) ));
}


void LayerManager::Draw() const
{
    for( auto layer : m_LayerStack ){
        layer->DrawTo( *m_Screen );
    }
}

void LayerManager::Move( LayerID id, Vector2<int> pos )
{
    FindLayer(id)->Move(pos);
}

void LayerManager::MoveRelative( LayerID id, Vector2<int> pos_diff )
{
    FindLayer(id)->MoveRelative(pos_diff);
}

void LayerManager::UpDown( LayerID id, int new_height )
{
    if( new_height < 0 ){
        Hide( id );
        return;
    }
    if( new_height > m_LayerStack.size() ){
        new_height = m_LayerStack.size();
    }

    auto layer   = FindLayer(id);
    auto old_pos = std::find( m_LayerStack.begin(), m_LayerStack.end(), layer );
    auto new_pos = m_LayerStack.begin() + new_height;

    // 非表示の場合、新しい位置にレイヤを挿入して終了
    if( old_pos == m_LayerStack.end() ){
        m_LayerStack.insert( new_pos, layer );
        return;
    }

    // 一番最後を指している場合、eraseすると end() の向こう側に行ってしまうので一つ戻す
    if( new_pos == m_LayerStack.end() ){
        --new_pos;
    }

    m_LayerStack.erase( old_pos );
    m_LayerStack.insert( new_pos, layer );
}

void LayerManager::Hide( LayerID id )
{
    auto layer = FindLayer(id);
    auto pos   = std::find( m_LayerStack.begin(), m_LayerStack.end(), layer );
    if( pos != m_LayerStack.end() ){
        m_LayerStack.erase(pos);
    }
}

Layer* LayerManager::FindLayer( LayerID id )
{
    auto pred = [id]( const LayerPtr& elem ){
        return elem->ID() == id;
    };

    auto itr = std::find_if( m_Layers.begin(), m_Layers.end(), pred );
    if( itr == m_Layers.end() ){
        return nullptr;
    }

    return itr->get();
}