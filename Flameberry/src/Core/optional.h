#pragma once

namespace flame
{
    /// @brief This `flame::optional` class will be soon removed as it's deprecated
    template<typename T>
    class optional
    {
    public:
        optional<T>() = default;
        optional<T>(T value)
            : m_Value(value), m_HasValue(true)
        {
        }
        operator T() const { return m_Value; }
        T& operator=(const T& value)
        {
            m_HasValue = true;
            m_Value = value;
            return m_Value;
        }
        bool has_value() const { return m_HasValue; }
    private:
        T m_Value;
        bool m_HasValue = false;
    };
}