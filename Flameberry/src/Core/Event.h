#pragma once

#include <vector>
#include <string>
#include <functional>

namespace Flameberry {
    enum class EventType : uint8_t
    {
        None = 0,
        WindowResized,
        KeyPressed,
        MouseButtonPressed,
        MouseScrolled
    };
    
    enum class EventCategory : uint8_t {
        None = 0,
        WindowEventCategory,
        KeyEventCategory,
        MouseEventCategory
    };

    enum EventCategory : uint8_t
    {
        None = 0,
        WindowEventCategory,
        KeyEventCategory,
        MouseEventCategory
    };

#define EVENT_CATEGORY(CATEGORY) inline EventCategory GetCategory() const override { return CATEGORY; }\
inline bool IsInCategory(const EventCategory& category) const override { return CATEGORY == category; }

    struct Event
    {
        bool Handled;

        virtual ~Event() = default;
        virtual EventType GetType() const = 0;
        virtual EventCategory GetCategory() const = 0;
        virtual std::string ToString() const = 0;

        virtual inline EventCategory GetCategory() const = 0;
        virtual inline bool IsInCategory(const EventCategory& category) const = 0;
    };

    struct WindowResizedEvent : public Event
    {
        uint32_t Width, Height;

        WindowResizedEvent(uint32_t width, uint32_t height) : Width(width), Height(height) {}
        EventType GetType() const override { return EventType::WindowResized; }
        EventCategory GetCategory() const override { return EventCategory::WindowEventCategory; }
        std::string ToString() const override { return "WindowResizedEvent: " + std::to_string(Width) + ", " + std::to_string(Height); }

        EVENT_CATEGORY(WindowEventCategory);
    };

    struct KeyPressedEvent : public Event
    {
        uint32_t KeyCode;

        KeyPressedEvent(uint32_t keyCode) : KeyCode(keyCode) {}
        EventType GetType() const override { return EventType::KeyPressed; }
        EventCategory GetCategory() const override { return EventCategory::KeyEventCategory; }
        std::string ToString() const override { return "KeyPressedEvent: " + std::to_string(KeyCode); }

        EVENT_CATEGORY(KeyEventCategory);
    };

    struct MouseButtonPressedEvent : public Event
    {
        uint32_t KeyCode;

        MouseButtonPressedEvent(uint32_t keyCode) : KeyCode(keyCode) {}
        EventType GetType() const override { return EventType::MouseButtonPressed; }
        EventCategory GetCategory() const override { return EventCategory::MouseEventCategory; }
        std::string ToString() const override { return "MouseButtonPressedEvent: " + std::to_string(KeyCode); }

        EVENT_CATEGORY(MouseEventCategory);
    };

    struct MouseScrollEvent : public Event
    {
        double OffsetX, OffsetY;

        MouseScrollEvent(double X, double Y) : OffsetX(X), OffsetY(Y) {}
        EventType GetType() const override { return EventType::MouseScrolled; }
        EventCategory GetCategory() const override { return EventCategory::MouseEventCategory; }
        std::string ToString() const override { return "MouseScrollEvent: X: " + std::to_string(OffsetX) + "Y: " + std::to_string(OffsetY); }

        EVENT_CATEGORY(MouseEventCategory);
    };
}
