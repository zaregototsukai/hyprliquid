#pragma once
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/render/pass/SurfacePassElement.hpp>
#include <hyprland/src/render/pass/TexPassElement.hpp>
#include <hyprland/src/render/gl/GLElementRenderer.hpp>
#include <hyprland/src/render/GLRenderer.hpp>
#include <hyprland/src/desktop/rule/layerRule/LayerRuleApplicator.hpp>
#include <hyprland/src/desktop/view/window/WindowPresentation.hpp>

inline CFunctionHook* g_IElementRendererDrawSurfaceHook;
using IElementRendererDrawSurface_t = void(*)(Render::IElementRenderer* this_ptr, Render::CRenderContext& ctx, WP<CSurfacePassElement> element, const CRegion& damage);
void HookIElementRendererDrawSurface(Render::IElementRenderer* this_ptr, Render::CRenderContext& ctx, WP<CSurfacePassElement> element, const CRegion& damage);

inline CFunctionHook* g_CHyprOpenGLImplRenderTextureInternalHook;
using CHyprOpenGLImplRenderTextureInternal_t = void(*)(Render::GL::CHyprOpenGLImpl* this_ptr, Render::CRenderContext& ctx, SP<Render::ITexture>, const CBox&, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data);
void HookCHyprOpenGLImplRenderTextureInternal(Render::GL::CHyprOpenGLImpl* this_ptr, Render::CRenderContext& ctx, SP<Render::ITexture>, const CBox&, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data);

inline CFunctionHook* g_CHyprOpenGLImplRenderTextureWithBlurInternalHook;
using CHyprOpenGLImplRenderTextureWithBlurInternal_t = void(*)(Render::GL::CHyprOpenGLImpl* this_ptr, Render::CRenderContext& ctx, SP<Render::ITexture>, const CBox&, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data);
void HookCHyprOpenGLImplRenderTextureWithBlurInternal(Render::GL::CHyprOpenGLImpl* this_ptr, Render::CRenderContext& ctx, SP<Render::ITexture>, const CBox&, const Render::GL::CHyprOpenGLImpl::STextureRenderData& data);

inline CFunctionHook* g_CSurfacePassElementOpaqueRegionHook;
using CSurfacePassElementOpaqueRegion_t = CRegion(*)(CSurfacePassElement* this_ptr, Render::CRenderContext& ctx);
CRegion HookCSurfacePassElementOpaqueRegion(CSurfacePassElement* this_ptr, Render::CRenderContext& ctx);

inline CFunctionHook* g_CWindowPresentationRoundingHook;
using CWindowPresentationRounding_t = float(*)(Desktop::View::CWindowPresentation* this_ptr);
float HookCWindowPresentationRounding(Desktop::View::CWindowPresentation* this_ptr);