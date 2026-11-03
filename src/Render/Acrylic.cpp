#include "Render.h"
#include "Shaders/Shaders.h"
#include "Config/RuleOrConfigValue.hpp"
#include "Utils/Utils.hpp"
#include "Utils/ColorSchemeHelper.h"
#include <hyprland/src/render/Renderer.hpp>

using namespace Shaders;

void RenderAcrylic(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& texture, const SP<ITexture>& mask, const CBox &box, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data, const bool thin, const SP<ClientContext>& client_context)
{
    static auto BRIGHTNESS   = RuleOrConfigValue<Config::FLOAT>(ConfigManager::ConfigType::BRIGHTNESS);
    static auto TINT_COLOR   = RuleOrConfigValue<CHyprColor>(ConfigManager::ConfigType::TINT_COLOR);
    static auto COLOR_SCHEME = RuleOrConfigValue<Config::INTEGER>(ConfigManager::ConfigType::COLOR_SCHEME);

    static const CHyprColor LuminosityColor_Light      = MakeColor(0xEE, 0.805);
    static const CHyprColor TintColor_Light            = MakeColor(0xFF, 0.126);

    static const CHyprColor LuminosityColor_Dark       = MakeColor(0x20, 0.805);
    static const CHyprColor TintColor_Dark             = MakeColor(0xFF, 0.650);

    static const CHyprColor LuminosityColor_Thin_Light = MakeColor(0xCC, 0.500);
    static const CHyprColor TintColor_Thin_Light       = MakeColor(0xFF, 0.000);

    static const CHyprColor LuminosityColor_Thin_Dark  = MakeColor(0x20, 0.500);
    static const CHyprColor TintColor_Thin_Dark        = MakeColor(0xFF, 0.000);

    float discard_alpha = 0.0;
    if (data.currentLS)
        discard_alpha = data.currentLS->m_ruleApplicator->ignoreAlpha().valueOrDefault();

    auto tex = EnsureBlur(ctx, data.a, const_cast<CRegion*>(data.damage), texture, 4, 4);
    auto& render_data = ctx.m_data;
    const float scale = render_data.pMonitor->m_scale;

    auto  tint_color = TINT_COLOR[client_context];
    float brightness = BRIGHTNESS[client_context];
    int color_scheme = COLOR_SCHEME[client_context];
    if (color_scheme == 3)
        color_scheme = ColorSchemeHelper::GetColorScheme();

    if (tint_color.a == 0)
    {
        if (color_scheme == 1)
            tint_color = thin ? TintColor_Thin_Dark : TintColor_Dark;
        else
            tint_color = thin ? TintColor_Thin_Light : TintColor_Light;
    }

    CHyprColor luminosity_color;
    if (color_scheme == 1)
        luminosity_color = thin ? LuminosityColor_Thin_Dark : LuminosityColor_Dark;
    else
        luminosity_color = thin ? LuminosityColor_Thin_Light : LuminosityColor_Light;

    const auto& size = fb.m_size;
    auto  shader = Render::GL::g_pHyprOpenGL->useShader(g_FluentMaterialShader).lock();
    shader->setUniformFloat(SHADER_DISCARD_ALPHA_VALUE, discard_alpha);
    if (mask)
    {
        shader->setUniformFloat2((eShaderUniform)CUSTOM_SHADER_TEXTURE_SCALE, size.x / box.width, size.y / box.height);
        shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_MASK_TEXTURE, 1);
        glActiveTexture(GL_TEXTURE1);
        mask->bind();
    }
    else
        shader->setUniformFloat2((eShaderUniform)CUSTOM_SHADER_TEXTURE_SCALE, 0.0, 0.0);

    shader->setUniformFloat2(SHADER_TOP_LEFT, box.x, box.y);
    shader->setUniformFloat2(SHADER_BOTTOM_RIGHT, box.x + box.width, box.y + box.height);
    shader->setUniformFloat2(SHADER_FULL_SIZE, size.x, size.y);
    shader->setUniformFloat (SHADER_BRIGHTNESS, brightness);
    shader->setUniformFloat4((eShaderUniform)CUSTOM_SHADER_LUMINOSITY_COLOR, luminosity_color.r, luminosity_color.g, luminosity_color.b, luminosity_color.a);
    shader->setUniformFloat4(SHADER_TINT, tint_color.r, tint_color.g, tint_color.b, tint_color.a);

    RenderShader(ctx, fb, tex, shader, data.damage);
    if (mask)
        mask->unbind();

}