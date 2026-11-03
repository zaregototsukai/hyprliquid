#include "EffectDamageManager.h"
#include "Config/RuleOrDefaultValue.hpp"
#include <hyprland/src/render/Context.hpp>
#include <hyprland/src/render/gl/blur/Kawase.hpp>
#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/desktop/state/LayerState.hpp>
#include <hyprland/src/desktop/view/window/Window.hpp>
#include <hyprland/src/desktop/view/window/WindowPresentation.hpp>
#include <hyprland/src/desktop/view/LayerSurface.hpp>
#include <hyprland/src/render/WindowRenderPresentation.hpp>
#include <hyprland/src/render/scene/SceneSelection.hpp>
#include <tuple>

float EffectBackdropMargin(const int effect)
{
    constexpr float DEFAULT_BACKDROP_MARGIN = 192;

    switch (effect)
    {
        case 2:
        case 3:
            return Render::GL::dualKawaseDamageRadius(4, 4) * 1.5;

        case 4:
        case 5:
            return 0;

        case 6:
            return Render::GL::dualKawaseDamageRadius(2, 2) * 1.5;

        case 1:
        default:
            return DEFAULT_BACKDROP_MARGIN;
    }
}

void EffectDamageManager::ExpandDamageForEffects(Render::CRenderContext& ctx)
{
    static auto EFFECT = RuleOrDefaultValue<Config::INTEGER>(ConfigManager::ConfigType::EFFECT, 0);

    EffectBackdropRegion.clear();

    if (ctx.m_data.damage.empty())
        return;

    const auto monitor = ctx.m_data.pMonitor.lock();
    if (!monitor)
        return;

    std::vector<std::tuple<CBox, float, bool>> boxes;

    const auto CollectBox = [&boxes](client_t client, const CBox& box)
    {
        const auto client_context = ClientContext::GetContext(client);
        if (!client_context)
            return;

        const int effect = EFFECT[client_context];
        if (effect == 0)
            return;

        boxes.emplace_back(box, EffectBackdropMargin(effect), false);
    };

    for (const auto& window : Desktop::windowState()->windows())
    {
        if (!window || !window->mapped() || window->isHidden() || window->m_monitor.lock() != monitor)
            continue;

        const auto presentation = window->presentation().renderPresentation(Render::eSceneMode::MONITOR);
        const auto pos          = window->position(Desktop::View::IGeometric::GEOMETRIC_CURRENT) + presentation.workspaceOffset + presentation.floatingOffset;
        const auto size         = window->size(Desktop::View::IGeometric::GEOMETRIC_CURRENT);

        CBox box = { pos.x - monitor->m_position.x, pos.y - monitor->m_position.y, size.x, size.y };
        CollectBox(window.get(), box);
    }

    for (const auto& layer : Desktop::layerState()->layers())
    {
        if (!layer || !layer->mapped() || layer->m_monitor.lock() != monitor)
            continue;

        const auto pos  = layer->position(Desktop::View::IGeometric::GEOMETRIC_CURRENT);
        const auto size = layer->size(Desktop::View::IGeometric::GEOMETRIC_CURRENT);

        CBox box = { pos.x - monitor->m_position.x, pos.y - monitor->m_position.y, size.x, size.y };
        CollectBox(layer.get(), box);
    }

    if (boxes.empty())
        return;

    CRegion selected_pixel;
    CRegion selected_logical;
    CRegion reach = ctx.m_data.damage.copy();

    bool changed = true;
    while (changed)
    {
        changed = false;

        for (auto& [box, margin, selected] : boxes)
        {
            if (selected)
                continue;

            auto pixel_box = box;
            pixel_box.scale(monitor->m_scale).round();
            CRegion probe{pixel_box};
            probe.intersect(reach);
            if (probe.empty())
                continue;

            selected = true;

            CBox expanded_pixel = pixel_box;
            expanded_pixel.expand(margin);
            selected_pixel.add(expanded_pixel);

            CBox expanded_logical = box;
            expanded_logical.expand(margin / monitor->m_scale);
            selected_logical.add(expanded_logical);

            changed = true;
        }

        if (!changed)
            break;

        reach = ctx.m_data.damage.copy();
        reach.add(selected_pixel);
    }

    if (selected_pixel.empty())
        return;

    selected_pixel.intersect(CBox{{}, monitor->m_transformedSize});
    selected_logical.intersect(CBox{{}, monitor->m_size});

    EffectBackdropRegion = selected_logical;
    ctx.m_data.damage.add(selected_pixel);
}


void EffectDamageManager::Init()
{
    m_RenderStageListener = Event::bus()->m_events.render.stage.listen([](Event::SRenderStageEvent event)
    {
        if (event.stage != RENDER_BEGIN || !event.context.has_value())
            return;

        ExpandDamageForEffects(event.context->get());
    });
}

void EffectDamageManager::Destroy()
{
    m_RenderStageListener.reset();
    EffectBackdropRegion.clear();
}