#include "Render.h"
#include "Shaders/Shaders.h"
#include "Config/RuleOrConfigValue.hpp"
#include "Utils/Utils.hpp"
#include "Utils/BackgroundManager.h"
#include "Utils/ColorSchemeHelper.h"
#include <hyprland/src/render/Renderer.hpp>


using namespace Shaders;

void RenderAero(IFramebuffer& fb, const SP<ITexture>& texture, const SP<ITexture>& mask, const CBox& box, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data, const SP<ClientContext>& client_context)
{
    using Render::GL::CGLFramebuffer;

    static auto CORNER_RADIUS = RuleOrConfigValue<Config::INTEGER>(ConfigManager::ConfigType::CORNER_RADIUS);
    static auto BRIGHTNESS    = RuleOrConfigValue<Config::FLOAT>(ConfigManager::ConfigType::BRIGHTNESS);
    static auto TINT_COLOR    = RuleOrConfigValue<CHyprColor>(ConfigManager::ConfigType::TINT_COLOR);

    static const CHyprColor LuminosityColor = MakeColor(0x4C, 0.4);
    static const CHyprColor TintColor       = MakeColor(0xE5, 0xE5, 0xFF, 0.2);

    float discard_alpha = 0.0;
    if (data.currentLS)
        discard_alpha = data.currentLS->m_ruleApplicator->ignoreAlpha().valueOrDefault();

    auto tex = EnsureBlur(data.a, const_cast<CRegion*>(data.damage), texture, 2, 2);

    const float scale = g_pHyprRenderer->m_renderData.pMonitor->m_scale;

    int corner_radius = CORNER_RADIUS[client_context];
    if (corner_radius < 0)
        corner_radius = data.round;
    else
        corner_radius *= scale;

    auto  tint_color = TINT_COLOR[client_context];
    float brightness = BRIGHTNESS[client_context];

    if (tint_color.a == 0)
        tint_color = TintColor;
    const CHyprColor& luminosity_color = LuminosityColor;

    auto reflection_map = BackgroundManager::GetAeroReflectionMap();

    const auto& size = fb.m_size;
    const auto  shader = Render::GL::g_pHyprOpenGL->useShader(g_AeroShader).lock();
    shader->setUniformFloat(SHADER_DISCARD_ALPHA_VALUE, discard_alpha);
    shader->setUniformFloat2(SHADER_TOP_LEFT, box.x, box.y);
    shader->setUniformFloat2(SHADER_BOTTOM_RIGHT, box.x + box.width, box.y + box.height);
    shader->setUniformFloat2(SHADER_FULL_SIZE, size.x, size.y);
    shader->setUniformFloat (SHADER_BRIGHTNESS, brightness);
    shader->setUniformFloat4((eShaderUniform)CUSTOM_SHADER_LUMINOSITY_COLOR, luminosity_color.r, luminosity_color.g, luminosity_color.b, luminosity_color.a);
    shader->setUniformFloat4(SHADER_TINT, tint_color.r, tint_color.g, tint_color.b, tint_color.a);

    if (reflection_map)
    {
        shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_REFLECTION_MAP, 1);
        glActiveTexture(GL_TEXTURE1);
        reflection_map->bind();
    }
    if (mask)
    {
        shader->setUniformFloat2((eShaderUniform)CUSTOM_SHADER_TEXTURE_SCALE, size.x / box.width, size.y / box.height);
        shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_MASK_TEXTURE, 2);
        glActiveTexture(GL_TEXTURE2);
        mask->bind();
    }
    else
        shader->setUniformFloat2((eShaderUniform)CUSTOM_SHADER_TEXTURE_SCALE, 0.0, 0.0);

    RenderShader(fb, tex, shader, data.damage);
    if (reflection_map)
        reflection_map->unbind();
    if (mask)
        mask->unbind();

}
