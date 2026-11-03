#pragma once
#include <hyprland/src/helpers/Color.hpp>
#include <GLES3/gl32.h>
#include <cstddef>
#include <functional>
#include <utility>

template <typename Tag>
struct RobberImpl
{
    static inline typename Tag::type ptr;
};

template <typename Target, typename T>
struct TargetMemberTag
{
    using type = T Target::*;
};

template <typename T>
struct GetMemberType;

template <typename M, typename C>
struct GetMemberType<M C::*>
{
    using typeM = M;
    using typeC = C;
};

template <auto PTR>
struct Infiltrator
{
    using T = GetMemberType<decltype(PTR)>;
    using M = T::typeM;
    using C = T::typeC;
    static inline bool initialized = (RobberImpl<TargetMemberTag<C, M>>::ptr = PTR, true);
};

template <typename Tag, auto PTR>
struct InfiltratorAlt
{
    static inline bool initialized = (RobberImpl<Tag>::ptr = PTR, true);
};

template <typename C, typename M>
struct Robber
{
    static M& Get(C* obj)
    {
        auto ptr = RobberImpl<TargetMemberTag<C, M>>::ptr;
        return obj->*ptr;
    }

    static auto GetFunc()
    {
        return RobberImpl<TargetMemberTag<C, M>>::ptr;
    }

    template <typename Obj, typename... Args>
    static auto Call(Obj* obj, Args&&... args)
    {
        auto ptr = GetFunc();
        return (obj->*ptr)(std::forward<Args>(args)...);
    }
};

template <typename C, typename M, size_t OFFSET = 0>
struct RefRobber
{
    static M& Get(C* obj)
    {
        return **reinterpret_cast<M* const*>(reinterpret_cast<std::byte*>(obj) + OFFSET);
    }
};

template <typename Tag>
struct RobberAlt
{
    using T = GetMemberType<typename Tag::type>;
    using M = T::typeM;
    using C = T::typeC;

    static auto& Get(C* obj)
    {
        auto ptr = RobberImpl<Tag>::ptr;
        return obj->*ptr;
    }

    static auto GetFunc()
    {
        return RobberImpl<Tag>::ptr;
    }

    template <typename Obj, typename... Args>
    static auto Call(Obj* obj, Args&&... args)
    {
        auto ptr = GetFunc();
        return (obj->*ptr)(std::forward<Args>(args)...);
    }
};


class FrameDelayGate
{
public:
    FrameDelayGate(int max_frames);
    bool operator()(bool should_delay);
    bool operator()(std::function<bool()> func);

private:
    int m_MaxFrames;
    int m_Counter = 0;
};

struct AsyncSSBOReadbackSlot
{
    GLuint SSBO = 0;
    GLsync Fence = nullptr;
};

class AsyncSSBOReadback
{
public:
    AsyncSSBOReadback();
    ~AsyncSSBOReadback();
    GLuint GetLatestResult();
    void PrepareWrite();
    void CommitWrite();

private:
    AsyncSSBOReadbackSlot& GetReadSlot();
    AsyncSSBOReadbackSlot& GetWriteSlot();
    bool IsReadReady();

    GLuint m_LastResult;
    int m_WriteSlotIndex;
    std::array<AsyncSSBOReadbackSlot, 2> m_Slots;

    static inline const GLuint ZERO = 0;
};

CHyprColor MakeColor(const uint8_t rgb, const float alpha);
CHyprColor MakeColor(const uint8_t r, const uint8_t g, const uint8_t b, const float alpha);

void DoAfterNLoops(const std::function<void()> fn, int loops);
