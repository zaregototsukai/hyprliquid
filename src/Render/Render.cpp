#include "Render.h"
#include "Utils/Utils.hpp"
#include "Shaders/Shaders.h"
#include "Config/RuleOrConfigValue.hpp"
#include "Config/RuleOrDefaultValue.hpp"
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprland/src/render/gl/GLTexture.hpp>
#include <hyprland/src/render/gl/blur/Kawase.hpp>
#include <hyprland/src/render/ShaderLoader.hpp>
#include <hyprland/src/helpers/cm/ColorManagement.hpp>
#include <hyprutils/utils/ScopeGuard.hpp>
#include <hyprgraphics/egl/Egl.hpp>

using namespace Shaders;

template struct Infiltrator<&Render::GL::CHyprOpenGLImpl::m_blend>;
void RenderShader(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& texture, const SP<CShader>& shader, const Hyprutils::Math::CRegion* damage, const Hyprutils::Math::Vector2D& viewport)
{
    const auto  BLENDBEFORE = Robber<Render::GL::CHyprOpenGLImpl, bool>::Get(Render::GL::g_pHyprOpenGL.get());
    auto& render_data = ctx.m_data;
    const bool  custom_viewport = viewport.x > 0 && viewport.y > 0;

    const auto  TRANSFORM  = Math::wlTransformToHyprutils(Math::invertTransform(render_data.pMonitor->m_transform));
    CBox        MONITORBOX = {0, 0, render_data.pMonitor->m_transformedSize.x, render_data.pMonitor->m_transformedSize.y};
    const auto& glMatrix = g_pHyprRenderer->projectBoxToTarget(ctx, MONITORBOX, TRANSFORM);

    if (!shader)
        return;
    
    GLint prev_viewport[4] = {};
    if (custom_viewport)
        glGetIntegerv(GL_VIEWPORT, prev_viewport);

    shader->setUniformMatrix3fv(SHADER_PROJ, 1, GL_TRUE, glMatrix.getMatrix());

    fb.bind();
    dc<Render::GL::CGLFramebuffer*>(&fb)->clearAfterInvalidation();
    if (custom_viewport)
        glViewport(0, 0, viewport.x, viewport.y);

    if (texture)
    {
        shader->setUniformInt(SHADER_TEX, 0);
        glActiveTexture(GL_TEXTURE0);
        texture->bind();
    }

    glBindVertexArray(shader->getUniformLocation(SHADER_SHADER_VAO));
    Render::GL::g_pHyprOpenGL->blend(false);
    if (damage && !damage->empty())
        damage->forEachRect([&ctx](const auto& rect)
        {
            Render::GL::g_pHyprOpenGL->scissor(ctx, &rect, ctx.m_data.transformDamage);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        });
    else
    {
        Render::GL::g_pHyprOpenGL->scissor(ctx, nullptr);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    Render::GL::g_pHyprOpenGL->blend(BLENDBEFORE);

    glBindVertexArray(0);
    if (texture)
        texture->unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (custom_viewport)
        glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
}

void DebugVDFMap(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& vdf_map, const CBox& box)
{
    if (!vdf_map)
        return;

    const auto& size   = fb.m_size;
    const auto  shader = Render::GL::g_pHyprOpenGL->useShader(g_DebugVDFMapShader).lock();
    shader->setUniformFloat2(SHADER_TOP_LEFT, box.x, box.y);
    shader->setUniformFloat2(SHADER_FULL_SIZE, size.x, size.y);
    if (vdf_map->m_size.x != 0.0 && vdf_map->m_size.y != 0.0)
        shader->setUniformFloat2((eShaderUniform)CUSTOM_SHADER_TEXTURE_SCALE, size.x / box.width, size.y / box.height);
    else
        shader->setUniformFloat2((eShaderUniform)CUSTOM_SHADER_TEXTURE_SCALE, 0.0, 0.0);
    RenderShader(ctx, fb, vdf_map, shader);
    vdf_map->unbind();
}

SP<ITexture> GetOrUpdateVDFMap(Render::CRenderContext& ctx, const SP<ITexture>& texture, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data, const SP<ClientContext>& client_context)
{
    static auto VDF_MAP_MODE          = RuleOrDefaultValue<Config::INTEGER>(ConfigManager::ConfigType::VDF_MAP_MODE, 0);
    static auto VDF_MAP_UPDATE_POLICY = RuleOrConfigValue<Config::INTEGER>(ConfigManager::ConfigType::VDF_MAP_UPDATE_POLICY);
    static auto CORNER_RADIUS         = RuleOrConfigValue<Config::INTEGER>(ConfigManager::ConfigType::CORNER_RADIUS);

    if (!texture)
        return nullptr;

    if (!client_context)
        return nullptr;

    int mode = VDF_MAP_MODE[client_context];
    if (mode == 0)
        return nullptr;

    auto vdf_map = client_context->VDFMap;
    int vdf_map_update_policy = VDF_MAP_UPDATE_POLICY[client_context];

    if (vdf_map_update_policy != -1 && client_context->TextureChecksumReadback)
        client_context->TextureChecksumReadback.reset();

    if (vdf_map)
    {
        if (vdf_map_update_policy == -1)
        {
            if (!client_context->TextureChecksumReadback)
                client_context->TextureChecksumReadback = makeShared<AsyncSSBOReadback>();
            auto checksum = GetTextureChecksum(texture, client_context->TextureChecksumReadback);
            if (client_context->VDFMapChecksum != checksum)
                client_context->VDFMapChecksum = checksum;
            else
                return vdf_map;
        }
        else if (vdf_map_update_policy == 0)
            return vdf_map;
        else if (vdf_map_update_policy > 0)
        {
            using namespace std::chrono;
            auto now = steady_clock::now();
            auto duration = now - client_context->VDFMapTimestamp;
            if (duration_cast<milliseconds>(duration).count() > vdf_map_update_policy)
                client_context->VDFMapTimestamp = now;
            else
                return vdf_map;
        }
    }

    float texture_size_w = texture->m_size.x;
    float texture_size_h = texture->m_size.y;

    if (texture_size_w <= 0 || texture_size_h <= 0)
        return nullptr;

    const auto& render_data = ctx.m_data;

    SP<ITexture> tex;
    const auto& monitor_size = render_data.pMonitor->m_pixelSize;
    if (texture_size_w > monitor_size.x || texture_size_h > monitor_size.y)
    {
        tex = DownsampleTexture(texture, monitor_size, client_context);
        if (!tex)
            return nullptr;
        texture_size_w = tex->m_size.x;
        texture_size_h = tex->m_size.y;
    }
    else
        tex = texture;

    const float scale = render_data.pMonitor->m_scale;

    int corner_radius = CORNER_RADIUS[client_context];
    if (corner_radius < 0)
        corner_radius = data.round;
    else
        corner_radius *= scale;

    auto& framebuffers = *(client_context->GetJFAFramebuffers(texture_size_w, texture_size_h));

    const     float max_size = std::max(texture_size_w, texture_size_h);
    const     int   steps = std::ceil(std::log2(max_size));
    constexpr int   additional_steps = 1;
    const     int   total_steps = steps + additional_steps;

    SP<CShader>     shader;
    int             current_index = 1;

    if (mode == 1)
    {
        shader = Render::GL::g_pHyprOpenGL->useShader(g_JumpFloodingInnerInitShader).lock();
        shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_CORNER_RADIUS, corner_radius);
        RenderShader(ctx, framebuffers[1], tex, shader);
    }
    else if (mode == 2)
    {
        shader = Render::GL::g_pHyprOpenGL->useShader(g_JumpFloodingOuterInitShader).lock();
        RenderShader(ctx, framebuffers[1], tex, shader);

        shader = Render::GL::g_pHyprOpenGL->useShader(g_JumpFloodingShader).lock();
        glUniform2i(shader->getUniformLocation(SHADER_FULL_SIZE), texture_size_w, texture_size_h);
        shader->setUniformFloat((eShaderUniform)CUSTOM_SHADER_JFA_DIRECTION, 1.0);
        for (int i = 0; i < total_steps; i++)
        {
            float stride = std::pow(2, steps - i - 1);
            if (stride < 1)
                stride = 1;
            shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_JFA_STRIDE, (int)stride);
            RenderShader(ctx, framebuffers[i % 2], framebuffers[(i + 1) % 2].getTexture(), shader);
        }
        current_index = total_steps - 1;

        shader = Render::GL::g_pHyprOpenGL->useShader(g_JumpFloodingDistanceFilterShader).lock();
        shader->setUniformFloat((eShaderUniform)CUSTOM_SHADER_CORNER_RADIUS, (float)corner_radius);
        RenderShader(ctx, framebuffers[(current_index + 1) % 2], framebuffers[current_index % 2].getTexture(), shader);
        current_index++;
    }
    else
        return nullptr;

    shader = Render::GL::g_pHyprOpenGL->useShader(g_JumpFloodingShader).lock();
    glUniform2i(shader->getUniformLocation(SHADER_FULL_SIZE), texture_size_w, texture_size_h);
    shader->setUniformFloat((eShaderUniform)CUSTOM_SHADER_JFA_DIRECTION, 0.0);
    for (int i = 0; i < total_steps; i++)
    {
        float stride = std::pow(2, steps - i - 1);
        if (stride < 1)
            stride = 1;
        shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_JFA_STRIDE, (int)stride);
        RenderShader(ctx, framebuffers[(current_index + i + 1) % 2], framebuffers[(current_index + i) % 2].getTexture(), shader);
    }
    current_index += total_steps;

    shader = Render::GL::g_pHyprOpenGL->useShader(g_JumpFloodingFinalShader).lock();
    shader->setUniformFloat2(SHADER_FULL_SIZE, texture_size_w, texture_size_h);
    shader->setUniformInt((eShaderUniform)CUSTOM_SHADER_MASK_TEXTURE, 1);
    glActiveTexture(GL_TEXTURE1);
    tex->bind();
    RenderShader(ctx, framebuffers[2], framebuffers[current_index % 2].getTexture(), shader);

    vdf_map = framebuffers[2].getTexture();
    if (vdf_map_update_policy == 0)
    {
        using namespace std::chrono;
        auto now = steady_clock::now();
        auto duration = now - client_context->VDFMapTimestamp;
        if (duration > 100ms)
            client_context->VDFMap = vdf_map;
    }
    else
        client_context->VDFMap = vdf_map;
    return vdf_map;
}

SP<ITexture> DownsampleTexture(const SP<ITexture>& texture, const Hyprutils::Math::Vector2D& size, const SP<ClientContext>& client_context)
{
    if (!texture)
        return nullptr;

    if (!client_context)
        return nullptr;
    
    const auto& texture_size  = texture->m_size;
    const float scale = std::min(size.x / texture_size.x, size.y / texture_size.y);
    const Vector2D new_size(int(scale * texture_size.x), int(scale * texture_size.y));

    if (client_context->GetSize() != new_size)
        client_context->CreateDownsampleFramebuffersAndTexture(new_size);

    const auto pframebuffers = client_context->GetDownsampleFramebuffers();
    SP<ITexture> tex = client_context->GetDownsampleTexture();

    if (!pframebuffers && !tex)
        return nullptr;

    const auto& framebuffers = *pframebuffers;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->m_texID, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->m_texID, 0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers[0]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[1]);
    glBlitFramebuffer(0, 0, texture_size.x, texture_size.y,
                      0, 0, new_size.x, new_size.y,
                      GL_COLOR_BUFFER_BIT,
                      GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

void RenderPassthrough(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& texture, const Hyprutils::Math::Vector2D& viewport)
{
    if (!texture)
        return;

    GLint prev_read_fb = 0;
    GLint prev_draw_fb = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fb);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fb);

    auto shader = Render::GL::g_pHyprOpenGL->useShader(Render::GL::g_pHyprOpenGL->getShaderVariant(Render::SH_FRAG_PASSTHRURGBA)).lock();
    RenderShader(ctx, fb, texture, shader, nullptr, viewport);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_fb);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fb);
}

SP<ITexture> ResizeTexture(Render::CRenderContext& ctx, const SP<ITexture>& texture, const Hyprutils::Math::Vector2D& size)
{
    if (!texture)
        return nullptr;

    if (size.x <= 0 || size.y <= 0)
        return nullptr;

    Render::GL::CGLFramebuffer fb;
    fb.alloc(size.x, size.y);
    RenderPassthrough(ctx, fb, texture, size);

    return fb.getTexture();
}

SP<Render::ITexture> CopyTexture(Render::CRenderContext& ctx, const SP<ITexture>& texture)
{
    return ResizeTexture(ctx, texture, texture->m_size);
}

GLuint GetTextureChecksum(const SP<ITexture>& texture, SP<AsyncSSBOReadback> readback)
{
    if (!texture)
        return 0;

    if (!readback)
        return 0;

    auto checksum = readback->GetLatestResult();
    readback->PrepareWrite();
    
    auto shader = Render::GL::g_pHyprOpenGL->useShader(g_ChecksumShader);
    glActiveTexture(GL_TEXTURE0);
    texture->bind();

    const auto& size = texture->m_size;
    constexpr int STRIDE = 16;
    constexpr int LOCAL_SIZE = 16;
    const int sample_x = (size.x + STRIDE - 1) / STRIDE;
    const int sample_y = (size.y + STRIDE - 1) / STRIDE;
    const int group_x = (sample_x + LOCAL_SIZE - 1) / LOCAL_SIZE;
    const int group_y = (sample_y + LOCAL_SIZE - 1) / LOCAL_SIZE;

    glDispatchCompute(group_x, group_y, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    readback->CommitWrite();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return checksum;
}

bool IsTextureBlack(const SP<ITexture>& texture, SP<AsyncSSBOReadback> readback)
{
    if (!texture)
        return true;

    if (!readback)
        return true;

    auto output = readback->GetLatestResult();
    readback->PrepareWrite();

    auto shader = Render::GL::g_pHyprOpenGL->useShader(g_BlackDetectionShader);
    shader->setUniformInt(SHADER_TEX, 0);
    glActiveTexture(GL_TEXTURE0);
    texture->bind();

    const auto& size = texture->m_size;
    constexpr int STRIDE = 16;
    constexpr int LOCAL_SIZE = 16;
    const int sample_x = (size.x + STRIDE - 1) / STRIDE;
    const int sample_y = (size.y + STRIDE - 1) / STRIDE;
    const int group_x = (sample_x + LOCAL_SIZE - 1) / LOCAL_SIZE;
    const int group_y = (sample_y + LOCAL_SIZE - 1) / LOCAL_SIZE;

    glDispatchCompute(group_x, group_y, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    readback->CommitWrite();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return output == 0;
}

SP<IFramebuffer> BlurTextureWithDamage(Render::CRenderContext& ctx, const float a, CRegion* originalDamage, const Config::INTEGER blur_size, const Config::INTEGER blur_passes, const SP<IFramebuffer>& source)
{
    using Hyprutils::Utils::CScopeGuard;

    auto monitor = ctx.m_data.pMonitor;
    if (!monitor)
        return nullptr;

    static auto PBLURSIZE   = CConfigValue<Config::INTEGER>("decoration:blur:size");
    static auto PBLURPASSES = CConfigValue<Config::INTEGER>("decoration:blur:passes");

    const auto config_size   = *PBLURSIZE;
    const auto config_passes = *PBLURPASSES;

    CScopeGuard guard
    {
        [&config_size, &config_passes]
        {
            *PBLURSIZE.ptr()   = config_size;
            *PBLURPASSES.ptr() = config_passes;
        }
    };

    *PBLURSIZE.ptr()   = std::clamp<Config::INTEGER>(blur_size, 1, 40);
    *PBLURPASSES.ptr() = std::clamp<Config::INTEGER>(blur_passes, 1, 8);

    CRegion damage {*originalDamage};
    auto transform = Math::invertTransform(ctx.m_data.pMonitor->m_transform);
    damage.transform(Math::wlTransformToHyprutils(transform), monitor->m_transformedSize.x, monitor->m_transformedSize.y);

    static auto kawase_blur_provider = Render::GL::CDualKawaseBlurProvider(*Render::GL::g_pHyprOpenGL);

    if (!source)
    {
        const auto render_target = ctx.m_data.currentFB;
        const auto blur_source   = !ctx.m_backdropCaptures.empty() && ctx.m_backdropCaptures.back().framebuffer ? ctx.m_backdropCaptures.back().framebuffer : render_target;
        if (!blur_source || !blur_source->getTexture())
            return nullptr;

        auto render_guard = g_pHyprRenderer->bindTempFB(ctx, render_target);
        return kawase_blur_provider.blur(ctx, blur_source, a, damage);
    }

    return kawase_blur_provider.blur(ctx, source, a, damage);
}

SP<ITexture> EnsureBlur(Render::CRenderContext& ctx, const float a, CRegion *originalDamage, SP<ITexture> texture, const Config::INTEGER blur_size, const Config::INTEGER blur_passes)
{
    static auto PBLURSIZE   = CConfigValue<Config::INTEGER>("decoration:blur:size");
    static auto PBLURPASSES = CConfigValue<Config::INTEGER>("decoration:blur:passes");

    if (blur_size != *PBLURSIZE || blur_passes != *PBLURPASSES || !texture)
    {
        const auto blurred = BlurTextureWithDamage(ctx, a, originalDamage, blur_size, blur_passes);
        return blurred ? blurred->getTexture() : nullptr;
    }
    else
        return texture;
}
