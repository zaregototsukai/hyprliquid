#include "BackgroundSharePassElement.h"
#include "Context/MonitorContext.h"
#include "Utils/BackgroundManager.h"
#include <hyprland/src/render/GLRenderer.hpp>

std::vector<UP<IPassElement>> CBackgroundSharePassElement::draw(Render::CRenderContext& ctx)
{
    auto& m_renderData = ctx.m_data;
    BackgroundManager::ShareBackground(m_renderData.pMonitor.lock(), m_renderData.currentFB->getTexture());

    return std::vector<UP<IPassElement>>();
}

bool CBackgroundSharePassElement::needsLiveBlur(Render::CRenderContext& ctx)
{
    return false;
}

bool CBackgroundSharePassElement::needsPrecomputeBlur(Render::CRenderContext& ctx)
{
    return false;
}

const char *CBackgroundSharePassElement::passName()
{
    return "CBackgroundSharePassElement";
}

ePassElementType CBackgroundSharePassElement::type()
{
    return EK_CUSTOM;
};
