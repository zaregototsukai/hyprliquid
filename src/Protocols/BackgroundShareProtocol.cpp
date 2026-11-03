#include "BackgroundShareProtocol.h"
#include "Context/ClientContext.h"
#include "Utils/BackgroundManager.h"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/protocols/core/Compositor.hpp>
#include <hyprland/src/desktop/view/LayerSurface.hpp>
#include <hyprland/src/protocols/LayerShell.hpp>
#include <hyprland/src/protocols/XDGShell.hpp>

BackgroundShareProtocolResource::BackgroundShareProtocolResource(UP<CZhyprBackgroundShareUnstableV1>&& resource)
    : m_resource{std::move(resource)}
    , SurfaceID(-1)
{
    if UNLIKELY (!good())
        return;

    m_resource->setGetBuffer([this](CZhyprBackgroundShareUnstableV1* r, uint32_t id, wl_resource* surface_resource)
    {
        auto surface = CWLSurfaceResource::fromResource(surface_resource);
        if (!surface)
            return;

        auto surface_id = surface->id();
        client_t client = nullptr;
        Hyprutils::Math::Vector2D position;
        for (const auto& layer: Desktop::layerState()->layers())
        {
            auto layer_surface = layer->m_layerSurface;
            if (!layer_surface || !layer_surface->good())
                continue;
            if (layer_surface->m_surface->id() == surface_id)
            {
                client = layer.get();
                position = layer->m_position;
                break;
            }
        }
        if (!client)
            for (const auto& window: Desktop::windowState()->windows())
            {
                auto window_surface = window->backend().surface();
                if (!window_surface || !window_surface->good())
                    continue;
                if (window_surface->id() == surface_id)
                {
                    client = window.get();
                    position = window->layoutBox().pos();
                    break;
                }
            }

        if (!client)
            return;

        auto client_context = ClientContext::GetContext(client);
        if (!client_context)
            return;

        m_session = makeUnique<CZhyprBufferSessionV1>(r->client(), 1, id);
        SurfaceID = surface_id;
        client_context->SurfaceID = surface_id;
        BackgroundManager::OnNewClient(client_context);

    });
    m_resource->setDestroy([this](CZhyprBackgroundShareUnstableV1* )   { PROTO::g_BackgroundShare->DestroyResource(this); });
    m_resource->setOnDestroy([this](CZhyprBackgroundShareUnstableV1* ) { PROTO::g_BackgroundShare->DestroyResource(this); });

}

bool BackgroundShareProtocolResource::good()
{
    return m_resource->resource();
}

CZhyprBufferSessionV1* BackgroundShareProtocolResource::GetSession()
{
    return m_session.get();
}

BackgroundShareProtocol::BackgroundShareProtocol(const wl_interface *iface, const int &ver, const std::string &name)
    : IWaylandProtocol(iface, ver, name)
{
}

void BackgroundShareProtocol::bindManager(wl_client* client, void* data, uint32_t ver, uint32_t id)
{
    const auto RESOURCE = WP<BackgroundShareProtocolResource>{m_managers.emplace_back(makeUnique<BackgroundShareProtocolResource>(makeUnique<CZhyprBackgroundShareUnstableV1>(client, ver, id)))};

    if UNLIKELY (!RESOURCE->good())
    {
        wl_client_post_no_memory(client);
        return;
    }
}

CZhyprBufferSessionV1* BackgroundShareProtocol::GetSession(int surface_id)
{
    for(const auto& r : m_managers)
        if (r->SurfaceID == surface_id)
            return r->GetSession();

    return nullptr;
}

void BackgroundShareProtocol::DestroyResource(BackgroundShareProtocolResource* res)
{
    std::erase_if(m_managers, [&](const auto& other) { return other.get() == res; });
}

