#pragma once

#include <hyprland/src/event/EventBus.hpp>

class EffectDamageManager
{
public:
    static void Init();
    static void Destroy();

public:
    static inline CRegion EffectBackdropRegion;

private:
    static void ExpandDamageForEffects(Render::CRenderContext& ctx);
    
private:
    static inline Hyprutils::Signal::CHyprSignalListener m_RenderStageListener;
};