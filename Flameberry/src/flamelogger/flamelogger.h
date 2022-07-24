#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

/// Xcode doesn't support integrated terminal output by default, so the escape characters for coloring output are printed out
/// as it is, which just creates a mess, so the macro `FL_XCODE_PROJ` is defined by CMake, if the cmake generator is `Xcode`
/// and the macros are defined as `""`
#ifdef FL_XCODE_PROJ

#define FL_COLOR_DEFAULT ""
#define FL_COLOR_RED ""
#define FL_COLOR_GREEN ""
#define FL_COLOR_YELLOW ""
#define FL_COLOR_PURPLE ""
#define FL_COLOR_CYAN ""
#define FL_COLOR_WHITE ""

#else

#define FL_COLOR_DEFAULT "\e[0m"
#define FL_COLOR_RED "\e[0;31m"
#define FL_COLOR_GREEN "\e[0;32m"
#define FL_COLOR_YELLOW "\e[0;33m"
#define FL_COLOR_CYAN "\e[0;36m"
#define FL_COLOR_PURPLE "\e[0;35m"
#define FL_COLOR_WHITE "\e[0;37m"

// #define FL_COLOR_DEFAULT "\033[0m"
// #define FL_COLOR_RED "\033[31m"
// #define FL_COLOR_GREEN "\033[32m"
// #define FL_COLOR_YELLOW "\033[33m"
// #define FL_COLOR_PURPLE "\033[35m"
// #define FL_COLOR_CYAN "\033[36m"
// #define FL_COLOR_WHITE "\033[37m"
#endif

namespace flamelogger {
    /// This enum class is currently only used for adding log level prefix to all logger messages
    enum class LogLevel { TRACE = 0, LOG, INFO, WARNING, ERROR };

    std::string get_current_time_string();

    /// Just a default function for the templated function below
    static std::vector<std::string> convert_params_to_string()
    {
        return {};
    }

    /// This function converts all arguments to strings, regardless of them being float, int, double, unsigned int, etc.
    template<typename T, typename... Args>
    static std::vector<std::string> convert_params_to_string(const T& message, const Args&... args)
    {
        std::vector<std::string> string_set;

        std::stringstream ss;
        ss << message;

        string_set.push_back(ss.str());

        auto arguments = convert_params_to_string(args...);
        for (auto i : arguments)
            string_set.push_back(i);

        return string_set;
    }

    /// This function formats the log message, so that all portions of string that contain '{`number`}'
    /// get replaced by the arguments provided next to the main message string
    template<typename T, typename... Args>
    static std::string format_string(const T& message, const Args&... args)
    {
        std::vector<std::string> param_list = convert_params_to_string(message, args...);

        std::string temp_string = "";
        bool is_in_brackets = false;

        std::vector<std::string> indexes_in_string_format;

        bool is_index_valid = false;

        for (auto character : param_list[0])
        {
            if (character == '}' && is_in_brackets)
            {
                is_in_brackets = false;
                if (temp_string != "")
                {
                    indexes_in_string_format.push_back(temp_string);
                    temp_string = "";
                    is_index_valid = false;
                }
            }

            if (is_in_brackets)
            {
                if (((int)character >= 48) && ((int)character <= 57) && is_index_valid)
                {
                    temp_string.push_back(character);
                }
                else
                {
                    temp_string = "";
                    is_index_valid = false;
                }
            }

            if (character == '{')
            {
                is_in_brackets = true;
                is_index_valid = true;
            }
        }

        std::string msg = param_list[0];

        for (uint32_t i = 0; i < indexes_in_string_format.size(); i++)
        {
            msg.replace(
                msg.find("{" + indexes_in_string_format[i] + "}"),
                2 + indexes_in_string_format[i].length(),
                param_list[std::stoi(indexes_in_string_format[i]) + 1]
            );
        }
        return msg;
    }

    class FLInstance
    {
    public:
        /// Instance Name should be set at the beginning of the program,
        /// which will be used as a prefix to all the log messages during runtime
        FLInstance(const char* instanceName);
        static std::shared_ptr<FLInstance> Create(const char* instanceName);
        void SetLogLevel(const LogLevel& logLevel);

        /// Logs the message in CYAN color in the terminal
        template<typename T, typename... Args>
        void log(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::LOG)
            {
                std::string output_message = format_string(message, args...);
                std::cout << FL_COLOR_CYAN << get_prefix(LogLevel::LOG) << output_message << FL_COLOR_DEFAULT << std::endl;
            }
        }

        /// Logs the message in WHITE color in the terminal
        template<typename T, typename... Args>
        void trace(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::TRACE)
            {
                std::string output_message = format_string(message, args...);
                std::cout << FL_COLOR_WHITE << get_prefix(LogLevel::TRACE) << output_message << FL_COLOR_DEFAULT << std::endl;
            }
        }

        /// Logs the message in GREEN color in the terminal
        template<typename T, typename... Args>
        void info(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::INFO)
            {
                std::string output_message = format_string(message, args...);
                std::cout << FL_COLOR_GREEN << get_prefix(LogLevel::INFO) << output_message << FL_COLOR_DEFAULT << std::endl;
            }
        }

        /// Logs the message in YELLOW color in the terminal
        template<typename T, typename... Args>
        void warn(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::WARNING)
            {
                std::string output_message = format_string(message, args...);
                std::cout << FL_COLOR_YELLOW << get_prefix(LogLevel::WARNING) << output_message << FL_COLOR_DEFAULT << std::endl;
            }
        }

        /// Logs the message in RED color in the terminal
        template<typename T, typename... Args>
        void error(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::ERROR)
            {
                std::string output_message = format_string(message, args...);
                std::cout << FL_COLOR_RED << get_prefix(LogLevel::ERROR) << output_message << FL_COLOR_DEFAULT << std::endl;
            }
        }
    private:
        /// Gets the prefix of the log message
        std::string get_prefix(const LogLevel& logLevel);
    private:
        /// The name which is used in prefix of the log message
        const char* m_InstanceName = "";
        LogLevel m_CurrentLogLevel;
    };
}
