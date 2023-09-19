#pragma once

#include <vector>
#include <string>
#include <functional>

namespace Flameberry {
    enum class EventType : uint8_t {
        None = 0,
        WindowResized,
        KeyPressed,
        MouseButtonPressed,
        MouseScrolled
    };

    struct Event {
        bool Handled;

        virtual ~Event() = default;
        virtual EventType GetType() const = 0;
        virtual std::string ToString() const = 0;
    };

    struct WindowResizedEvent : public Event {
        uint32_t Width, Height;

        WindowResizedEvent(uint32_t width, uint32_t height) : Width(width), Height(height) {}
        EventType GetType() const override { return EventType::WindowResized; }
        std::string ToString() const override { return "WindowResizedEvent: " + std::to_string(Width) + ", " + std::to_string(Height); }
    };

    struct KeyPressedEvent : public Event {
        uint32_t KeyCode;

        KeyPressedEvent(uint32_t keyCode) : KeyCode(keyCode) {}
        EventType GetType() const override { return EventType::KeyPressed; }
        std::string ToString() const override { return "KeyPressedEvent: " + std::to_string(KeyCode); }
    };

    struct MouseButtonPressedEvent : public Event {
        uint32_t KeyCode;

        MouseButtonPressedEvent(uint32_t keyCode) : KeyCode(keyCode) {}
        EventType GetType() const override { return EventType::MouseButtonPressed; }
        std::string ToString() const override { return "MouseButtonPressedEvent: " + std::to_string(KeyCode); }
    };

    struct MouseScrollEvent : public Event {
        double OffsetX, OffsetY;

        MouseScrollEvent(double X, double Y) : OffsetX(X), OffsetY(Y) {}
        EventType GetType() const override { return EventType::MouseScrolled; }
        std::string ToString() const override { return "MouseScrollEvent: X: " + std::to_string(OffsetX) + "Y: " + std::to_string(OffsetY); }
    };
}
