#pragma once

#include <memory>

#include "Graphic.hpp"
#include "Window.hpp"

using LayerID = unsigned int;

class Layer
{
public:

    //! @brief 指定されたIDを持つレイヤーを生成する
    Layer( LayerID id = 0 );
    
    //! @brief このインスタンスのIDを返す
    LayerID ID() const;

    //! @brief ウインドウを設定する。既存のウィンドウはこのレイヤーから外れる
    Layer& SetWindow( const std::shared_ptr<Window>& window );
    //! @brief 設定されたウィンドウを返す
    std::shared_ptr<Window> GetWindow() const;

    //! @brief レイヤーの位置情報を指定された絶対座標へと更新する。再描写はしない
    Layer& Move( Vector2<int> pos );
    //! @brief レイヤーの位置情報を指定された双代座標へと更新する。再描写はしない
    Layer& MoveRelative( Vector2<int> pos_diff );

    //! @brief writer に現在設定されているウィンドウの内容を描写する
    void DrawTo( IPixelWriter& writer ) const;

private:

    unsigned int  m_ID;
    Vector2<int> m_Pos;
    std::shared_ptr<Window> m_Window;
};

class LayerManager
{
public:

    LayerManager();
    ~LayerManager() = default;

    //! @brief  Drawメソッド などで描写する際の描画先を指定する
    void SetWriter( IPixelWriter* writer );

    /**
     * @brief  新しいレイヤーを生成して参照を返す
     * 新しく生成されたレイヤーの実態は LayerManager 内部のコンテナで保持される
     */
    Layer& NewLayer();

    //! @brief 現在表示状態にあるレイヤーを描写する
    void Draw() const;
    //! @brief レイヤーの位置情報を指定された絶対座標へと更新する。再描写はしない
    void Move( LayerID id, Vector2<int> pos );
    //! @brief レイヤーの位置情報を指定された双代座標へと更新する。再描写はしない
    void MoveRelative( LayerID id, Vector2<int> pos_diff );

    /**
     * @brief  レイヤーの高さ方向の位置を指定された位置に移動する
     * new_height に負の高さを指定するとレイヤーは非表示になり
     * 0以上を指定するとその高さとなる
     * 現在のレイヤー数以上の数値を指定した場合は最前面のレイヤーとなる
     */
    void UpDown( LayerID id, int new_height );

    //! @brief  レイヤーを非表示にする
    void Hide( LayerID id );

private:

    using LayerPtr = std::unique_ptr<Layer>;

    Layer* FindLayer( LayerID id );

    IPixelWriter* m_Writer;
    std::vector<LayerPtr> m_Layers;
    std::vector<Layer*>   m_LayerStack;
    LayerID m_LatestID;
};