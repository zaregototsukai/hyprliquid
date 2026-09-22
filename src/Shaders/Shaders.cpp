#include "Shaders.h"
#include "Utils/Utils.hpp"
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/shaders/Shaders.hpp>
#include <fstream>
#include <sstream>
#include <ranges>

template struct Infiltrator<&CShader::m_uniformLocations>;
template struct Infiltrator<&CShader::m_program>;
template struct Infiltrator<&CShader::createVao>;
template struct Infiltrator<&CShader::compileShader>;

const std::string Tex300VertSource = std::string([] consteval -> std::string_view
{
    for (int i = 0; i < SHADERS.size(); i++)
        if (SHADERS[i].first == "tex300.vert")
            return SHADERS[i].second;
    return "";
}());

namespace Shaders
{
    void InitLocations(std::array<GLint, eShaderUniform::SHADER_LAST>& uniform_locations, GLuint program)
    {
        uniform_locations[SHADER_PROJ]       = glGetUniformLocation(program, "proj");
        uniform_locations[SHADER_TEX_ATTRIB] = glGetAttribLocation (program, "texcoord");
        uniform_locations[SHADER_POS_ATTRIB] = glGetAttribLocation (program, "pos");
    }

    SP<CShader> CreateJumpFloodingInnerInitShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)JumpFloodingInnerInitSource, true, false))
            return nullptr;
        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        m_uniformLocations[SHADER_TEX]                  = glGetUniformLocation(program, "Texture");
        m_uniformLocations[CUSTOM_SHADER_CORNER_RADIUS] = glGetUniformLocation(program, "CornerRadius");

        Robber<CShader, void()>::Call(shader.get());
        g_JumpFloodingInnerInitShader = shader;
        return shader;
    }

    SP<CShader> CreateJumpFloodingOuterInitShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)JumpFloodingOuterInitSource, true, false))
            return nullptr;
        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        m_uniformLocations[SHADER_TEX] = glGetUniformLocation(program, "Texture");

        Robber<CShader, void()>::Call(shader.get());
        g_JumpFloodingOuterInitShader = shader;
        return shader;
    }

    SP<CShader> CreateJumpFloodingShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)JumpFloodingSource, true, false))
            return nullptr;
        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        m_uniformLocations[SHADER_TEX]                  = glGetUniformLocation(program, "Texture");
        m_uniformLocations[SHADER_FULL_SIZE]            = glGetUniformLocation(program, "TextureSize");
        m_uniformLocations[CUSTOM_SHADER_JFA_STRIDE]    = glGetUniformLocation(program, "Stride");
        m_uniformLocations[CUSTOM_SHADER_JFA_DIRECTION] = glGetUniformLocation(program, "Direction");

        Robber<CShader, void()>::Call(shader.get());
        g_JumpFloodingShader = shader;
        return shader;
    }

    SP<CShader> CreateJumpFloodingDistanceFilterShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)JumpFloodingDistanceFilterSource, true, false))
            return nullptr;
        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        m_uniformLocations[SHADER_TEX]                  = glGetUniformLocation(program, "Texture");
        m_uniformLocations[CUSTOM_SHADER_CORNER_RADIUS] = glGetUniformLocation(program, "CornerRadius");

        Robber<CShader, void()>::Call(shader.get());
        g_JumpFloodingDistanceFilterShader = shader;
        return shader;
    }

    SP<CShader> CreateJumpFloodingFinalShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)JumpFloodingFinalSource, true, false))
            return nullptr;
        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        m_uniformLocations[SHADER_TEX]                 = glGetUniformLocation(program, "Texture");
        m_uniformLocations[CUSTOM_SHADER_MASK_TEXTURE] = glGetUniformLocation(program, "Mask");

        Robber<CShader, void()>::Call(shader.get());
        g_JumpFloodingFinalShader = shader;
        return shader;
    }

    SP<CShader> CreateDebugVDFMapShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)DebugVDFMapSource, true, false))
            return nullptr;
        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        m_uniformLocations[SHADER_TEX]                  = glGetUniformLocation(program, "Texture");
        m_uniformLocations[SHADER_TOP_LEFT]             = glGetUniformLocation(program, "TopLeft");
        m_uniformLocations[SHADER_FULL_SIZE]            = glGetUniformLocation(program, "Fullsize");
        m_uniformLocations[CUSTOM_SHADER_TEXTURE_SCALE] = glGetUniformLocation(program, "TextureScale");

        Robber<CShader, void()>::Call(shader.get());
        g_DebugVDFMapShader = shader;
        return shader;
    }

    SP<CShader> CreateChecksumShader()
    {
        auto shader = makeShared<CShader>();
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        program = glCreateProgram();
        if (!program)
            return nullptr;

        auto _shader = Robber<CShader, GLuint(const GLuint&, std::string, bool, bool)>::Call(shader.get(), GL_COMPUTE_SHADER, (const char*)ChecksumSource, true, false);
        glAttachShader(program, _shader);
        glLinkProgram(program);
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (success != GL_TRUE)
            return nullptr;

        glDetachShader(program, _shader);
        glDeleteShader(_shader);

        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        m_uniformLocations[SHADER_TEX] = glGetUniformLocation(program, "Texture");

        g_ChecksumShader = shader;
        return shader;
    }

    SP<CShader> CreateBlackDetectionShader()
    {
        auto shader = makeShared<CShader>();
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        program = glCreateProgram();
        if (!program)
            return nullptr;

        auto _shader = Robber<CShader, GLuint(const GLuint&, std::string, bool, bool)>::Call(shader.get(), GL_COMPUTE_SHADER, (const char*)BlackDetectionSource, true, false);
        glAttachShader(program, _shader);
        glLinkProgram(program);
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (success != GL_TRUE)
            return nullptr;

        glDetachShader(program, _shader);
        glDeleteShader(_shader);

        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        m_uniformLocations[SHADER_TEX] = glGetUniformLocation(program, "Texture");

        g_BlackDetectionShader = shader;
        return shader;
    }

    SP<CShader> CreateLiquidGlassShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)LiquidGlassShaderSource, true, false))
            return nullptr;

        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        m_uniformLocations[SHADER_TEX]                     = glGetUniformLocation(program, "Texture");
        m_uniformLocations[SHADER_TOP_LEFT]                = glGetUniformLocation(program, "TopLeft");
        m_uniformLocations[SHADER_BOTTOM_RIGHT]            = glGetUniformLocation(program, "BottomRight");
        m_uniformLocations[SHADER_FULL_SIZE]               = glGetUniformLocation(program, "Fullsize");
        m_uniformLocations[SHADER_TINT]                    = glGetUniformLocation(program, "TintColor");
        m_uniformLocations[SHADER_BRIGHTNESS]              = glGetUniformLocation(program, "Brightness");
        m_uniformLocations[SHADER_ROUNDING_POWER]          = glGetUniformLocation(program, "RoundingPower");
        m_uniformLocations[CUSTOM_SHADER_TEXTURE_SCALE]    = glGetUniformLocation(program, "TextureScale");
        m_uniformLocations[CUSTOM_SHADER_HAS_VDF_MAP]      = glGetUniformLocation(program, "HasVDFMap");
        m_uniformLocations[CUSTOM_SHADER_VDF_MAP]          = glGetUniformLocation(program, "VDFMap");
        m_uniformLocations[CUSTOM_SHADER_HIGHLIGHT_STYLE]  = glGetUniformLocation(program, "HighlightStyle");
        m_uniformLocations[CUSTOM_SHADER_CORNER_RADIUS]    = glGetUniformLocation(program, "CornerRadius");
        m_uniformLocations[CUSTOM_SHADER_Z_RADIUS]         = glGetUniformLocation(program, "ZRadius");
        m_uniformLocations[CUSTOM_SHADER_GLASS_IOR]        = glGetUniformLocation(program, "GlassIOR");
        m_uniformLocations[CUSTOM_SHADER_GLASS_DISPERSION] = glGetUniformLocation(program, "GlassDispersion");
        m_uniformLocations[CUSTOM_SHADER_GLASS_IOR_RGB]    = glGetUniformLocation(program, "GlassIOR_RGB");
        m_uniformLocations[CUSTOM_SHADER_GLASS_THICKNESS]  = glGetUniformLocation(program, "GlassThickness");

        Robber<CShader, void()>::Call(shader.get());
        g_LiquidGlassShader = shader;
        return shader;
    }

    SP<CShader> CreateFluentMaterialShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)FluentMaterialShaderSource, true, false))
            return nullptr;

        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        m_uniformLocations[SHADER_TEX]                     = glGetUniformLocation(program, "Texture");
        m_uniformLocations[CUSTOM_SHADER_MASK_TEXTURE]     = glGetUniformLocation(program, "Mask");
        m_uniformLocations[SHADER_TOP_LEFT]                = glGetUniformLocation(program, "TopLeft");
        m_uniformLocations[SHADER_BOTTOM_RIGHT]            = glGetUniformLocation(program, "BottomRight");
        m_uniformLocations[SHADER_FULL_SIZE]               = glGetUniformLocation(program, "Fullsize");
        m_uniformLocations[CUSTOM_SHADER_TEXTURE_SCALE]    = glGetUniformLocation(program, "TextureScale");
        m_uniformLocations[CUSTOM_SHADER_LUMINOSITY_COLOR] = glGetUniformLocation(program, "LuminosityColor");
        m_uniformLocations[SHADER_TINT]                    = glGetUniformLocation(program, "TintColor");
        m_uniformLocations[SHADER_BRIGHTNESS]              = glGetUniformLocation(program, "Brightness");
        m_uniformLocations[SHADER_DISCARD_ALPHA_VALUE]     = glGetUniformLocation(program, "DiscardAlphaValue");

        Robber<CShader, void()>::Call(shader.get());
        g_FluentMaterialShader = shader;
        return shader;
    }

    SP<CShader> CreateAeroShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)AeroShaderSource, true, false))
            return nullptr;

        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        m_uniformLocations[SHADER_TEX]                     = glGetUniformLocation(program, "Texture");
        m_uniformLocations[CUSTOM_SHADER_REFLECTION_MAP]   = glGetUniformLocation(program, "ReflectionMap");
        m_uniformLocations[CUSTOM_SHADER_MASK_TEXTURE]     = glGetUniformLocation(program, "Mask");
        m_uniformLocations[SHADER_TOP_LEFT]                = glGetUniformLocation(program, "TopLeft");
        m_uniformLocations[SHADER_BOTTOM_RIGHT]            = glGetUniformLocation(program, "BottomRight");
        m_uniformLocations[SHADER_FULL_SIZE]               = glGetUniformLocation(program, "Fullsize");
        m_uniformLocations[CUSTOM_SHADER_TEXTURE_SCALE]    = glGetUniformLocation(program, "TextureScale");
        m_uniformLocations[CUSTOM_SHADER_LUMINOSITY_COLOR] = glGetUniformLocation(program, "LuminosityColor");
        m_uniformLocations[SHADER_TINT]                    = glGetUniformLocation(program, "TintColor");
        m_uniformLocations[SHADER_BRIGHTNESS]              = glGetUniformLocation(program, "Brightness");
        m_uniformLocations[SHADER_DISCARD_ALPHA_VALUE]     = glGetUniformLocation(program, "DiscardAlphaValue");

        Robber<CShader, void()>::Call(shader.get());
        g_AeroShader = shader;
        return shader;
    }

    SP<CShader> CreateAeroReflectionMapShader()
    {
        const auto& vert_source = Tex300VertSource;
        auto shader = makeShared<CShader>();
        if (!shader->createProgram(vert_source, (const char*)AeroReflectionMapShaderSource, true, false))
            return nullptr;

        auto& m_uniformLocations = Robber<CShader, std::array<GLint, eShaderUniform::SHADER_LAST>>::Get(shader.get());
        auto& program = Robber<CShader, GLuint>::Get(shader.get());
        InitLocations(m_uniformLocations, program);

        Robber<CShader, void()>::Call(shader.get());
        g_AeroReflectionMapShader = shader;
        return shader;
    }

    void Init()
    {
        Render::GL::g_pHyprOpenGL->makeEGLCurrent();
        constexpr std::array<SP<CShader>(*)(), 12> functions
        {
            CreateJumpFloodingInnerInitShader,
            CreateJumpFloodingShader,
            CreateJumpFloodingFinalShader,
            CreateJumpFloodingOuterInitShader,
            CreateJumpFloodingDistanceFilterShader,
            CreateDebugVDFMapShader,
            CreateChecksumShader,
            CreateBlackDetectionShader,
            CreateLiquidGlassShader,
            CreateFluentMaterialShader,
            CreateAeroShader,
            CreateAeroReflectionMapShader
        };

        if (std::ranges::any_of(functions, [](auto func) { return !func(); }))
        {
            Destroy();
            return;
        }

        Shaders::IsInitialized = true;
    }

    void Destroy()
    {
        g_JumpFloodingInnerInitShader.reset();
        g_JumpFloodingShader.reset();
        g_JumpFloodingFinalShader.reset();
        g_JumpFloodingOuterInitShader.reset();
        g_JumpFloodingDistanceFilterShader.reset();
        g_DebugVDFMapShader.reset();
        g_ChecksumShader.reset();
        g_BlackDetectionShader.reset();
        g_LiquidGlassShader.reset();
        g_FluentMaterialShader.reset();
        g_AeroShader.reset();
        g_AeroReflectionMapShader.reset();

        Shaders::IsInitialized = false;
    }
}