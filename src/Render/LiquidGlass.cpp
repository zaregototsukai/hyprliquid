#include "Render.h"
#include "Shaders/Shaders.h"
#include "Config/RuleOrConfigValue.hpp"
#include <hyprland/src/render/Renderer.hpp>

using namespace Shaders;

void RenderLiquidGlass(IFramebuffer& fb, const SP<ITexture>& texture, const CBox& box, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data, const SP<ITexture>& vdf_map, const SP<ClientContext>& client_context)
{
    static auto CORNER_RADIUS    = RuleOrConfigValue<Config::INTEGER>(ConfigManager::ConfigType::CORNER_RADIUS);
    static auto Z_RADIUS         = RuleOrConfigValue<Config::INTEGER>(ConfigManager::ConfigType::Z_RADIUS);
    static auto GLASS_IOR        = RuleOrConfigValue<Config::FLOAT>(ConfigManager::ConfigType::GLASS_IOR);
    static auto GLASS_THICKNESS  = RuleOrConfigValue<Config::FLOAT>(ConfigManager::ConfigType::GLASS_THICKNESS);
    static auto GLASS_IOR_R      = RuleOrConfigValue<Config::FLOAT>(ConfigManager::ConfigType::GLASS_IOR_R);
    static auto GLASS_IOR_G      = RuleOrConfigValue<Config::FLOAT>(ConfigManager::ConfigType::GLASS_IOR_G);
    static auto GLASS_IOR_B      = RuleOrConfigValue<Config::FLOAT>(ConfigManager::ConfigType::GLASS_IOR_B);
    static auto GLASS_DISPERSION = RuleOrConfigValue<Config::BOOL>(ConfigManager::ConfigType::GLASS_DISPERSION);
    static auto BRIGHTNESS       = RuleOrConfigValue<Config::FLOAT>(ConfigManager::ConfigType::BRIGHTNESS);
    static auto TINT_COLOR       = RuleOrConfigValue<CHyprColor>(ConfigManager::ConfigType::TINT_COLOR);
    static auto HIGHLIGHT_STYLE  = RuleOrConfigValue<Config::INTEGER>(ConfigManager::ConfigType::HIGHLIGHT_STYLE);

    const float scale = g_pHyprRenderer->m_renderData.pMonitor->m_scale;
    const float rounding_power = data.roundingPower;

    int corner_radius = CORNER_RADIUS[client_context];
    if (corner_radius < 0)
        corner_radius = data.round;
    else
        corner_radius *= scale * rounding_power / 2;
    if (data.round > 0 && corner_radius > data.round)
        corner_radius = data.round;

    int z_radius = Z_RADIUS[client_context];
    if (z_radius < 0)
        z_radius = corner_radius;
    else
        z_radius *= scale * rounding_power / 2;
    if (z_radius > corner_radius)
        z_radius = corner_radius;

    const auto  tint_color       = TINT_COLOR[client_context];
    const float brightness       = BRIGHTNESS[client_context];
    const int   highlight_style  = HIGHLIGHT_STYLE[client_context];
    const bool  glass_dispersion = GLASS_DISPERSION[client_context];

    const auto& size = fb.m_size;
    const auto  shader = Render::GL::g_pHyprOpenGL->useShader(g_LiquidGlassShader).lock();
    shader->setUniformFloat2(SHADER_TOP_LEFT, box.x, box.y);
    shader->setUniformFloat2(SHADER_BOTTOM_RIGHT, box.x + box.width, box.y + box.height);
    shader->setUniformFloat2(SHADER_FULL_SIZE, size.x, size.y);

    shader->setUniformFloat((eShaderUniform)CUSTOM_SHADER_GLASS_IOR, GLASS_IOR[client_context]);
    shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_GLASS_DISPERSION, glass_dispersion);
    if (glass_dispersion)
        shader->setUniformFloat3((eShaderUniform)CUSTOM_SHADER_GLASS_IOR_RGB, GLASS_IOR_R[client_context], GLASS_IOR_G[client_context], GLASS_IOR_B[client_context]);

    shader->setUniformFloat((eShaderUniform)CUSTOM_SHADER_CORNER_RADIUS, corner_radius);
    shader->setUniformFloat((eShaderUniform)CUSTOM_SHADER_Z_RADIUS, z_radius);
    shader->setUniformFloat((eShaderUniform)CUSTOM_SHADER_GLASS_THICKNESS, GLASS_THICKNESS[client_context] * scale);
    shader->setUniformInt  ((eShaderUniform)CUSTOM_SHADER_HIGHLIGHT_STYLE, highlight_style);

    shader->setUniformFloat4(SHADER_TINT, tint_color.r, tint_color.g, tint_color.b, tint_color.a);
    shader->setUniformFloat (SHADER_BRIGHTNESS, brightness);
    shader->setUniformFloat (SHADER_ROUNDING_POWER, rounding_power);

    if (vdf_map)
    {
        if (vdf_map->m_size.x != 0.0 && vdf_map->m_size.y != 0.0)
            shader->setUniformFloat2((eShaderUniform)CUSTOM_SHADER_TEXTURE_SCALE, size.x / box.width, size.y / box.height);
        else
            shader->setUniformFloat2((eShaderUniform)CUSTOM_SHADER_TEXTURE_SCALE, 0.0, 0.0);
        shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_HAS_VDF_MAP, true);
        shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_VDF_MAP, 1);
        glActiveTexture(GL_TEXTURE1);
        vdf_map->bind();
    }
    else
        shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_HAS_VDF_MAP, false);
    RenderShader(fb, texture, shader, data.damage);
    if (vdf_map)
        vdf_map->unbind();
}
