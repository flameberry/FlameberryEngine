#pragma once

#include <sys/stat.h>

namespace Flameberry {
    class FileHandler
    {
    public:
        FileHandler(const char* filePath);
        ~FileHandler();

        char* MapFileToMemory();
        void UnmapFileFromMemory();
    private:
        const char* m_FilePath;

        // Mapping Resources
        char* m_FileInMemory = nullptr;
        int m_FileDescriptor;
        struct stat m_FileStats;
    };
}