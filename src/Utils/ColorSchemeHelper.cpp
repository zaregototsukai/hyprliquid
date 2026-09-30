#include "ColorSchemeHelper.h"
#include "Config/ConfigManager.h"
#include <hyprland/src/Compositor.hpp>
#include <poll.h>
#include <chrono>
#include <limits>
#include <string_view>

#ifdef HYPRLIQUID_NO_SYSTEMD

void ColorSchemeHelper::Init()
{
    m_Initialized = true;
}

void ColorSchemeHelper::Destroy()
{
    m_Initialized = false;
}

#else

int ColorSchemeHelper::OnSettingsChanged(sd_bus_message* message, void* userdata, sd_bus_error* reterr_error)
{
    using namespace std::string_view_literals;

    const char* name_space;
    const char* key;
    int result;

    const sd_bus_error* error = sd_bus_message_get_error(message);
    if (error)
        return 0;

    result = sd_bus_message_read(message, "ss", &name_space, &key);
    if (result < 0)
        return 0;

    if (name_space != "org.freedesktop.appearance"sv || key != "color-scheme"sv)
        return 0;

    result = sd_bus_message_enter_container(message, SD_BUS_TYPE_VARIANT, "u");
    if (result >= 0)
    {
        uint32_t value = 0;
        result = sd_bus_message_read(message, "u", &value);
        sd_bus_message_exit_container(message);
        if (result >= 0)
            m_ColorScheme = value;
    }

    return 0;
}

int ColorSchemeHelper::OnInitialQuery(sd_bus_message* message, void* userdata, sd_bus_error* reterr_error)
{
    const sd_bus_error* error = sd_bus_message_get_error(message);
    if (error)
        return 0;

    int result = sd_bus_message_enter_container(message, SD_BUS_TYPE_VARIANT, "u");
    if (result >= 0)
    {
        uint32_t value = 0;
        result = sd_bus_message_read(message, "u", &value);
        sd_bus_message_exit_container(message);
        if (result >= 0)
            m_ColorScheme = value;
    }

    return 0;
}

void ColorSchemeHelper::UpdateEventSources()
{
    if (!m_Bus)
        return;

    if (m_EventSource)
    {
        const int events = sd_bus_get_events(m_Bus);
        uint32_t wlEvents = 0;

        if (events >= 0)
        {
            if (events & POLLIN)
                wlEvents |= WL_EVENT_READABLE;
            if (events & POLLOUT)
                wlEvents |= WL_EVENT_WRITABLE;
        }

        wl_event_source_fd_update(m_EventSource, wlEvents);
    }

    if (m_TimerEventSource)
    {
        constexpr uint64_t uint64_max = std::numeric_limits<uint64_t>::max();
        constexpr int      int_max    = std::numeric_limits<int>::max();

        uint64_t deadline = uint64_max;
        int result = sd_bus_get_timeout(m_Bus, &deadline);

        if (result < 0 || deadline == uint64_max)
        {
            wl_event_source_timer_update(m_TimerEventSource, -1);
            return;
        }

        const auto now_time_since_epoch = std::chrono::steady_clock::now().time_since_epoch();
        const int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(now_time_since_epoch).count();
        const int64_t delay_us = deadline <= now_us ? 0 : deadline - now_us;
        const uint64_t delay_ms = std::chrono::ceil<std::chrono::milliseconds>(std::chrono::microseconds{delay_us}).count();

        wl_event_source_timer_update(m_TimerEventSource, static_cast<int>(delay_ms > int_max ? int_max : delay_ms));
    }
}

void ColorSchemeHelper::ProcessBus()
{
    if (!m_Bus)
        return;

    int result;
    while ((result = sd_bus_process(m_Bus, nullptr)) > 0);

    if (result >= 0)
        UpdateEventSources();
}

int ColorSchemeHelper::OnDBusFD(int fd, uint32_t mask, void* data)
{
    ProcessBus();
    return 0;
}

int ColorSchemeHelper::OnDBusTimeout(void* data)
{
    ProcessBus();
    return 0;
}

void ColorSchemeHelper::Init()
{
    static auto WATCH_SYSTEM_COLOR_SCHEME = CConfigValue<Config::BOOL>(std::string(ConfigManager::ConfigNames[ConfigManager::ConfigType::WATCH_SYSTEM_COLOR_SCHEME]));

    if (!*WATCH_SYSTEM_COLOR_SCHEME)
        return;

    int result = sd_bus_open_user(&m_Bus);
    if (result < 0)
        return;

    result = sd_bus_match_signal
    (
        m_Bus, nullptr,
        "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Settings",
        "SettingChanged", OnSettingsChanged,
        nullptr
    );
    if (result < 0)
    {
        Destroy();
        return;
    }

    result = sd_bus_call_method_async
    (
        m_Bus, nullptr,
        "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Settings",
        "ReadOne", OnInitialQuery,
        nullptr,
        "ss", "org.freedesktop.appearance", "color-scheme"
    );
    if (result < 0)
    {
        Destroy();
        return;
    }

    int dbus_fd = sd_bus_get_fd(m_Bus);
    if (dbus_fd < 0)
    {
        Destroy();
        return;
    }

    auto event_loop = g_pCompositor->m_wlEventLoop;
    m_EventSource = wl_event_loop_add_fd(event_loop, dbus_fd, 0, OnDBusFD, m_Bus);
    m_TimerEventSource = wl_event_loop_add_timer(event_loop, OnDBusTimeout, m_Bus);
    if (!m_EventSource || !m_TimerEventSource)
    {
        Destroy();
        return;
    }

    m_Initialized = true;
    ProcessBus();
}

void ColorSchemeHelper::Destroy()
{
    if (m_EventSource)
    {
        wl_event_source_remove(m_EventSource);
        m_EventSource = nullptr;
    }

    if (m_TimerEventSource)
    {
        wl_event_source_remove(m_TimerEventSource);
        m_TimerEventSource = nullptr;
    }

    if (m_Bus)
    {
        sd_bus_flush_close_unref(m_Bus);
        m_Bus = nullptr;
    }

    m_Initialized = false;
}

#endif

int ColorSchemeHelper::GetColorScheme()
{
    return m_ColorScheme;
}

bool ColorSchemeHelper::IsInitialized()
{
    return m_Initialized;
}
