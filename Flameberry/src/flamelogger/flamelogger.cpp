#include "flamelogger.h"
#include <iomanip>
#include <ctime>

namespace flamelogger {
    std::string get_current_time_string()
    {
        std::stringstream prefix("");
        std::time_t now = std::time(0);
        std::tm* currentTime = std::localtime(&now);
        prefix << "[" << std::setfill('0') << std::setw(2) << std::to_string(currentTime->tm_hour) << ":" << std::setfill('0') << std::setw(2) << std::to_string(currentTime->tm_min) << ":" << std::setfill('0') << std::setw(2) << std::to_string(currentTime->tm_sec) << "] ";

        return prefix.str();
    }

    FLInstance::FLInstance(const char* instanceName)
        : m_CurrentLogLevel(LogLevel::TRACE)
    {
        if (strcmp(instanceName, ""))
            m_InstanceName = instanceName;
    }

    std::shared_ptr<FLInstance> FLInstance::Create(const char* instanceName)
    {
        return std::make_shared<FLInstance>(instanceName);
    }

    void FLInstance::SetLogLevel(const LogLevel& logLevel)
    {
        m_CurrentLogLevel = logLevel;
    }

    std::string FLInstance::get_prefix(const LogLevel& level)
    {
        std::stringstream prefix("");
        std::time_t now = std::time(0);
        std::tm* currentTime = std::localtime(&now);
        prefix << "[" << std::setfill('0') << std::setw(2) << std::to_string(currentTime->tm_hour) << ":" << std::setfill('0') << std::setw(2) << std::to_string(currentTime->tm_min) << ":" << std::setfill('0') << std::setw(2) << std::to_string(currentTime->tm_sec) << "] ";

        prefix << "[" << m_InstanceName << "] ";

        switch (level)
        {
        case LogLevel::LOG:
            prefix << "LOG: ";
            break;
        case LogLevel::TRACE:
            prefix << "TRACE: ";
            break;
        case LogLevel::INFO:
            prefix << "INFO: ";
            break;
        case LogLevel::WARNING:
            prefix << "WARNING: ";
            break;
        case LogLevel::ERROR:
            prefix << "ERROR: ";
            break;
        }

        return prefix.str();
    }
}