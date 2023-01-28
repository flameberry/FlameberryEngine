#include "FileHandler.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "Core.h"

namespace Flameberry {
    FileHandler::FileHandler(const char* filePath)
        : m_FilePath(filePath)
    {
    }

    FileHandler::~FileHandler()
    {
    }

    char* FileHandler::MapFileToMemory()
    {
        // For macOS
        m_FileDescriptor = open(m_FilePath, O_RDONLY, S_IRUSR | S_IWUSR);
        if (fstat(m_FileDescriptor, &m_FileStats) == -1)
            FL_ERROR("Couldn't get the file size!");

        char* m_FileInMemory = (char*)mmap(NULL, m_FileStats.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, m_FileDescriptor, 0);
        return m_FileInMemory;
    }

    void FileHandler::UnmapFileFromMemory()
    {
        if (m_FileInMemory)
        {
            // For macOS
            if (munmap((void*)m_FileInMemory, m_FileStats.st_size) == -1)
                FL_ERROR("Failed to unmap contents of file: {0}", m_FilePath);
            close(m_FileDescriptor);
            m_FileInMemory = nullptr;
        }
    }
}