#pragma once
#include <hyprland/src/render/pass/PassElement.hpp>

class CBackgroundSharePassElement : public IPassElement
{
public:
    virtual std::vector<UP<IPassElement>> draw(Render::CRenderContext& ctx);
    virtual bool                          needsLiveBlur(Render::CRenderContext& ctx);
    virtual bool                          needsPrecomputeBlur(Render::CRenderContext& ctx);
    virtual const char*                   passName();
    virtual ePassElementType              type();
};