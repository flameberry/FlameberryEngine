#include "ModelLoader.h"

#include <fstream>
#include <sstream>

#include "Core/Timer.h"
#include "Core/Core.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

namespace Flameberry {
    enum TagTypeOBJ { NONE = 0, VERTEX_POSITION, VERTEX_TEXTURE_UV, VERTEX_NORMAL, FACE };

    float ModelLoader::ParseFloat(const char* str, char delimiter)
    {
        bool int_part_ended = false;
        float number = 0.0f;
        int i = 0;

        for (char* currentChar = (char*)str; *currentChar != delimiter && *currentChar != '\0'; currentChar++)
        {
            if (*currentChar == '.')
                int_part_ended = true;
            else if (char digit = *currentChar - '0'; digit >= 0 && digit <= 9)
            {
                if (!int_part_ended)
                    number = number * 10 + digit;
                else
                {
                    i++;
                    number += digit / pow(10, i);
                }
            }
        }
        return number * (*str == '-' ? -1.0f : 1.0f);
    }

    std::tuple<std::vector<OpenGLVertex>, std::vector<uint32_t>> ModelLoader::LoadOBJ(const std::string& modelPath)
    {
        FL_SCOPED_TIMER("Load_OBJ_v2");
        std::vector<OpenGLVertex> vertices;
        std::vector<uint32_t> indices;

        std::vector<float> positions;
        std::vector<float> textureUVs;
        std::vector<float> normals;

        float textureIndex = 0.0f;
        int entityID = -1;

        std::vector<uint32_t> faceIndices;
        faceIndices.reserve(3);

        TagTypeOBJ currentTagType = TagTypeOBJ::NONE;

        // (Use fopen with 'r') (windows solution)
        // Try mmap
        int fd = open(modelPath.c_str(), O_RDONLY, S_IRUSR | S_IWUSR);
        struct stat sb;

        if (fstat(fd, &sb) == -1)
            FL_ERROR("Couldn't get the file size!");

        char* file_in_memory = (char*)mmap(NULL, sb.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
        char* token = file_in_memory;

        while (*token != '\0')
        {
            // Identifying the current line tag
            if (*token == 'v')
            {
                token++;
                switch (*token)
                {
                case ' ': currentTagType = TagTypeOBJ::VERTEX_POSITION; token++; break;
                case 't': currentTagType = TagTypeOBJ::VERTEX_TEXTURE_UV; token += 2; break;
                case 'n': currentTagType = TagTypeOBJ::VERTEX_NORMAL; token += 2; break;
                }
            }
            else if (*token == 'f')
            {
                currentTagType = TagTypeOBJ::FACE;
                token += 2;
            }
            else
            {
                token += strcspn(token, "\n") + 1;
                continue;
            } // skip current iteration as we have nothing to read in the line

            while (*token != '\n')
            {
                switch (currentTagType)
                {
                case TagTypeOBJ::VERTEX_POSITION: {
                    positions.push_back(ParseFloat(token));
                    size_t end = strcspn(token, "\n");
                    size_t nextSpace = strcspn(token, " ");
                    token += end > nextSpace ? nextSpace + 1 : end;
                    break;
                }
                case TagTypeOBJ::VERTEX_TEXTURE_UV: {
                    textureUVs.push_back(ParseFloat(token));
                    size_t end = strcspn(token, "\n");
                    size_t nextSpace = strcspn(token, " ");
                    token += end > nextSpace ? nextSpace + 1 : end;
                    break;
                }
                case TagTypeOBJ::VERTEX_NORMAL: {
                    normals.push_back(ParseFloat(token));
                    size_t end = strcspn(token, "\n");
                    size_t nextSpace = strcspn(token, " ");
                    token += end > nextSpace ? nextSpace + 1 : end;
                    break;
                }
                case TagTypeOBJ::FACE: {
                    uint16_t faceVertices = 0;
                    uint32_t parsedInteger = 0;
                    while (*token != '\n')
                    {
                        bool wordBreak = false;
                        if (*token == '/')
                        {
                            faceIndices.push_back(parsedInteger);
                            parsedInteger = 0;
                        }
                        else if (*token == ' ')
                            wordBreak = true;
                        else
                            parsedInteger = parsedInteger * 10 + (*token) - '0';

                        if (*(token + 1) == '\n')
                            wordBreak = true;

                        if (wordBreak)
                        {
                            faceIndices.push_back(parsedInteger);
                            parsedInteger = 0;

                            auto& vertex = vertices.emplace_back();
                            vertex.position = { positions[(faceIndices[0] - 1) * 3], positions[(faceIndices[0] - 1) * 3 + 1], positions[(faceIndices[0] - 1) * 3 + 2] };
                            vertex.texture_uv = { textureUVs[(faceIndices[1] - 1) * 2], textureUVs[(faceIndices[1] - 1) * 2 + 1] };
                            vertex.normal = { normals[(faceIndices[2] - 1) * 3], normals[(faceIndices[2] - 1) * 3 + 1], normals[(faceIndices[2] - 1) * 3 + 2] };
                            vertex.texture_index = textureIndex;
                            vertex.entityID = entityID;

                            faceIndices.clear();
                            faceVertices++;
                        }
                        token++;
                    }

                    // Add indices
                    switch (faceVertices)
                    {
                    case 3:
                    {
                        indices.push_back((uint32_t)vertices.size() - 3);
                        indices.push_back((uint32_t)vertices.size() - 2);
                        indices.push_back((uint32_t)vertices.size() - 1);
                        break;
                    }
                    case 4:
                    {
                        uint32_t offset = (uint32_t)vertices.size() - 4;
                        indices.push_back(1 + offset);
                        indices.push_back(2 + offset);
                        indices.push_back(3 + offset);

                        indices.push_back(3 + offset);
                        indices.push_back(0 + offset);
                        indices.push_back(1 + offset);
                        break;
                    }
                    default: break;
                    }
                    break;
                }
                case TagTypeOBJ::NONE: token++; break;
                default: token++; break;
                }
            }
            token++;
        }

        if (munmap((void*)file_in_memory, sb.st_size) == -1)
            FL_ERROR("Failed to unmap contents of file: {0}", modelPath);
        close(fd);

        return { vertices, indices };
    }
}
