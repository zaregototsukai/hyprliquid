#include "BackgroundManager.h"
#include "Utils.hpp"
#include "Render/Render.h"
#include "Render/Pass/BackgroundSharePassElement.h"
#include "Context/MonitorContext.h"
#include "Shaders/Shaders.h"
#include "Protocols/BackgroundShareProtocol.h"
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/gl/GLTexture.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprland/protocols/wlr-layer-shell-unstable-v1.hpp>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC eglExportDMABUFImageQueryMESA = nullptr;
PFNEGLEXPORTDMABUFIMAGEMESAPROC      eglExportDMABUFImageMESA      = nullptr;

void BackgroundManager::DestroySharedBackgroundResource(MonitorContext* monitor_context)
{
    static auto eglDestroyImageKHR = Render::GL::g_pHyprOpenGL->m_proc.eglDestroyImageKHR;

    if (!monitor_context)
        return;

    if (!monitor_context->SharedBackgroundEGLResource)
        return;

    const auto fd = monitor_context->SharedBackgroundEGLResource->FD;
    if (fd)
        ::close(fd);

    const auto& egl_display = Render::GL::g_pHyprOpenGL->m_eglDisplay;
    const auto& image = monitor_context->SharedBackgroundEGLResource->EGLImage;
    if (eglDestroyImageKHR)
        eglDestroyImageKHR(egl_display, image);

    monitor_context->SharedBackgroundEGLResource.reset();
    monitor_context->SharedBackgroundFramebuffer.reset();
}

void BackgroundManager::Init()
{
    SharingBackground = false;
    m_BackgroundChangedListener = m_BackgroundChanged.listen([](const std::pair<PHLMONITOR, SP<Render::ITexture>>& data)
    {
        auto& [monitor, texture] = data;
        CreateBlurredBackground(monitor, texture);
    });

    m_RenderStageListener = Event::bus()->m_events.render.stage.listen([](Event::SRenderStageEvent event)
    {
        if (!event.context.has_value())
            return;
        
        const auto& stage = event.stage;
        auto& ctx = event.context->get();
        
        if (stage != RENDER_POST_WALLPAPER)
            return;

        if (SharingBackground)
            g_pHyprRenderer->addPassElement(ctx, makeUnique<CBackgroundSharePassElement>());

        auto monitor = ctx.m_data.pMonitor.lock();
        if (!monitor->m_layerSurfaceLayers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND].empty())
            return;

        auto monitor_context = MonitorContext::GetContext(monitor.get());
        if (!monitor_context)
            return;

        auto state = monitor_context->GetBackgroundState();
        if (state == MonitorBackgroundState::LAYERSHELL)
        {
            auto new_texture = monitor->m_background ? monitor->m_background : nullptr;
            auto new_state   = new_texture ? MonitorBackgroundState::INTRINSIC : MonitorBackgroundState::NONE;

            m_BackgroundChanged.emit({monitor, new_texture});
            monitor_context->SetBackgroundState(new_state);
        }
        else if (state == MonitorBackgroundState::INTRINSIC && !monitor->m_background)
        {
            m_BackgroundChanged.emit({monitor, nullptr});
            monitor_context->SetBackgroundState(MonitorBackgroundState::NONE);
        }
        else if (state == MonitorBackgroundState::NONE && monitor->m_background)
        {
            m_BackgroundChanged.emit({monitor, monitor->m_background});
            monitor_context->SetBackgroundState(MonitorBackgroundState::INTRINSIC);
        }
    });
}

void BackgroundManager::Destroy()
{
    m_EGLInitialized = false;
    m_BackgroundChangedListener.reset();
    m_RenderStageListener.reset();
    m_CustomAeroReflectionMap.reset();
    m_DefaultAeroReflectionMap.reset();
}

void BackgroundManager::CreateBlurredBackground(PHLMONITOR monitor, SP<Render::ITexture> texture)
{
    auto monitor_context = MonitorContext::GetContext(monitor.get());
    if (!texture)
    {
        monitor_context->SetBlurredBackground(nullptr);
        return;
    }

    GLint prev_read_fbo = 0, prev_draw_fbo = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fbo);

    auto&   ctx = g_pHyprRenderer->context();
    CRegion damage {0, 0, monitor->m_pixelSize.x, monitor->m_pixelSize.y};

    auto source = makeShared<Render::GL::CGLFramebuffer>();
    if (!source->alloc(monitor->m_pixelSize.x, monitor->m_pixelSize.y))
        return;
    RenderPassthrough(ctx, *source, texture, monitor->m_pixelSize);

    const auto blurred = BlurTextureWithDamage(ctx, 1.0, &damage, 8, 5, source);
    monitor_context->SetBlurredBackground(blurred ? CopyTexture(ctx, blurred->getTexture()) : nullptr);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fbo);
}

void BackgroundManager::ShareBackground(PHLMONITOR monitor, SP<Render::ITexture> texture)
{
    if (!SharingBackground)
        return;

    if (!PROTO::g_BackgroundShare)
        return;

    auto monitor_context = MonitorContext::GetContext(monitor.get());
    if (!monitor_context)
        return;

    if (EGLClients.empty())
    {
        if (monitor_context->SharedBackgroundEGLResource)
            DestroySharedBackgroundResource(monitor_context.get());
        return;
    }

    const auto& size = monitor->m_pixelSize;
    if (auto& egl_resource = monitor_context->SharedBackgroundEGLResource)
        if (egl_resource->Width != size.x || egl_resource->Height != size.y)
        {
            DestroySharedBackgroundResource(monitor_context.get());
            for (auto& client : EGLClients)
            {
                auto client_context = client.lock();
                if (client_context && client_context->GetMonitor().get() == monitor.get())
                    client_context->GotBackground = false;
            }
        }

    if (!monitor_context->SharedBackgroundEGLResource)
    {
        auto fb = monitor_context->SharedBackgroundFramebuffer;
        fb = makeShared<Render::GL::CGLFramebuffer>();
        fb->alloc(size.x, size.y);
        auto tex = fb->getTexture();
        auto resource = CreateEGLImage(tex->m_texID, size.x, size.y);
        if (!resource)
        {
            monitor_context->SharedBackgroundEGLResource = nullptr;
            monitor_context->SharedBackgroundFramebuffer = nullptr;
            return;
        }
        monitor_context->SharedBackgroundEGLResource = resource;
        monitor_context->SharedBackgroundFramebuffer = fb;
    }

    RenderPassthrough(g_pHyprRenderer->context(), *monitor_context->SharedBackgroundFramebuffer, texture, size);
    monitor_context->SetSharedBackground(monitor_context->SharedBackgroundFramebuffer->getTexture());

    for (auto iter = EGLClients.begin(); iter != EGLClients.end(); )
    {
        if (iter->expired())
        {
            iter = EGLClients.erase(iter);
            continue;
        }

        auto client_context = iter->lock();
        Vector2D position;

        if (client_context->GetType() == ClientType::LAYER)
        {
            auto layer = rc<Desktop::View::CLayerSurface*>(client_context->GetClient());
            if (layer->m_monitor.get() != monitor.get())
            {
                iter++;
                continue;
            }
            position = layer->m_position;
        }
        else if (client_context->GetType() == ClientType::WINDOW)
        {
            auto window = rc<Desktop::View::CWindow*>(client_context->GetClient());
            if (window->m_monitor.get() != monitor.get())
            {
                iter++;
                continue;
            }
            position = window->layoutBox().pos();
        }
        else
        {
            iter++;
            continue;
        }

        const int& surface_id = client_context->SurfaceID;
        CZhyprBufferSessionV1* session = nullptr;
        if (!client_context->GotBackground)
        {
            session = PROTO::g_BackgroundShare->GetSession(surface_id);
            if (!session)
            {
                iter++;
                continue;
            }
            auto buffer_resource = monitor_context->SharedBackgroundEGLResource;
            auto [fd, width, height, fourcc, stride, offset, modifier_low, modifier_high, _] = *buffer_resource;
            session->sendBuffer(fd, width, height, fourcc, stride, offset, modifier_low, modifier_high);
            client_context->GotBackground = true;
        }

        position *= monitor->m_scale;
        if (position != client_context->Position)
        {
            if (!session)
                session = PROTO::g_BackgroundShare->GetSession(surface_id);
            if (!session)
            {
                iter++;
                continue;
            }
            client_context->Position = position;
            session->sendPosition(position.x, position.y);
        }

        iter++;
    }
}

SP<Render::ITexture> BackgroundManager::GetAeroReflectionMap()
{
    static auto AERO_REFLECTION_MAP_PATH = CConfigValue<Config::STRING>(std::string(ConfigManager::ConfigNames[ConfigManager::ConfigType::AERO_REFLECTION_MAP_PATH]));
    static std::string CURRENT_PATH = "";
    static bool INVALID_PATH = false;

    const std::string path = *AERO_REFLECTION_MAP_PATH;
    if (path.empty())
        return GetDefaultAeroReflectionMap();

    if (CURRENT_PATH != path)
    {
        CreateCustomAeroReflectionMap();
        CURRENT_PATH = path;
        INVALID_PATH = GetCustomAeroReflectionMap() == nullptr;
    }

    if (INVALID_PATH)
        return GetDefaultAeroReflectionMap();

    return GetCustomAeroReflectionMap();
}


void BackgroundManager::CreateDefaultAeroReflectionMap()
{
    GLint prev_fbo = 0;
    GLboolean prev_color_mask[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    glGetBooleanv(GL_COLOR_WRITEMASK, prev_color_mask);

    const Hyprutils::Math::Vector2D size {800, 600};
    Render::GL::CGLFramebuffer fb;

    fb.alloc(size.x, size.y);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    auto shader = Render::GL::g_pHyprOpenGL->useShader(Shaders::g_AeroReflectionMapShader).lock();
    RenderShader(g_pHyprRenderer->context(), fb, nullptr, shader, nullptr, size);

    glColorMask(prev_color_mask[0], prev_color_mask[1], prev_color_mask[2], prev_color_mask[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);

    m_DefaultAeroReflectionMap = fb.getTexture();
}

SP<Render::ITexture> BackgroundManager::GetDefaultAeroReflectionMap()
{
    if (m_DefaultAeroReflectionMap)
        return m_DefaultAeroReflectionMap;

    CreateDefaultAeroReflectionMap();
    return m_DefaultAeroReflectionMap;
}

void BackgroundManager::CreateCustomAeroReflectionMap()
{
    static auto AERO_REFLECTION_MAP_PATH = CConfigValue<Config::STRING>(std::string(ConfigManager::ConfigNames[ConfigManager::ConfigType::AERO_REFLECTION_MAP_PATH]));

    const std::string path = *AERO_REFLECTION_MAP_PATH;
    int width, height, channels;
    auto data = stbi_load(path.data(), &width, &height, &channels, 4);

    if (!data)
    {
        m_CustomAeroReflectionMap = nullptr;
        return;
    }

    auto texture = makeShared<Render::GL::CGLTexture>();
    texture->allocate({width, height});
    texture->bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    texture->unbind();
    stbi_image_free(data);

    m_CustomAeroReflectionMap = texture;
}

SP<Render::ITexture> BackgroundManager::GetCustomAeroReflectionMap()
{
    return m_CustomAeroReflectionMap;
}

bool BackgroundManager::IsEGLInitialized()
{
    return m_EGLInitialized;
}

bool BackgroundManager::InitEGL()
{
    static auto BACKGROUND_SHARING = CConfigValue<Config::BOOL>(std::string(ConfigManager::ConfigNames[ConfigManager::ConfigType::BACKGROUND_SHARING]));

    if (!*BACKGROUND_SHARING)
        return false;

    eglExportDMABUFImageQueryMESA = (PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC)eglGetProcAddress("eglExportDMABUFImageQueryMESA");
    if (!eglExportDMABUFImageQueryMESA)
        return false;

    eglExportDMABUFImageMESA = (PFNEGLEXPORTDMABUFIMAGEMESAPROC)eglGetProcAddress("eglExportDMABUFImageMESA");
    if (!eglExportDMABUFImageMESA)
        return false;

    SharingBackground = true;
    m_EGLInitialized = true;
    return true;
}

void BackgroundManager::DestroyEGL()
{
    m_EGLInitialized = false;
    SharingBackground = false;
    EGLClients.clear();
    eglExportDMABUFImageQueryMESA = nullptr;
    eglExportDMABUFImageMESA = nullptr;
}

SP<EGLBufferResource> BackgroundManager::CreateEGLImage(const GLint texture, const uint32_t width, const uint32_t height)
{
    static auto eglCreateImageKHR  = Render::GL::g_pHyprOpenGL->m_proc.eglCreateImageKHR;
    static auto eglDestroyImageKHR = Render::GL::g_pHyprOpenGL->m_proc.eglDestroyImageKHR;

    if (!eglCreateImageKHR)
        return nullptr;

    if (!m_EGLInitialized)
        return nullptr;

    if (!SharingBackground)
        return nullptr;

    EGLClientBuffer client_buffer = (EGLClientBuffer)(uintptr_t)texture;

    EGLint attribs[]
    {
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_NONE
    };

    const auto& egl_display = Render::GL::g_pHyprOpenGL->m_eglDisplay;
    const auto& egl_context = Render::GL::g_pHyprOpenGL->m_eglContext;
    auto image = eglCreateImageKHR(egl_display, egl_context, EGL_GL_TEXTURE_2D_KHR, client_buffer, attribs);
    if (image == EGL_NO_IMAGE_KHR)
        return nullptr;

    int fourcc;
    int num_planes;
    EGLuint64KHR modifier;
    if (!eglExportDMABUFImageQueryMESA(egl_display, image, &fourcc, &num_planes, &modifier) || num_planes != 1)
    {
        if (eglDestroyImageKHR)
            eglDestroyImageKHR(egl_display, image);
        return nullptr;
    }

    int fd = 0;
    EGLint stride;
    EGLint offset;
    if (!eglExportDMABUFImageMESA(egl_display, image, &fd, &stride, &offset))
    {
        if (eglDestroyImageKHR)
            eglDestroyImageKHR(egl_display, image);
        if (fd)
            ::close(fd);
        return nullptr;
    }

    return makeShared<EGLBufferResource>(EGLBufferResource
    {
        .FD = fd,
        .Width = width,
        .Height = height,
        .Fourcc = fourcc,
        .Stride = stride,
        .Offset = offset,
        .ModifierLow = (uint32_t)(modifier & 0xFFFFFFFF),
        .ModifierHigh = (uint32_t)(modifier >> 32),
        .EGLImage = image
    });

}

void BackgroundManager::OnNewClient(SP<ClientContext> client_context)
{
    EGLClients.push_back(client_context);
}
