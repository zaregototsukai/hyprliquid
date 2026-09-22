#version 300 es
precision highp float;

uniform sampler2D Texture;
uniform sampler2D VDFMap;
uniform bool      HasVDFMap;
uniform vec2      TopLeft;
uniform vec2      BottomRight;
uniform vec2      Fullsize;
uniform vec2      TextureScale;
uniform float     CornerRadius;
uniform float     ZRadius;
uniform float     GlassIOR;
uniform bool      GlassDispersion;
uniform vec3      GlassIOR_RGB;
uniform float     GlassThickness;
uniform float     Brightness;
uniform vec4      TintColor;
uniform int       HighlightStyle;
uniform float     RoundingPower;

in  vec2 v_texcoord;
out vec4 FragColor;

const float AIR_IOR            = 1.0;
const vec3  INCIDENT_VECTOR    = vec3(0.0, 0.0, -1.0);
const float M_PI               = 3.1415926535897932384626433832795;
const float SMOOTHING_CONSTANT = M_PI / 5.34665792551;

vec2 SafeNormalize(vec2 vector)
{
    return length(vector) > 0.0 ? normalize(vector) : vec2(0.0);
}

vec2 VDFRectangle(vec2 pos, vec2 size)
{
    vec2 d = abs(pos) - size / 2.0;
    return sign(pos) * max(d, 0.0);
}

vec3 GetRoundNormal(vec2 normal, float dist, float radius)
{
    float r = clamp(dist / max(1e-11, radius), 0.0, 1.0);
    float z = sqrt(1.0 - r * r);
    vec3 n = normalize(vec3(normal * r, z));
    return mix(vec3(0.0, 0.0, 1.0), n, step(1e-11, dist));
}

vec3 GetEllipseNormal(vec2 normal, float dist, float a, float b)
{
    float r = clamp(dist / max(1e-11, a), 0.0, 1.0);
    float z = sqrt(1.0 - r * r) * b;
    float t = a / b;
    float k = t * t * z / max(1e-11, dist);
    vec3  n = normalize(vec3(normal, k));
    return mix(vec3(0.0, 0.0, 1.0), n, step(1e-11, dist));
}

float IORToF0(float ior)
{
    float t = (ior - AIR_IOR) / (ior + AIR_IOR);
    return t * t;
}

float pow5(float x)
{
    return x * x * x * x * x;
}

float FresnelSchlick(float cos_theta, float f0)
{
    return f0 + (1.0 - f0) * pow5(1.0 - cos_theta);
}

void main()
{
    float dist;
    float alpha;
    vec2  normal_2d;
    vec3  normal;

    if (HasVDFMap)
    {
        vec2 uv = (v_texcoord - TopLeft / Fullsize) * TextureScale;
        vec3 vdf_map = texture(VDFMap, uv).xyz;

        vec2 vdf = vdf_map.xy;
        float len = length(vdf);
        dist = max(len - CornerRadius + ZRadius, 0.0);
        alpha = vdf_map.z * smoothstep(0.0, 2.0, CornerRadius - len);
        normal_2d = SafeNormalize(vdf);
        normal = GetRoundNormal(normal_2d, dist, ZRadius);
    }
    else
    {
        vec2 center = (BottomRight + TopLeft) / 2.0;
        vec2 rect_size = (BottomRight - TopLeft - CornerRadius * 2.0);
        vec2 pos = gl_FragCoord.xy - center;

        vec2 vdf = VDFRectangle(pos, rect_size);
        float len = length(vdf);
        dist = max(len - CornerRadius + ZRadius, 0.0);
        normal_2d = SafeNormalize(vdf);

        if (RoundingPower == 2.0)
        {
            alpha = smoothstep(0.0, 1.0, (CornerRadius - len + SMOOTHING_CONSTANT) / (SMOOTHING_CONSTANT * 2.0));
            normal = GetRoundNormal(normal_2d, dist, ZRadius);
        }
        else
        {
            vec2 v = abs(vdf);
            float theta = atan(v.y, v.x);
            float max_dist = pow(pow(CornerRadius, RoundingPower) / (pow(sin(theta), RoundingPower) + pow(cos(theta), RoundingPower)), 1.0 / RoundingPower);
            max_dist = min(vdf.x, vdf.y) == 0.0 ? CornerRadius : max_dist;

            alpha = smoothstep(0.0, 1.0, (max_dist - len + SMOOTHING_CONSTANT) / (SMOOTHING_CONSTANT * 2.0));
            normal = GetEllipseNormal(normal_2d, dist, max(1e-3, max_dist - CornerRadius + ZRadius), ZRadius);
        }
    }
    
    vec2 col = (normal.z * ZRadius + GlassThickness) / Fullsize;
    vec3 color;

    if (GlassDispersion) for (int i = 0; i < 3; i++)
    {
        vec3 refracted_n = refract(INCIDENT_VECTOR, normal, AIR_IOR / GlassIOR_RGB[i]);
        float cos_theta = max(1e-11, dot(refracted_n, INCIDENT_VECTOR));

        vec2 uv_offset = col / cos_theta * refracted_n.xy;
        color[i] = texture(Texture, v_texcoord + uv_offset)[i];
    }
    else
    {
        vec3 refracted_n = refract(INCIDENT_VECTOR, normal, AIR_IOR / GlassIOR);
        float cos_theta = max(1e-11, dot(refracted_n, INCIDENT_VECTOR));

        vec2 uv_offset = col / cos_theta * refracted_n.xy;
        color = texture(Texture, v_texcoord + uv_offset).rgb;
    }

    color = mix(color, TintColor.rgb, TintColor.a);
    color *= Brightness;

    if (HighlightStyle > 0)
    {
        float n_dot_v = max(dot(normal, -INCIDENT_VECTOR), 0.0);
        float fresnel = FresnelSchlick(n_dot_v, IORToF0(GlassIOR));
        switch (HighlightStyle)
        {
            case 1:
                color = mix(color, vec3(1.0), fresnel);
                break;
            case 2:
                color = mix(color, vec3(1.0), fresnel * abs(dot(normal_2d, vec2(0.7071))));
                break;
            case 3:
            {
                float a = smoothstep(0.707, 1.0, abs(dot(normal_2d, vec2(0.573576436351046, 0.8191520442889918))));
                color = mix(color, vec3(1.0), fresnel * a);
                break;
            }
            case 4:
            {
                float w = smoothstep(0.5, 1.0, abs(dot(normal_2d, vec2(0.0, 1.0))));
                color = mix(color, vec3(0.0), fresnel);
                color = mix(color, vec3(1.0), fresnel * w);
                break;
            }
            default:
                break;
        }
    }

    FragColor = vec4(color * alpha, alpha);
}
