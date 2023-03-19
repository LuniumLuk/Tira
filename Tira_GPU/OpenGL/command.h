#pragma once

#if defined(_MSC_VER)
// type conversion warning
#pragma warning(disable : 4267)
#endif

#include <functional>
#include <vector>

namespace GL {

using CommandType = std::function<void(void)>;

struct CommandList {
    auto invoke() const noexcept -> void;
    auto clear() noexcept -> void;
    auto emplace(CommandType&& cmd) noexcept -> void;

    static auto EnableDepthTest(bool value) noexcept -> CommandType;
    static auto ClearScreenColor(float r, float g, float b, float a) noexcept -> CommandType;
    static auto ClearScreenColorAndDepthBuffer(float r, float g, float b, float a) noexcept -> CommandType;
    static auto SetVieportSize(size_t w, size_t h) noexcept -> CommandType;

private:
    std::vector<CommandType> commands;
};

template<class ...T>
struct EventList
{
    using EventType = std::function<void(T...)>;

    auto connect(EventType const& event) noexcept -> void;

    template<class ...U>
    auto emit(U&&... args) noexcept -> void;

private:
    std::vector<EventType> events;
};

template<class ...T>
auto EventList<T...>::connect(EventType const& event) noexcept -> void {
    events.push_back(event);
}

template<class ...T>
template<class ...U>
auto EventList<T...>::emit(U&& ...args) noexcept -> void {
    for (auto& event : events)
        event(std::forward<U>(args)...);
}

}
