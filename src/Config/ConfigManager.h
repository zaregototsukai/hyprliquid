#pragma once
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/desktop/rule/windowRule/WindowRuleApplicator.hpp>
#include <hyprland/src/desktop/rule/layerRule/LayerRuleEffectContainer.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <array>
#include <string_view>

class ConfigManager
{
public:
    enum ConfigType : unsigned char
    {
        ENABLED,
        BACKGROUND_SHARING,
        WATCH_SYSTEM_COLOR_SCHEME,
        EFFECT,
        CORNER_RADIUS,
        Z_RADIUS,
        GLASS_THICKNESS,
        GLASS_IOR,
        GLASS_IOR_R,
        GLASS_IOR_G,
        GLASS_IOR_B,
        GLASS_DISPERSION,
        VDF_MAP_MODE,
        VDF_MAP_UPDATE_POLICY,
        VDF_MAP_DEBUG_MODE,
        TINT_COLOR,
        BRIGHTNESS,
        HIGHLIGHT_STYLE,
        COLOR_SCHEME,
        AERO_REFLECTION_MAP_PATH,
        ROUNDING_LUA,
        FORCE_FULL_DAMAGE,

        CONFIG_LAST
    };

    enum class DataType
    {
        CONFIGDATATYPE_BOOL,
        CONFIGDATATYPE_INT,
        CONFIGDATATYPE_FLOAT,
        CONFIGDATATYPE_STR,
        CONFIGDATATYPE_COLOR
    };

    inline static constexpr std::array<std::string_view, ConfigType::CONFIG_LAST> ConfigNames
    {
        "plugin:hyprliquid:enabled",
        "plugin:hyprliquid:background_sharing",
        "plugin:hyprliquid:watch_system_color_scheme",
        "plugin:hyprliquid:effect",
        "plugin:hyprliquid:corner_radius",
        "plugin:hyprliquid:z_radius",
        "plugin:hyprliquid:glass_thickness",
        "plugin:hyprliquid:glass_ior",
        "plugin:hyprliquid:glass_ior_r",
        "plugin:hyprliquid:glass_ior_g",
        "plugin:hyprliquid:glass_ior_b",
        "plugin:hyprliquid:glass_dispersion",
        "plugin:hyprliquid:vdf_map_mode",
        "plugin:hyprliquid:vdf_map_update_policy",
        "plugin:hyprliquid:vdf_map_debug_mode",
        "plugin:hyprliquid:tint_color",
        "plugin:hyprliquid:brightness",
        "plugin:hyprliquid:highlight_style",
        "plugin:hyprliquid:color_scheme",
        "plugin:hyprliquid:aero_reflection_map_path",
        "plugin:hyprliquid:rounding_lua",
        "plugin:hyprliquid:force_full_damage",
    };

    inline static constexpr std::array<DataType, ConfigType::CONFIG_LAST> ConfigValueTypes
    {
        DataType::CONFIGDATATYPE_BOOL,
        DataType::CONFIGDATATYPE_BOOL,
        DataType::CONFIGDATATYPE_BOOL,
        DataType::CONFIGDATATYPE_INT,
        DataType::CONFIGDATATYPE_INT,
        DataType::CONFIGDATATYPE_INT,
        DataType::CONFIGDATATYPE_FLOAT,
        DataType::CONFIGDATATYPE_FLOAT,
        DataType::CONFIGDATATYPE_FLOAT,
        DataType::CONFIGDATATYPE_FLOAT,
        DataType::CONFIGDATATYPE_FLOAT,
        DataType::CONFIGDATATYPE_BOOL,
        DataType::CONFIGDATATYPE_INT,
        DataType::CONFIGDATATYPE_INT,
        DataType::CONFIGDATATYPE_INT,
        DataType::CONFIGDATATYPE_COLOR,
        DataType::CONFIGDATATYPE_FLOAT,
        DataType::CONFIGDATATYPE_INT,
        DataType::CONFIGDATATYPE_INT,
        DataType::CONFIGDATATYPE_STR,
        DataType::CONFIGDATATYPE_INT,
        DataType::CONFIGDATATYPE_BOOL
    };

    inline static std::array<SP<Config::Values::IValue>, ConfigType::CONFIG_LAST> ConfigValues;
    inline static uint16_t WindowRuleEffectIndexOffset;
    inline static uint16_t LayerRuleEffectIndexOffset;

    using Variant = std::variant<std::monostate, Config::BOOL, Config::INTEGER, Config::FLOAT, Config::STRING, CHyprColor>;

    static void Init(HANDLE handle);
    static void Destroy();
private:
    inline static Hyprutils::Signal::CHyprSignalListener m_WindowRuleUpdatedListener;
    inline static Hyprutils::Signal::CHyprSignalListener m_LayerRuleUpdatedListener;
};
