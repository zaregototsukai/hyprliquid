#pragma once
#include <wayland-server-core.h>

#ifndef HYPRLIQUID_NO_SYSTEMD
#include <systemd/sd-bus.h>
#endif

class ColorSchemeHelper
{
public:
    static void Init();
    static void Destroy();
    static int  GetColorScheme();
    static bool IsInitialized();

private:
#ifndef HYPRLIQUID_NO_SYSTEMD
    static int OnSettingsChanged(sd_bus_message* message, void* userdata, sd_bus_error* reterr_error);
    static int OnInitialQuery(sd_bus_message* message, void* userdata, sd_bus_error* reterr_error);
    static int OnDBusFD(int fd, uint32_t mask, void* data);
    static int OnDBusTimeout(void* data);
    static void ProcessBus();
    static void UpdateEventSources();

    static inline sd_bus*          m_Bus              = nullptr;
    static inline wl_event_source* m_EventSource      = nullptr;
    static inline wl_event_source* m_TimerEventSource = nullptr;
#endif

    static inline int  m_ColorScheme = 2;
    static inline bool m_Initialized = false;
};