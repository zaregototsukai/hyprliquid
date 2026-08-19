#include "Hooks/Hooks.h"
#include "Shaders/Shaders.h"
#include "Protocols/BackgroundShareProtocol.h"
#include "Config/ConfigManager.h"
#include "Context/ClientContext.h"
#include "Context/MonitorContext.h"
#include "Render/Render.h"
#include "Utils/Utils.hpp"
#include "Utils/BackgroundManager.h"
#include "Utils/ColorSchemeHelper.h"
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/desktop/view/LayerSurface.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#include <hyprland/src/managers/WelcomeManager.hpp>
#include <hyprland/src/config/ConfigManager.hpp>

const std::array<std::tuple<const char*, CFunctionHook**, void*>, 3> Hooks
{
    std::tuple {"IElementRenderer11drawSurface",                  &g_IElementRendererDrawSurfaceHook,                  (void*)HookIElementRendererDrawSurface},
    std::tuple {"CHyprOpenGLImpl21renderTextureInternal",         &g_CHyprOpenGLImplRenderTextureInternalHook,         (void*)HookCHyprOpenGLImplRenderTextureInternal},
    std::tuple {"CHyprOpenGLImpl29renderTextureWithBlurInternal", &g_CHyprOpenGLImplRenderTextureWithBlurInternalHook, (void*)HookCHyprOpenGLImplRenderTextureWithBlurInternal}
};

static CHyprSignalListener HyprlandReadyListener;

void OnHyprlandReady(HANDLE handle)
{
    HyprlandReadyListener.reset();

    if (!Shaders::IsInitialized)
        Shaders::Init();

    if (!Shaders::IsInitialized)
    {
        const std::string message = "[hyprliquid] Failed to create shaders!";
        HyprlandAPI::addNotification(handle, message, CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        Log::logger->log(Log::ERR, message);
        return;
    }

    if (!ColorSchemeHelper::IsInitialized())
        ColorSchemeHelper::Init();

    if (!BackgroundManager::IsEGLInitialized())
    {
        const std::string extensions = eglQueryString(Render::GL::g_pHyprOpenGL->m_eglDisplay, EGL_EXTENSIONS);
        if (extensions.contains("EGL_MESA_image_dma_buf_export") && BackgroundManager::InitEGL())
            PROTO::g_BackgroundShare = makeUnique<BackgroundShareProtocol>(&zhypr_background_share_unstable_v1_interface, 1, "BackgroundShare");
    }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle)
{
    const std::string COMPOSITOR_HASH = __hyprland_api_get_hash();
    const std::string CLIENT_HASH = __hyprland_api_get_client_hash();

    if (COMPOSITOR_HASH != CLIENT_HASH)
    {
        HyprlandAPI::addNotification(handle, "[hyprliquid] Mismatched headers! Can't proceed.", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprliquid] Version mismatch.");
    }

    MonitorContext::Init();
    ClientContext::Init();
    ConfigManager::Init(handle);
    Shaders::Init();
    BackgroundManager::Init();

    if (!std::filesystem::exists(g_pCompositor->m_instancePath + "/hyprland.lock") || !g_pWelcomeManager)
        HyprlandReadyListener = Event::bus()->m_events.ready.listen([handle] { OnHyprlandReady(handle); });
    else
        HyprlandReadyListener = Event::bus()->m_events.config.reloaded.listen([handle] { OnHyprlandReady(handle); });

    for (const auto& [symbol, hook, function] : Hooks)
    {
        auto matches = HyprlandAPI::findFunctionsByName(handle, symbol);
        if (matches.empty())
        {
            auto message = std::format("[hyprliquid] Failed to find symbol: \"{}\".", symbol);
            Log::logger->log(Log::ERR, message);
            throw std::runtime_error(message);
        }

        auto& match = matches.front();
        *hook = HyprlandAPI::createFunctionHook(handle, match.address, function);
        if (!(*hook)->hook())
        {
            auto message = std::format("[hyprliquid] Failed to hook function: \"{}\".", symbol);
            Log::logger->log(Log::ERR, message);
            throw std::runtime_error(message);
        }
    }

    if (Config::mgr()->type() == Config::CONFIG_LUA)
    {
        HyprlandAPI::addLuaFunction(handle, "hyprliquid", "placeholder", [](lua_State* L) { return 0; });

        auto matches = HyprlandAPI::findFunctionsByName(handle, "Desktop4View7CWindow8roundingEv");
        if (!matches.empty())
        {
            auto& match = matches.front();
            g_CWindowRoundingHook = HyprlandAPI::createFunctionHook(handle, match.address, (void*)HookCWindowRounding);
            g_CWindowRoundingHook->hook();
        }
    }

    return { "hyprliquid", "Liquid Glass, Acrylic, Mica, and Aero material effects for Hyprland.", "zaregototsukai", "0.1.1" };
}

APICALL EXPORT void PLUGIN_EXIT()
{
    ColorSchemeHelper::Destroy();
    BackgroundManager::Destroy();
    BackgroundManager::DestroyEGL();
    MonitorContext::Destroy();
    ClientContext::Destroy();
    ConfigManager::Destroy();
    Shaders::Destroy();
    HyprlandReadyListener.reset();
    PROTO::g_BackgroundShare.reset();
}

APICALL EXPORT std::string PLUGIN_API_VERSION()
{
    return HYPRLAND_API_VERSION;
}