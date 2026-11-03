#include "Hooks.h"
#include "Utils/Utils.hpp"
#include "Render/Render.h"
#include "Config/ConfigManager.h"
#include "Config/RuleOrDefaultValue.hpp"
#include "Config/RuleOrConfigValue.hpp"
#include "Shaders/Shaders.h"
#include "Utils/BackgroundManager.h"
#include "Utils/EffectDamageManager.h"
#include "Context/MonitorContext.h"
#include <hyprland/src/desktop/view/window/Window.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/protocols/LayerShell.hpp>
#include <hyprland/src/protocols/XDGShell.hpp>
#include <hyprland/src/xwayland/XSurface.hpp>
#include <hyprland/src/desktop/rule/layerRule/LayerRule.hpp>
#include <hyprland/src/managers/fullscreen/FullscreenController.hpp>
#include <hyprutils/utils/ScopeGuard.hpp>

using Hyprutils::Utils::CScopeGuard;
using Render::ITexture;

bool InCSurfacePassElementDraw = false;
bool InCHyprOpenGLImplRenderTextureWithBlurInternal = false;
SP<AsyncSSBOReadback> g_BlackDetectionReadback;

void HookIElementRendererDrawSurface(Render::IElementRenderer* this_ptr, Render::CRenderContext& ctx, WP<CSurfacePassElement> element, const CRegion& damage)
{
    static auto HYPRLIQUID_ENABLED = CConfigValue<Config::BOOL>(std::string(ConfigManager::ConfigNames[ConfigManager::ConfigType::ENABLED]));
    static auto EFFECT             = RuleOrDefaultValue<Config::INTEGER>(ConfigManager::ConfigType::EFFECT, 0);

    if (!*HYPRLIQUID_ENABLED)
        return IElementRendererDrawSurface_t(g_IElementRendererDrawSurfaceHook->m_original)(this_ptr, ctx, element, damage);

    if (!Shaders::IsInitialized)
        return IElementRendererDrawSurface_t(g_IElementRendererDrawSurfaceHook->m_original)(this_ptr, ctx, element, damage);

    bool hyprliquid_enabled = false;
    auto& m_data = element->m_data;

    if (m_data.popup)
        return IElementRendererDrawSurface_t(g_IElementRendererDrawSurfaceHook->m_original)(this_ptr, ctx, element, damage);

    if (m_data.pLS)
    {
        auto& tex = m_data.texture;
        auto& monitor = m_data.pMonitor;

        if (EFFECT[m_data.pLS.get()] != 0)
            hyprliquid_enabled = true;
        else if (m_data.pLS->m_namespace.contains("paper") && !monitor->m_layerSurfaceLayers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND].empty())
        {
            auto monitor_context = MonitorContext::GetContext(monitor.get());
            if (monitor_context->GetBackgroundState() != MonitorBackgroundState::LAYERSHELL)
            {
                static FrameDelayGate delay_gate{5};
                if (!g_BlackDetectionReadback)
                    g_BlackDetectionReadback = makeShared<AsyncSSBOReadback>();

                if (delay_gate([tex] { return IsTextureBlack(tex, g_BlackDetectionReadback); }))
                {
                    g_BlackDetectionReadback.reset();
                    BackgroundManager::m_BackgroundChanged.emit({monitor.lock(), tex});
                    monitor_context->SetBackgroundState(MonitorBackgroundState::LAYERSHELL);
                }
            }
        }
    }
    else if (m_data.pWindow && EFFECT[m_data.pWindow.get()] != 0)
        hyprliquid_enabled = true;

    if (hyprliquid_enabled)
    {
        InCSurfacePassElementDraw = true;
        CScopeGuard guard { []{ InCSurfacePassElementDraw = false; } };
        return IElementRendererDrawSurface_t(g_IElementRendererDrawSurfaceHook->m_original)(this_ptr, ctx, element, damage);
    }

    IElementRendererDrawSurface_t(g_IElementRendererDrawSurfaceHook->m_original)(this_ptr, ctx, element, damage);
}

SP<ITexture> CLIENT_TEXTURE;
void HookCHyprOpenGLImplRenderTextureWithBlurInternal(Render::GL::CHyprOpenGLImpl* this_ptr, Render::CRenderContext& ctx, SP<Render::ITexture> tex, const CBox& box, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data)
{
    InCHyprOpenGLImplRenderTextureWithBlurInternal = true;
    CLIENT_TEXTURE = tex;
    CScopeGuard guard
    {
        []
        {
            InCHyprOpenGLImplRenderTextureWithBlurInternal = false;
            CLIENT_TEXTURE.reset();
        }
    };
    CHyprOpenGLImplRenderTextureWithBlurInternal_t(g_CHyprOpenGLImplRenderTextureWithBlurInternalHook->m_original)(this_ptr, ctx, tex, box, data);
}

void HookCHyprOpenGLImplRenderTextureInternal(Render::GL::CHyprOpenGLImpl* this_ptr, Render::CRenderContext& ctx, SP<Render::ITexture> tex, const CBox& box, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data)
{
    using Render::IFramebuffer;

    static auto VDF_MAP_DEBUG_MODE = RuleOrDefaultValue<Config::INTEGER>(ConfigManager::ConfigType::VDF_MAP_DEBUG_MODE, 0);
    static auto EFFECT             = RuleOrDefaultValue<Config::INTEGER>(ConfigManager::ConfigType::EFFECT, 0);
    static auto FORCE_FULL_DAMAGE  = RuleOrConfigValue<Config::BOOL>(ConfigManager::ConfigType::FORCE_FULL_DAMAGE);

    if (!InCSurfacePassElementDraw)
        return CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, tex, box, data);

    auto& render_data = ctx.m_data;

    client_t client = nullptr;
    if (render_data.currentWindow)
        client = render_data.currentWindow.get();
    else if (data.currentLS)
        client = data.currentLS.get();

    auto client_context = ClientContext::GetContext(client);
    if (!client_context)
        return CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, tex, box, data);

    const int effect = EFFECT[client_context];
    if (effect == 0)
        return CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, tex, box, data);

    int vdf_map_debug_mode = VDF_MAP_DEBUG_MODE[client_context];

    CRegion _damage{box};
    auto pdata = const_cast<Render::GL::CHyprOpenGLImpl::STextureRenderData*>(&data);

    if (FORCE_FULL_DAMAGE[client_context])
        pdata->damage = &_damage;

    auto fb = render_data.pMonitor->resources()->getUnusedWorkBuffer();
    if (InCHyprOpenGLImplRenderTextureWithBlurInternal)
    {
        if (!data.blurredBG)
        {
            if (data.discardActive)
                return CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, tex, box, data);

            if (fb->getTexture()->m_texID == tex->m_texID)
            {
                auto _ = fb;
                fb = render_data.pMonitor->resources()->getUnusedWorkBuffer();
            }

            if (vdf_map_debug_mode == 1 || vdf_map_debug_mode == 2)
                DebugVDFMap(ctx, *fb, GetOrUpdateVDFMap(ctx, CLIENT_TEXTURE, data, client_context), box);
            else switch(effect)
            {
                case 1:
                    if (render_data.currentWindow && Fullscreen::controller()->isFullscreen(render_data.currentWindow.lock(), Fullscreen::eFullscreenMode::FSMODE_FULLSCREEN))
                        return CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, tex, box, data);
                    RenderLiquidGlass(ctx, *fb, tex, box, data, GetOrUpdateVDFMap(ctx, CLIENT_TEXTURE, data, client_context), client_context);
                    break;
                case 2:
                    RenderAcrylic(ctx, *fb, tex, CLIENT_TEXTURE, box, data, false, client_context);
                    break;
                case 3:
                    RenderAcrylic(ctx, *fb, tex, CLIENT_TEXTURE, box, data, true, client_context);
                    break;
                case 4:
                    RenderMica(ctx, *fb, CLIENT_TEXTURE, box, data, false, client_context);
                    break;
                case 5:
                    RenderMica(ctx, *fb, CLIENT_TEXTURE, box, data, true, client_context);
                    break;
                case 6:
                    RenderAero(ctx, *fb, CLIENT_TEXTURE, tex, box, data, client_context);
                    break;
                case 0:
                default:
                    break;
            }

            render_data.currentFB->bind();
            return CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, fb->getTexture(), box, data);
        }
        else if (vdf_map_debug_mode != 2)
            CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, tex, box, data);

        return;
    }

    if (vdf_map_debug_mode == 1 || vdf_map_debug_mode == 2)
        DebugVDFMap(ctx, *fb, GetOrUpdateVDFMap(ctx, tex, data, client_context), box);
    else switch(effect)
    {
        case 1:
            if (render_data.currentWindow && Fullscreen::controller()->isFullscreen(render_data.currentWindow.lock(), Fullscreen::eFullscreenMode::FSMODE_FULLSCREEN))
                return CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, tex, box, data);
            RenderLiquidGlass(ctx, *fb, render_data.currentFB->getTexture(), box, data, GetOrUpdateVDFMap(ctx, tex, data, client_context), client_context);
            break;
        case 2:
            RenderAcrylic(ctx, *fb, nullptr, tex, box, data, false, client_context);
            break;
        case 3:
            RenderAcrylic(ctx, *fb, nullptr, tex, box, data, true, client_context);
            break;
        case 4:
            RenderMica(ctx, *fb, tex, box, data, false, client_context);
            break;
        case 5:
            RenderMica(ctx, *fb, tex, box, data, true, client_context);
            break;
        case 6:
            RenderAero(ctx, *fb, nullptr, tex, box, data, client_context);
            break;
        case 0:
        default:
            break;
    }

    auto uv_topleft     = pdata->primarySurfaceUVTopLeft;
    auto uv_bottomright = pdata->primarySurfaceUVBottomRight;

    CBox transformedBox = box;
    transformedBox.transform(Math::wlTransformToHyprutils(Math::invertTransform(render_data.pMonitor->m_transform)), render_data.pMonitor->m_transformedSize.x,
                            render_data.pMonitor->m_transformedSize.y);

    CBox monitorSpaceBox = {transformedBox.pos().x / render_data.pMonitor->m_pixelSize.x * render_data.pMonitor->m_transformedSize.x,
                            transformedBox.pos().y / render_data.pMonitor->m_pixelSize.y * render_data.pMonitor->m_transformedSize.y,
                            transformedBox.width / render_data.pMonitor->m_pixelSize.x * render_data.pMonitor->m_transformedSize.x,
                            transformedBox.height / render_data.pMonitor->m_pixelSize.y * render_data.pMonitor->m_transformedSize.y};

    pdata->primarySurfaceUVTopLeft     = monitorSpaceBox.pos() / render_data.pMonitor->m_transformedSize;
    pdata->primarySurfaceUVBottomRight = (monitorSpaceBox.pos() + monitorSpaceBox.size()) / render_data.pMonitor->m_transformedSize;

    render_data.currentFB->bind();
    CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, fb->getTexture(), box, data);

    pdata->primarySurfaceUVTopLeft     = uv_topleft;
    pdata->primarySurfaceUVBottomRight = uv_bottomright;

    if (vdf_map_debug_mode != 2)
        CHyprOpenGLImplRenderTextureInternal_t(g_CHyprOpenGLImplRenderTextureInternalHook->m_original)(this_ptr, ctx, tex, box, data);
}

CRegion HookCSurfacePassElementOpaqueRegion(CSurfacePassElement* this_ptr, Render::CRenderContext& ctx)
{
    static auto EFFECT = RuleOrDefaultValue<Config::INTEGER>(ConfigManager::ConfigType::EFFECT, 0);

    client_t    client = this_ptr->m_data.pWindow ? sc<client_t>(this_ptr->m_data.pWindow.get()) : sc<client_t>(this_ptr->m_data.pLS.get());
    const auto& client_context = ClientContext::GetContext(client);
    if (client_context && EFFECT[client_context] != 0)
        return {};

    CRegion opaque = CSurfacePassElementOpaqueRegion_t(g_CSurfacePassElementOpaqueRegionHook->m_original)(this_ptr, ctx);
    if (!opaque.empty() && !EffectDamageManager::EffectBackdropRegion.empty())
        opaque.subtract(EffectDamageManager::EffectBackdropRegion);

    return opaque;
}

float HookCWindowPresentationRounding(Desktop::View::CWindowPresentation* this_ptr)
{
    static auto ROUNDING_LUA   = RuleOrDefaultValue<Config::INTEGER>(ConfigManager::ConfigType::ROUNDING_LUA, -1);
    static auto PROUNDINGPOWER = CConfigValue<Config::FLOAT>("decoration:rounding_power");

    auto& window = RefRobber<Desktop::View::CWindowPresentation, Desktop::View::CWindow>::Get(this_ptr);

    float rounding = ROUNDING_LUA[&window];
    if (rounding == -1)
        return CWindowPresentationRounding_t(g_CWindowPresentationRoundingHook->m_original)(this_ptr);

    float rounding_power = window.m_ruleApplicator->roundingPower().valueOr(*PROUNDINGPOWER);
    return rounding * (rounding_power / 2.0);
}