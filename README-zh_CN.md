# hyprliquid

<p align="center"><a href="README.md">English</a> | 简体中文</p>

hyprliquid 是一款 Hyprland 插件，可为窗口和 layer surface 添加 Liquid Glass、Acrylic、Mica 以及 Aero 风格的材质效果。

## 功能

- Liquid Glass 折射效果：支持可选 RGB 色散、可配置高光与圆角。
- Acrylic、Acrylic Thin、Mica、Mica Alt 和 Aero 材质效果。
- 支持按窗口、按 layer 设置独立规则，材质调节项可配置全局默认值。
- 可选监听 XDG Desktop Portal 配色方案，用于 Acrylic 和 Mica 的自动明暗切换。
- 可选的 `background-share` Wayland 协议，供需要获取背景像素的兼容客户端使用。

## 效果展示

<p align="center">
    <a href="assets/Preview-1.jpeg"><img src="assets/Preview-1.jpeg" alt="Liquid Glass effect" width="32%" /></a>
    <a href="assets/Preview-2.jpeg"><img src="assets/Preview-2.jpeg" alt="hyprliquid material effects" width="32%" /></a>
    <a href="assets/Preview-3.jpeg"><img src="assets/Preview-3.jpeg" alt="Liquid Glass with blur" width="32%" /></a>
</p>

## 安装

### hyprpm

请安装所用发行版提供的 `cmake`、`stb`、Wayland、systemd 以及 Hyprland 开发文件。`hyprwayland-scanner` 为可选依赖：若不可用，构建系统将使用仓库中预生成的协议源码。

```sh
hyprpm update
hyprpm add https://github.com/zaregototsukai/hyprliquid
hyprpm enable hyprliquid
```

### Arch Linux（AUR）

```sh
# 使用你偏好的 AUR 助手，例如：
yay -S hyprliquid
```

加载已安装的模块：

```sh
hyprctl plugin load /usr/lib/libhyprliquid.so
```

### 手动构建

请安装所用发行版提供的 `cmake`、`stb`、Wayland、systemd 以及 Hyprland 开发文件。`hyprwayland-scanner` 为可选依赖：若不可用，构建系统将使用仓库中预生成的协议源码。

克隆仓库，构建 Release 模块并安装：

```sh
git clone https://github.com/zaregototsukai/hyprliquid
cd hyprliquid
make release
sudo make install
```

使用以下命令加载：

```sh
hyprctl plugin load /usr/lib/libhyprliquid.so
```

### 启动时自动加载

使用以下任一方式，在 Hyprland 启动时自动加载插件。

#### Lua

```lua
-- hyprpm
hl.on("hyprland.start", function ()
    hl.exec_cmd("hyprpm reload");
end);

-- AUR或手动安装
hl.plugin.load("/usr/lib/libhyprliquid.so");
```

#### Hyprlang

```ini
# hyprpm
exec-once = hyprpm reload

# AUR或手动安装
plugin = /usr/lib/libhyprliquid.so
```

## 配置

配置分为全局默认值与规则级覆盖值。对于支持全局回退的选项，规则中设置的值会覆盖同名全局值；两者均未设置时，使用编译期默认值。

在旧版 Hyprlang 配置中，全局设置写在 `plugin { hyprliquid { ... } }` 块内；规则级设置放在 `windowrule` 或 `layerrule` 内的 `hyprliquid` 块中。Lua 配置则使用对应的 `plugin.hyprliquid` 表以及 `hyprliquid:<option>` 规则键。

### Lua 示例

```lua
if hl.plugin.hyprliquid then
    hl.config(
    {
        plugin =
        {
            hyprliquid =
            {
                watch_system_color_scheme = true,
                background_sharing = true
            }
        }
    });

    hl.window_rule(
    {
        name = "kitty",
        match = { class = "kitty" },
        border_size = 0,
        no_blur = true,
        no_shadow = true,
        ["hyprliquid:rounding_lua"] = 64,
        ["hyprliquid:effect"] = "liquid_glass",
        ["hyprliquid:tint_color"] = "rgba(0x1a, 0x1b, 0x26, 0.5)",
        ["hyprliquid:highlight_style"] = 2,
        ["hyprliquid:glass_dispersion"] = true
    });

    hl.window_rule(
    {
        name = "zen-browser",
        match = { class = "zen" },
        ["hyprliquid:effect"] = "acrylic_thin",
        ["hyprliquid:color_scheme"] = "follow_system"
    });

    hl.window_rule(
    {
        name = "vscode",
        match = { class = "code" },
        ["hyprliquid:effect"] = "acrylic",
        ["hyprliquid:color_scheme"] = "dark"
    });

    hl.window_rule(
    {
        name = "dolphin",
        match = { class = "org.kde.dolphin" },
        ["hyprliquid:effect"] = "mica_alt",
        ["hyprliquid:color_scheme"] = "light"
    });

    hl.layer_rule(
    {
        name = "waybar",
        match = { namespace = "waybar" },
        ["hyprliquid:effect"] = "liquid_glass",
        ["hyprliquid:corner_radius"] = 20,
        ["hyprliquid:highlight_style"] = 4,
        ["hyprliquid:vdf_map_mode"] = 1,
        ["hyprliquid:vdf_map_update_policy"] = "onchange"
    });

    hl.layer_rule(
    {
        name = "wlogout",
        match = { namespace = "logout_dialog" },
        blur = true,
        ["hyprliquid:effect"] = "liquid_glass",
        ["hyprliquid:corner_radius"] = 96,
        ["hyprliquid:highlight_style"] = 4,
        ["hyprliquid:vdf_map_mode"] = 2,
        ["hyprliquid:vdf_map_update_policy"] = "once"
    });
end
```

### 旧版 Hyprlang 示例

```ini
plugin {
    hyprliquid {
        watch_system_color_scheme = on
        background_sharing = on
    }
}

windowrule {
    name = kitty
    match:class = kitty
    border_size = 0
    no_blur = on
    no_shadow = on
    rounding = 64
    hyprliquid {
        effect = liquid_glass
        tint_color = rgba(0x1a, 0x1b, 0x26, 0.5)
        highlight_style = 2
        glass_dispersion = on
    }
}

windowrule {
    name = zen-browser
    match:class = zen
    hyprliquid {
        effect = acrylic_thin
        color_scheme = follow_system
    }
}

windowrule {
    name = vscode
    match:class = code
    hyprliquid {
        effect = acrylic
        color_scheme = dark
    }
}

windowrule {
    name = dolphin
    match:class = org.kde.dolphin
    hyprliquid {
        effect = mica_alt
        color_scheme = light
    }
}

layerrule {
    name = waybar
    match:namespace = waybar
    hyprliquid {
        effect = liquid_glass
        corner_radius = 20
        highlight_style = 4
        vdf_map_mode = 1
        vdf_map_update_policy = onchange
    }
}

layerrule {
    name = wlogout
    match:namespace = logout_dialog
    blur = on
    hyprliquid {
        effect = liquid_glass
        corner_radius = 96
        highlight_style = 4
        vdf_map_mode = 2
        vdf_map_update_policy = once
    }
}
```

### 全局变量

| 名称 | 类型 | 默认值 | 描述 |
| --- | --- | --- | --- |
| `enabled` | `bool` | `true` | hyprliquid 所有渲染效果的总开关。 |
| `background_sharing` | `bool` | `false` | 启用可选的 `background-share` Wayland 协议。需要 `EGL_MESA_image_dma_buf_export` 支持，且客户端需实现该协议。 |
| `watch_system_color_scheme` | `bool` | `false` | 监听 XDG Desktop Portal 配色方案变更。使用 `color_scheme = follow_system` 前必须开启此项。 |
| `aero_reflection_map_path` | `string` | 空 | Aero 反射贴图的自定义图片路径。路径为空或图片不可读时，回退到内置贴图。（如果你想找原版 Win7 反射贴图，可以去 `C:\Windows\Resources\Themes\Aero\aero.msstyles` 文件中找。） |

> [!NOTE]
> 请在插件加载前设置 `watch_system_color_scheme` 和 `background_sharing`。修改任一选项后需重启 Hyprland。

### 规则变量

作用域标注为**全局回退**的选项也可设为全局值。`color_scheme` 和 `vdf_map_update_policy` 在规则中可使用字符串名称，在全局配置中请使用对应的数值。**仅规则**选项必须写在匹配的 `windowrule` 或 `layerrule` 内。

| 名称 | 作用域 | 类型 | 默认值 | 可接受的值 | 描述 |
| --- | --- | --- | --- | --- | --- |
| `effect` | 仅规则 | `string` 或 `int` | `none` | `none`、`liquid_glass`、`acrylic`、`acrylic_thin`、`mica`、`mica_alt`、`aero`，或 `0`～`6` | 选择材质效果。 |
| `corner_radius` | 全局回退 | `int` | `-1` | `-1` 或非负逻辑像素值 | 设为 `-1` 时，沿用该窗口 / Layer 表面在 Hyprland 中的圆角值。 |
| `z_radius` | 全局回退 | `int` | `-1` | `-1` 或非负逻辑像素值 | Liquid Glass 深度半径。设为 `-1` 时取 `corner_radius` 的值；超过圆角半径的值会被钳制。 |
| `glass_thickness` | 全局回退 | `float` | `500.0` | 建议非负值 | 控制 Liquid Glass 的折射偏移量。 |
| `glass_ior` | 全局回退 | `float` | `1.035` | 建议正值 | RGB 色散关闭时使用的折射率。 |
| `glass_ior_r` | 全局回退 | `float` | `1.02` | 建议正值 | RGB 色散开启时，红色通道的折射率。 |
| `glass_ior_g` | 全局回退 | `float` | `1.035` | 建议正值 | RGB 色散开启时，绿色通道的折射率。 |
| `glass_ior_b` | 全局回退 | `float` | `1.05` | 建议正值 | RGB 色散开启时，蓝色通道的折射率。 |
| `glass_dispersion` | 全局回退 | `bool` | `false` | 布尔值 | 开启 RGB 色散，启用后使用 `glass_ior_r`、`glass_ior_g`、`glass_ior_b` 分别控制三通道折射率。 |
| `vdf_map_mode` | 仅规则 | `int` | `0` | `0`、`1`、`2` | 选择 Liquid Glass 的矢量距离场（VDF）模式。`0` 禁用 VDF 贴图生成。 |
| `vdf_map_update_policy` | 全局回退 | 全局：`int`；规则：`string` 或 `int` | `-2`（`always`） | 规则：`always`、`onchange`、`once`，或类似 `250ms`、`2s`、`1min`、`1hour` 的时间间隔；全局：数值代码。详见 [VDF 更新策略](#vdf-更新策略)。 | 控制 VDF 贴图的重建时机。`onchange` 是一种异步近似策略，旨在降低性能开销。 |
| `vdf_map_debug_mode` | 仅规则 | `int` | `0` | `0`、`1`、`2` | 开启 VDF 诊断渲染模式。 |
| `tint_color` | 全局回退 | color | transparent（透明） | Hyprland 颜色语法，例如 `rgba(0x1a, 0x1b, 0x26, 0.5)` | 为材质叠加色调。设为透明值时，使用材质内置的默认色调。 |
| `brightness` | 全局回退 | `float` | `1.0` | 建议正值 | 最终材质亮度的倍乘系数。 |
| `highlight_style` | 全局回退 | `int` | `0` | `0`～`4` | 选择 Liquid Glass 高光样式：<br> `0`：关闭。<br> `1`：全部边缘。<br> `2`：左上与右下边缘。<br> `3`：OS 26 风格高光。<br> `4`：OS 27 风格高光。 |
| `color_scheme` | 全局回退 | 全局：`int`；规则：`string` | `0`（light） | `dark`、`light`、`follow_system`，或 `0`～`3` | 选择 Acrylic 和 Mica 的配色方案。`follow_system` 需要开启 `watch_system_color_scheme = true`。 |
| `rounding_lua` | 仅 Lua 配置与窗口规则 | `int` | `-1` | `-1` 或非负逻辑像素值 | 通过 Lua 配置时覆盖窗口圆角。Hyprland 的 Lua API 将常规 `rounding` 规则上限限制为 `20`，如需更大圆角请使用 `rounding_lua`。此选项在旧版 Hyprlang 配置及 layer surface 上无效——这些场景请使用常规 `rounding` 设置。 |
| `force_full_damage` | 全局回退 | `bool` | `false` | 布尔值 | 让材质渲染覆盖整个表面，而不是只渲染受损区域。如果遇到渲染问题，可以开启此项。 |

### 什么是 VDF 贴图？

Liquid Glass 需要边缘法线来计算折射。对于 `kitty` 或 `neovide` 这类只有单个圆角矩形的简单窗口，法线可以直接计算，无需 VDF 贴图。这种情况下请保持 `vdf_map_mode` 为 `0`。

矢量距离场（VDF）贴图为每个像素存储一个指向对应圆角矩形中心的向量。与有符号距离场（SDF）不同，VDF 存储的是向量而非标量距离。当直接计算法线不够用时，hyprliquid 通过 Jump Flooding 算法生成 VDF 贴图。

- `vdf_map_mode = 1`：适用于 `waybar` 这类包含多个独立圆角矩形的布局。
- `vdf_map_mode = 2`：适用于 `wlogout` 这类更密集的布局，其圆角矩形在水平与垂直方向紧密相邻、间距小于圆角半径。此模式会运行两遍 Jump Flooding，GPU 开销最高。

> [!NOTE]
> 同一表面内的所有圆角矩形必须使用相同的圆角半径。

请保持 `vdf_map_mode` 为 `0` 以获得最低开销，仅在 Liquid Glass 边缘处理确实需要时才开启 VDF 模式。

### VDF 更新策略

- `always`：每帧重建。适合内容持续变化的场景，GPU 开销最高。
- `onchange`：当插件的异步纹理摘要发生变化时重建。开销更低，适合 `waybar` 这类偶尔变化的内容；但细微或短暂的变化可能被延迟检测甚至漏检。
- `once`：待表面稳定后构建一次。适合 `wlogout` 这类静态内容。
- 时间间隔：按指定间隔重建。不带单位的正数视为毫秒。

### 调试 VDF 贴图

当无法确定表面的圆角半径时，可将 `vdf_map_debug_mode` 设为 `1` 或 `2` 来开启 VDF 调试。模式 `1` 保留其原始纹理可见；模式 `2` 隐藏其纹理。alpha 非零的区域渲染为白色，其余区域渲染为黑色。

反复调整 `corner_radius`，直到白色区域内的黑色面积尽可能小但不完全消失。此时 VDF 贴图应已正确生成；移除 `vdf_map_debug_mode` 或将其恢复为 `0` 即可。

## 背景共享协议

开启 `background_sharing = true` 后，hyprliquid 会启用不稳定的 `zhypr_background_share_unstable_v1` 协议。该协议面向需要获取壁纸像素来实现自定义效果的应用（例如 Liquid Glass 滑块）。常规材质渲染无需此协议。

为保护隐私，协议仅共享 `BACKGROUND` 图层的像素，绝不共享窗口或其他非背景图层的内容。协议在兼容客户端绑定之前不会产生任何实际效果。
