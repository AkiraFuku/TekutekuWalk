#pragma once
#include <wrl.h>
#include <memory>
#include "Singleton.h"
class ImGuiManager : public Singleton<ImGuiManager>
{
public:
    friend class Singleton<ImGuiManager>;
    friend struct std::default_delete<ImGuiManager>;
    void Initialize();
    void Finalize();
    void Begin();
    void End();
    void Draw();

    // 表示・非表示の切り替え
    bool IsShow() const { return isShow_; }
    void SetShow(bool show);
    void ToggleShow();
protected:
    ImGuiManager() = default;
    ~ImGuiManager() override = default;

private:
    bool isShow_ = true;

    
    
   
};

