#pragma once

#include <vector>
#include <string>
#include <functional>

namespace Flameberry {
    enum class EventType {
        NONE = 0,
        KEY_PRESSED,
        MOUSEBUTTON_PRESSED,
        MOUSE_SCROLL
    };

    struct Event {
        bool Handled;

        virtual ~Event() = default;
        virtual EventType GetType() const = 0;
        virtual std::string ToString() const = 0;
    };

    struct KeyPressedEvent : public Event {
        uint32_t KeyCode;

        KeyPressedEvent(uint32_t keyCode) : KeyCode(keyCode) {}
        EventType GetType() const override { return EventType::KEY_PRESSED; }
        std::string ToString() const override { return "KeyPressedEvent: " + std::to_string(KeyCode); }
    };

    struct MouseButtonPressedEvent : public Event {
        uint32_t KeyCode;

        MouseButtonPressedEvent(uint32_t keyCode) : KeyCode(keyCode) {}
        EventType GetType() const override { return EventType::MOUSEBUTTON_PRESSED; }
        std::string ToString() const override { return "MouseButtonPressedEvent: " + std::to_string(KeyCode); }
    };

    struct MouseScrollEvent : public Event {
        double OffsetX, OffsetY;

        MouseScrollEvent(double X, double Y) : OffsetX(X), OffsetY(Y) {}
        EventType GetType() const override { return EventType::MOUSE_SCROLL; }
        std::string ToString() const override { return "MouseScrollEvent: X: " + std::to_string(OffsetX) + "Y: " + std::to_string(OffsetY); }
    };
}
