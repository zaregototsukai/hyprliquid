#include "ClientContext.h"
#include <hyprland/protocols/wlr-layer-shell-unstable-v1.hpp>
#include <hyprland/src/desktop/view/window/Window.hpp>
#include <hyprland/src/desktop/view/LayerSurface.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/desktop/state/LayerState.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/render/gl/GLTexture.hpp>

UP<CHyprSignalListener> g_WindowOpened;
UP<CHyprSignalListener> g_WindowClosed;
UP<CHyprSignalListener> g_LayerOpened;
UP<CHyprSignalListener> g_LayerClosed;

void ClientContext::Init()
{
    for (const auto window : Desktop::windowState()->windows())
        ClientContext::CreateContext(window.get(), ClientType::WINDOW);

    for (const auto layer : Desktop::layerState()->layers())
        if (layer->m_layer != ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND)
            ClientContext::CreateContext(layer.get(), ClientType::LAYER);

    g_WindowOpened = makeUnique<CHyprSignalListener>(Event::bus()->m_events.window.open.listen([&](const PHLWINDOW& window)
    {
        void* client = window.get();
        auto client_context = ClientContext::GetContext(client);
        if (!client_context)
            client_context = ClientContext::CreateContext(client, ClientType::WINDOW);
    }));

    g_LayerOpened = makeUnique<CHyprSignalListener>(Event::bus()->m_events.layer.opened.listen([&](const PHLLS& layer)
    {
        void* client = layer.get();
        auto client_context = ClientContext::GetContext(client);
        if (!client_context)
            client_context = ClientContext::CreateContext(client, ClientType::LAYER);
    }));

    g_WindowClosed = makeUnique<CHyprSignalListener>(Event::bus()->m_events.window.close.listen([&](const PHLWINDOW& window)
    {
        g_ClientContexts.erase(window.get());
    }));

    g_LayerClosed = makeUnique<CHyprSignalListener>(Event::bus()->m_events.layer.closed.listen([&](const PHLLS& layer)
    {
        g_ClientContexts.erase(layer.get());
    }));
}

void ClientContext::Destroy()
{
    g_WindowOpened.reset();
    g_LayerOpened.reset();
    g_WindowClosed.reset();
    g_LayerClosed.reset();
    g_ClientContexts.clear();
}

SP<ClientContext> ClientContext::GetContext(const client_t client)
{
    if (client == nullptr)
        return nullptr;
    auto iter = g_ClientContexts.find(client);
    if (iter == g_ClientContexts.end())
        return nullptr;
    return iter->second;
}

SP<ClientContext> ClientContext::CreateContext(const client_t client, const ClientType client_type)
{
    return g_ClientContexts.emplace(client, makeShared<ClientContext>(client, client_type)).first->second;
}

ClientContext::ClientContext(const client_t client, const ClientType client_type)
    : m_Client{client}
    , m_ClientType{client_type}
    , VDFMap{nullptr}
    , VDFMapTimestamp{std::chrono::steady_clock::now()}
    , VDFMapChecksum{0}
    , SurfaceID{-1}
    , GotBackground{false}
    , Position{std::numeric_limits<double>::max(), std::numeric_limits<double>::max()}
{
}

ClientContext::~ClientContext()
{
    TextureChecksumReadback.reset();
    if (m_DownsampleFramebuffers)
        glDeleteFramebuffers(m_DownsampleFramebuffers->size(), m_DownsampleFramebuffers->data());
}

client_t ClientContext::GetClient()
{
    return m_Client;
}

PHLMONITOR ClientContext::GetMonitor()
{
    if (m_ClientType == ClientType::LAYER)
        return rc<Desktop::View::CLayerSurface*>(m_Client)->m_monitor.lock();
    if (m_ClientType == ClientType::WINDOW)
        return rc<Desktop::View::CWindow*>(m_Client)->m_monitor.lock();
    return nullptr;
}

SP<std::array<Render::GL::CGLFramebuffer, 3>> ClientContext::GetJFAFramebuffers(const int width, const int height)
{
    if (m_Size.x != width || m_Size.y != height || !m_JFAFramebuffers)
    {
        m_Size = {width,  height};
        CreateJFAFramebuffers();
    }

    return m_JFAFramebuffers;
}

SP<std::array<GLuint, 2>> ClientContext::GetDownsampleFramebuffers()
{
    return m_DownsampleFramebuffers;
}

SP<Render::ITexture> ClientContext::GetDownsampleTexture()
{
    return m_DownsampleTexture;
}

const Vector2D& ClientContext::GetSize() const
{
    return m_Size;
}

const ClientType ClientContext::GetType() const
{
    return m_ClientType;
}

void ClientContext::CreateJFAFramebuffers()
{
    m_JFAFramebuffers = makeShared<std::array<Render::GL::CGLFramebuffer, 3>>();
    for (int i = 0; i < m_JFAFramebuffers->size(); i++)
    {
        auto& fb = m_JFAFramebuffers->at(i);
        fb.alloc(m_Size.x, m_Size.y);
        auto tex = fb.getTexture();
        tex->bind();
        if (i < 2)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, m_Size.x, m_Size.y, 0, GL_RG, GL_FLOAT, nullptr);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Size.x, m_Size.y, 0, GL_RGB, GL_FLOAT, nullptr);

        fb.bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->m_texID, 0);
        tex->m_size = m_Size;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ClientContext::ResizeJFATextures()
{
    for (int i = 0; i < m_JFAFramebuffers->size(); i++)
    {
        auto& fb  = m_JFAFramebuffers->at(i);
        const auto& tex = fb.getTexture();
        tex->bind();
        if (i < 2)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, m_Size.x, m_Size.y, 0, GL_RG, GL_FLOAT, nullptr);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Size.x, m_Size.y, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void ClientContext::CreateDownsampleFramebuffersAndTexture(const Vector2D& size)
{
    if (!m_DownsampleFramebuffers)
    {
        m_DownsampleFramebuffers = makeShared<std::array<GLuint, 2>>();
        glGenFramebuffers(m_DownsampleFramebuffers->size(), m_DownsampleFramebuffers->data());
    }

    auto& framebuffers = *m_DownsampleFramebuffers;

    m_DownsampleTexture = makeShared<Render::GL::CGLTexture>();
    m_DownsampleTexture->allocate(size);
    m_DownsampleTexture->bind();
    m_DownsampleTexture->setTexParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_DownsampleTexture->setTexParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_DownsampleTexture->setTexParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_DownsampleTexture->setTexParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (m_Size != size)
    {
        m_Size = size;
        if (m_JFAFramebuffers)
            ResizeJFATextures();
    }
}
