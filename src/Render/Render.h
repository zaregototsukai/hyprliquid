#pragma once
#include "Context/ClientContext.h"
#include <hyprland/src/render/Framebuffer.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprland/src/render/OpenGL.hpp>

using IFramebuffer = Render::IFramebuffer;
using ITexture = Render::ITexture;

void RenderShader(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& texture, const SP<CShader>& shader, const Hyprutils::Math::CRegion* damage = nullptr, const Hyprutils::Math::Vector2D& viewport = {});

void         DebugVDFMap(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& vdf_map, const CBox& box);
SP<ITexture> GetOrUpdateVDFMap(Render::CRenderContext& ctx, const SP<ITexture>& texture, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data, const SP<ClientContext>& client_context);

SP<ITexture> DownsampleTexture(const SP<ITexture>& texture, const Hyprutils::Math::Vector2D& size, const SP<ClientContext>& client_context);
void         RenderPassthrough(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& texture, const Hyprutils::Math::Vector2D& viewport);
SP<ITexture> ResizeTexture(Render::CRenderContext& ctx, const SP<ITexture>& texture, const Hyprutils::Math::Vector2D& size);
SP<ITexture> CopyTexture(Render::CRenderContext& ctx, const SP<ITexture>& texture);

GLuint GetTextureChecksum(const SP<ITexture>& texture, SP<AsyncSSBOReadback> readback);
bool   IsTextureBlack(const SP<ITexture>& texture, SP<AsyncSSBOReadback> readback);

SP<IFramebuffer> BlurTextureWithDamage(Render::CRenderContext& ctx, const float a, CRegion* originalDamage, const Config::INTEGER blur_size, const Config::INTEGER blur_passes, const SP<IFramebuffer>& source = nullptr);
SP<ITexture> EnsureBlur(Render::CRenderContext& ctx, const float a, CRegion* originalDamage, SP<ITexture> texture, const Config::INTEGER blur_size, const Config::INTEGER blur_passes);

void RenderLiquidGlass(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& texture, const CBox& box, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data, const SP<ITexture>& vdf_map = nullptr, const SP<ClientContext>& client_context = nullptr);
void RenderAcrylic(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& texture, const SP<ITexture>& mask, const CBox& box, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data, const bool thin = false, const SP<ClientContext>& client_context = nullptr);
void RenderMica(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& mask, const CBox& box, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data, const bool alt = false, const SP<ClientContext>& client_context = nullptr);
void RenderAero(Render::CRenderContext& ctx, IFramebuffer& fb, const SP<ITexture>& texture, const SP<ITexture>& mask, const CBox &box, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data, const SP<ClientContext>& client_context = nullptr);
