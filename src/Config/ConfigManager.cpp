#include "ConfigManager.h"
#include "Context/ClientContext.h"
#include <hyprland/src/config/values/ConfigValues.hpp>
#include <hyprland/src/config/shared/parserUtils/ParserUtils.hpp>
#include <hyprland/src/desktop/view/LayerSurface.hpp>
#include <hyprland/src/desktop/rule/layerRule/LayerRuleApplicator.hpp>
#include <hyprland/src/helpers/MiscFunctions.hpp>
#include <string>

using props_t = void*;
using WindowRuleProps = std::unordered_map<Desktop::Rule::CWindowRuleEffectContainer::storageType, UP<Desktop::Rule::CWindowRuleApplicator::SCustomPropContainer>>;
using LayerRuleProps  = std::unordered_map<Desktop::Rule::CLayerRuleEffectContainer::storageType, UP<Desktop::Rule::CLayerRuleApplicator::SCustomPropContainer>>;

void OnRuleUpdate(ClientContext& client_context, void* pprops)
{
    const auto client_type = client_context.GetType();
    for (int i = 0; i < ConfigManager::CONFIG_LAST; i++)
    {
        if (client_type == ClientType::UNKNOWN)
            continue;

        int effect_index = i + (client_type == ClientType::LAYER ? ConfigManager::LayerRuleEffectIndexOffset : ConfigManager::WindowRuleEffectIndexOffset);
        std::string* peffect = nullptr;
        if (client_type == ClientType::WINDOW)
        {
            const auto& props = *sc<WindowRuleProps*>(pprops);
            if (auto it = props.find(effect_index); it != props.end())
                peffect = &it->second->effect;
        }
        else if (client_type == ClientType::LAYER)
        {
            const auto& props = *sc<LayerRuleProps*>(pprops);
            if (auto it = props.find(effect_index); it != props.end())
                peffect = &it->second->effect;
        }

        if (!peffect)
        {
            if (client_context.RuleConfigValueCache)
                client_context.RuleConfigValueCache->at(i) = ConfigManager::Variant{};
            continue;
        }

        ConfigManager::Variant value;
        const auto data_type = ConfigManager::ConfigValueTypes[i];
        const auto& effect = *peffect;
        switch (data_type)
        {
            case ConfigManager::DataType::CONFIGDATATYPE_BOOL:
                value = truthy(effect);
                break;

            case ConfigManager::DataType::CONFIGDATATYPE_INT:
            {
                Config::INTEGER v;

                if (i == ConfigManager::VDF_MAP_UPDATE_POLICY)
                {
                    if (effect == "always")
                        value = -2;
                    else if (effect == "onchange")
                        value = -1;
                    else if (effect == "once")
                        value = 0;
                    else
                    {
                        int unit = 1;
                        if (effect.ends_with("ms"))
                            ;
                        else if (effect.ends_with("s"))
                            unit = 1000;
                        else if (effect.ends_with("min"))
                            unit = 1000 * 60;
                        else if (effect.ends_with("hour"))
                            unit = 1000 * 60 * 60;

                        auto res = std::from_chars(effect.data(), effect.data() + effect.size(), v);
                        if (res.ec == std::errc())
                            v = v * unit;
                        else
                            v = -2;

                        if (v < -2)
                            v = -2;
                        value = v;
                    }
                    break;
                }
                if (i == ConfigManager::EFFECT)
                {
                    if (effect == "none")
                        value = 0;
                    else if (effect == "liquid_glass")
                        value = 1;
                    else if (effect == "acrylic")
                        value = 2;
                    else if (effect == "acrylic_thin")
                        value = 3;
                    else if (effect == "mica")
                        value = 4;
                    else if (effect == "mica_alt")
                        value = 5;
                    else if (effect == "aero")
                        value = 6;
                    else
                    {
                        auto res = std::from_chars(effect.data(), effect.data() + effect.size(), v);
                        if (res.ec == std::errc())
                            value = v < 0 || v > 6 ? 0 : v;
                        else
                            value = 0;
                    }
                    break;
                }
                if (i == ConfigManager::COLOR_SCHEME)
                {
                    if (effect == "dark")
                        value = 1;
                    else if (effect == "light")
                        value = 2;
                    else if (effect == "follow_system")
                        value = 3;
                    else
                    {
                        auto res = std::from_chars(effect.data(), effect.data() + effect.size(), v);
                        if (res.ec == std::errc())
                            value = v < 0 || v > 3 ? 0 : v;
                        else
                            value = 0;
                    }
                    break;
                }

                auto res = std::from_chars(effect.data(), effect.data() + effect.size(), v);
                if (res.ec == std::errc())
                    value = v;
                break;
            }

            case ConfigManager::DataType::CONFIGDATATYPE_FLOAT:
            {
                Config::FLOAT v;
                auto res = std::from_chars(effect.data(), effect.data() + effect.size(), v);
                if (res.ec == std::errc())
                    value = v;
                break;
            }

            case ConfigManager::DataType::CONFIGDATATYPE_COLOR:
            {
                auto res = Config::ParserUtils::parseColor(effect);
                if (res.has_value())
                    value = CHyprColor(res.value());
                break;
            }

            case ConfigManager::DataType::CONFIGDATATYPE_STR:
            default:
                value = effect;
                break;
        }

        if (!client_context.RuleConfigValueCache)
            client_context.RuleConfigValueCache = makeUnique<std::array<ConfigManager::Variant, ConfigManager::ConfigType::CONFIG_LAST>>();
        auto& cache = *client_context.RuleConfigValueCache;
        cache[i] = value;
    }
}

void ConfigManager::Init(HANDLE handle)
{
    ConfigValues =
    {
        makeShared<Config::Values::CBoolValue> (ConfigNames[ConfigType::ENABLED].data(),                    "", true),
        makeShared<Config::Values::CBoolValue> (ConfigNames[ConfigType::BACKGROUND_SHARING].data(),         "", false),
        makeShared<Config::Values::CBoolValue> (ConfigNames[ConfigType::WATCH_SYSTEM_COLOR_SCHEME].data(),  "", false),
        makeShared<Config::Values::CIntValue>  (ConfigNames[ConfigType::EFFECT].data(),                     "", 0),
        makeShared<Config::Values::CIntValue>  (ConfigNames[ConfigType::CORNER_RADIUS].data(),              "", -1),
        makeShared<Config::Values::CIntValue>  (ConfigNames[ConfigType::Z_RADIUS].data(),                   "", -1),
        makeShared<Config::Values::CFloatValue>(ConfigNames[ConfigType::GLASS_THICKNESS].data(),            "", 500.0),
        makeShared<Config::Values::CFloatValue>(ConfigNames[ConfigType::GLASS_IOR].data(),                  "", 1.035),
        makeShared<Config::Values::CFloatValue>(ConfigNames[ConfigType::GLASS_IOR_R].data(),                "", 1.02),
        makeShared<Config::Values::CFloatValue>(ConfigNames[ConfigType::GLASS_IOR_G].data(),                "", 1.035),
        makeShared<Config::Values::CFloatValue>(ConfigNames[ConfigType::GLASS_IOR_B].data(),                "", 1.05),
        makeShared<Config::Values::CBoolValue> (ConfigNames[ConfigType::GLASS_DISPERSION].data(),           "", false),
        makeShared<Config::Values::CIntValue>  (ConfigNames[ConfigType::VDF_MAP_MODE].data(),               "", 0),
        makeShared<Config::Values::CIntValue>  (ConfigNames[ConfigType::VDF_MAP_UPDATE_POLICY].data(),      "", -2),
        makeShared<Config::Values::CIntValue>  (ConfigNames[ConfigType::VDF_MAP_DEBUG_MODE].data(),         "", 0),
        makeShared<Config::Values::CColorValue>(ConfigNames[ConfigType::TINT_COLOR].data(),                 "", 0),
        makeShared<Config::Values::CFloatValue>(ConfigNames[ConfigType::BRIGHTNESS].data(),                 "", 1.0),
        makeShared<Config::Values::CIntValue>  (ConfigNames[ConfigType::HIGHLIGHT_STYLE].data(),            "", 0),
        makeShared<Config::Values::CIntValue>  (ConfigNames[ConfigType::COLOR_SCHEME].data(),               "", 0),
        makeShared<Config::Values::String>     (ConfigNames[ConfigType::AERO_REFLECTION_MAP_PATH].data(),   "", ""),
        makeShared<Config::Values::CIntValue>  (ConfigNames[ConfigType::ROUNDING_LUA].data(),               "", -1),
        makeShared<Config::Values::CBoolValue> (ConfigNames[ConfigType::FORCE_FULL_DAMAGE].data(),          "", false)
    };

    for (const auto& cv : ConfigValues)
        HyprlandAPI::addConfigValueV2(handle, cv);

    auto window_effects = Desktop::Rule::windowEffects();
    WindowRuleEffectIndexOffset = window_effects->registerEffect(std::string(ConfigNames[0].substr(7)));
    for (int i = 1; i < ConfigType::CONFIG_LAST; i++)
        window_effects->registerEffect(std::string(ConfigNames[i].substr(7)));

    auto layer_effects = Desktop::Rule::layerEffects();
    LayerRuleEffectIndexOffset = layer_effects->registerEffect(std::string(ConfigNames[0].substr(7)));
    for (int i = 1; i < ConfigType::CONFIG_LAST; i++)
        layer_effects->registerEffect(std::string(ConfigNames[i].substr(7)));

    m_WindowRuleUpdatedListener = Event::bus()->m_events.window.updateRules.listen([&](PHLWINDOW window)
    {
        void* client = window.get();
        auto client_context = ClientContext::GetContext(client);
        if (!client_context)
        {
            client_context = ClientContext::CreateContext(client, ClientType::WINDOW);
            if (!client_context)
                return;
        }
        auto& props = window->m_ruleApplicator->m_otherProps.props;
        OnRuleUpdate(*client_context, &props);
    });

    m_LayerRuleUpdatedListener = Event::bus()->m_events.layer.updateRules.listen([&](PHLLS layer)
    {
        void* client = layer.get();
        auto client_context = ClientContext::GetContext(client);
        if (!client_context)
        {
            client_context = ClientContext::CreateContext(client, ClientType::LAYER);
            if (!client_context)
                return;
        }
        auto& props = layer->m_ruleApplicator->m_otherProps.props;
        OnRuleUpdate(*client_context, &props);
    });
}

void ConfigManager::Destroy()
{
    m_WindowRuleUpdatedListener.reset();
    m_LayerRuleUpdatedListener.reset();

    auto window_effects = Desktop::Rule::windowEffects();
    auto layer_effects = Desktop::Rule::layerEffects();
    for (int i = 0; i < ConfigType::CONFIG_LAST; i++)
    {
        window_effects->unregisterEffect(WindowRuleEffectIndexOffset + i);
        layer_effects->unregisterEffect(LayerRuleEffectIndexOffset + i);
    }
}
