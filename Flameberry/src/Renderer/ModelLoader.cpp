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
    enum TagTypeOBJ
    {
        NONE = 0, VERTEX_POSITION, VERTEX_TEXTURE_UV, VERTEX_NORMAL, FACE
    };

    float ModelLoader::ParseFloat(const char* str, char delimiter)
    {
        // FL_SCOPED_TIMER("Parse_Float");
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

#if 0
    std::tuple<std::vector<OpenGLVertex>, std::vector<uint32_t>> ModelLoader::LoadOBJ(const std::string& modelPath)
    {
        FL_SCOPED_TIMER("Load_OBJ_v1");
        std::vector<OpenGLVertex> vertices;
        std::vector<uint32_t> indices;

        std::vector<float> positions;
        std::vector<float> textureUVs;
        std::vector<float> normals;

        std::vector<uint32_t> index_values;
        index_values.reserve(3);

        std::string tempString;
        std::ifstream stream(modelPath);
        FL_ASSERT(stream.is_open(), "Failed to open the model path: {0}!", modelPath);

        while (stream.good())
        {
            static uint32_t currentLineNumber = 0;
            getline(stream, tempString);
            currentLineNumber++;
            std::string_view tempStrView(tempString.c_str());

            size_t start = 0, end = tempStrView.find_first_of(' ');
            std::string_view currentTag = tempStrView.substr(start, end);
            start = tempStrView.find_first_not_of(' ', end);

            uint32_t faceVertices = 0;
            while (start != std::string::npos)
            {
                // Invalidate end variable
                end = tempStrView.find(' ', start);

                // Perform operations on substring
                std::string_view subStr = tempStrView.substr(start, end - start);
                if (currentTag == "v")
                    positions.push_back(std::stof(subStr.data()));
                else if (currentTag == "vt")
                    textureUVs.push_back(std::stof(subStr.data()));
                else if (currentTag == "vn")
                    normals.push_back(std::stof(subStr.data()));
                else if (currentTag == "f")
                {
                    uint32_t index_val = 0;
                    for (const auto& character : subStr)
                    {
                        if (character == '/')
                        {
                            index_values.push_back(index_val);
                            index_val = 0;
                        }
                        else
                        {
                            index_val = index_val * 10 + character - '0';
                        }
                    }
                    index_values.push_back(index_val);

                    auto& vertex = vertices.emplace_back();
                    vertex.position = { positions[(index_values[0] - 1) * 3], positions[(index_values[0] - 1) * 3 + 1], positions[(index_values[0] - 1) * 3 + 2] };
                    vertex.texture_uv = { textureUVs[(index_values[1] - 1) * 2], textureUVs[(index_values[1] - 1) * 2 + 1] };
                    vertex.normal = { normals[(index_values[2] - 1) * 3], normals[(index_values[2] - 1) * 3 + 1], normals[(index_values[2] - 1) * 3 + 2] };
                    vertex.texture_index = 0.0f;

                    index_values.clear();
                    faceVertices++;
                }

                // Invalidate start variable
                start = tempStrView.find_first_not_of(' ', end);
            }

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
            default:
                break;
            }
        }
        stream.close();
        return { vertices, indices };
    }
#else
    std::tuple<std::vector<OpenGLVertex>, std::vector<uint32_t>> ModelLoader::LoadOBJ(const std::string& modelPath)
    {
        FL_SCOPED_TIMER("Load_OBJ_v2");
        std::vector<OpenGLVertex> vertices;
        std::vector<uint32_t> indices;

        std::vector<float> positions;
        std::vector<float> textureUVs;
        std::vector<float> normals;

        std::vector<uint32_t> faceIndices;
        faceIndices.reserve(3);

        TagTypeOBJ currentTagType = TagTypeOBJ::NONE;

        // Trying mmap
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
                        if (*token == '/')
                        {
                            faceIndices.push_back(parsedInteger);
                            parsedInteger = 0;
                        }
                        else if (*token == ' ')
                        {
                            faceIndices.push_back(parsedInteger);
                            parsedInteger = 0;

                            auto& vertex = vertices.emplace_back();
                            vertex.position = { positions[(faceIndices[0] - 1) * 3], positions[(faceIndices[0] - 1) * 3 + 1], positions[(faceIndices[0] - 1) * 3 + 2] };
                            vertex.texture_uv = { textureUVs[(faceIndices[1] - 1) * 2], textureUVs[(faceIndices[1] - 1) * 2 + 1] };
                            vertex.normal = { normals[(faceIndices[2] - 1) * 3], normals[(faceIndices[2] - 1) * 3 + 1], normals[(faceIndices[2] - 1) * 3 + 2] };
                            vertex.texture_index = 0.0f;

                            faceIndices.clear();
                            faceVertices++;
                        }
                        else
                        {
                            parsedInteger = parsedInteger * 10 + (*token) - '0';
                        }
                        token++;
                    }
                    faceIndices.push_back(parsedInteger);
                    parsedInteger = 0;

                    auto& vertex = vertices.emplace_back();
                    vertex.position = { positions[(faceIndices[0] - 1) * 3], positions[(faceIndices[0] - 1) * 3 + 1], positions[(faceIndices[0] - 1) * 3 + 2] };
                    vertex.texture_uv = { textureUVs[(faceIndices[1] - 1) * 2], textureUVs[(faceIndices[1] - 1) * 2 + 1] };
                    vertex.normal = { normals[(faceIndices[2] - 1) * 3], normals[(faceIndices[2] - 1) * 3 + 1], normals[(faceIndices[2] - 1) * 3 + 2] };
                    vertex.texture_index = 0.0f;
                    faceVertices++;

                    faceIndices.clear();

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

        // Standard method
        // std::vector<OpenGLVertex> vertices;
        // std::vector<uint32_t> indices;

        // std::vector<float> positions;
        // std::vector<float> textureUVs;
        // std::vector<float> normals;

        // std::vector<uint32_t> faceIndices;
        // faceIndices.reserve(3);

        // TagTypeOBJ currentTagType = TagTypeOBJ::NONE;
        // std::string currentLine;
        // std::ifstream stream(modelPath);
        // FL_ASSERT(stream.is_open(), "Failed to open the model path: {0}!", modelPath);

        // while (stream.good())
        // {
        //     getline(stream, currentLine);

        //     char* currentChar = currentLine.data();
        //     if (*currentChar == 'v')
        //     {
        //         currentChar++;
        //         switch (*currentChar)
        //         {
        //         case ' ': currentTagType = TagTypeOBJ::VERTEX_POSITION; currentChar++; break;
        //         case 't': currentTagType = TagTypeOBJ::VERTEX_TEXTURE_UV; currentChar += 2; break;
        //         case 'n': currentTagType = TagTypeOBJ::VERTEX_NORMAL; currentChar += 2; break;
        //         }
        //     }
        //     else if (*currentChar == 'f')
        //     {
        //         currentTagType = TagTypeOBJ::FACE;
        //         currentChar += 2;
        //     }
        //     else
        //         continue; // skip current iteration as we have nothing to read in the line

        //     while (*currentChar != '\0')
        //     {
        //         switch (currentTagType)
        //         {
        //         case TagTypeOBJ::VERTEX_POSITION: {
        //             positions.push_back(ParseFloat(currentChar));
        //             size_t end = strcspn(currentChar, "\0");
        //             size_t nextSpace = strcspn(currentChar, " ");
        //             currentChar += end > nextSpace ? nextSpace + 1 : end;
        //             break;
        //         }
        //         case TagTypeOBJ::VERTEX_TEXTURE_UV: {
        //             textureUVs.push_back(ParseFloat(currentChar));
        //             size_t end = strcspn(currentChar, "\0");
        //             size_t nextSpace = strcspn(currentChar, " ");
        //             currentChar += end > nextSpace ? nextSpace + 1 : end;
        //             break;
        //         }
        //         case TagTypeOBJ::VERTEX_NORMAL: {
        //             normals.push_back(ParseFloat(currentChar));
        //             size_t end = strcspn(currentChar, "\0");
        //             size_t nextSpace = strcspn(currentChar, " ");
        //             currentChar += end > nextSpace ? nextSpace + 1 : end;
        //             break;
        //         }
        //         case TagTypeOBJ::FACE: {
        //             uint16_t faceVertices = 0;
        //             uint32_t parsedInteger = 0;
        //             while (*currentChar != '\0')
        //             {
        //                 if (*currentChar == '/')
        //                 {
        //                     faceIndices.push_back(parsedInteger);
        //                     parsedInteger = 0;
        //                 }
        //                 else if (*currentChar == ' ')
        //                 {
        //                     faceIndices.push_back(parsedInteger);
        //                     parsedInteger = 0;

        //                     auto& vertex = vertices.emplace_back();
        //                     vertex.position = { positions[(faceIndices[0] - 1) * 3], positions[(faceIndices[0] - 1) * 3 + 1], positions[(faceIndices[0] - 1) * 3 + 2] };
        //                     vertex.texture_uv = { textureUVs[(faceIndices[1] - 1) * 2], textureUVs[(faceIndices[1] - 1) * 2 + 1] };
        //                     vertex.normal = { normals[(faceIndices[2] - 1) * 3], normals[(faceIndices[2] - 1) * 3 + 1], normals[(faceIndices[2] - 1) * 3 + 2] };
        //                     vertex.texture_index = 0.0f;

        //                     faceIndices.clear();
        //                     faceVertices++;
        //                 }
        //                 else
        //                 {
        //                     parsedInteger = parsedInteger * 10 + (*currentChar) - '0';
        //                 }
        //                 currentChar++;
        //             }
        //             faceIndices.push_back(parsedInteger);
        //             parsedInteger = 0;

        //             auto& vertex = vertices.emplace_back();
        //             vertex.position = { positions[(faceIndices[0] - 1) * 3], positions[(faceIndices[0] - 1) * 3 + 1], positions[(faceIndices[0] - 1) * 3 + 2] };
        //             vertex.texture_uv = { textureUVs[(faceIndices[1] - 1) * 2], textureUVs[(faceIndices[1] - 1) * 2 + 1] };
        //             vertex.normal = { normals[(faceIndices[2] - 1) * 3], normals[(faceIndices[2] - 1) * 3 + 1], normals[(faceIndices[2] - 1) * 3 + 2] };
        //             vertex.texture_index = 0.0f;
        //             faceVertices++;

        //             faceIndices.clear();

        //             // Add indices
        //             switch (faceVertices)
        //             {
        //             case 3:
        //             {
        //                 indices.push_back((uint32_t)vertices.size() - 3);
        //                 indices.push_back((uint32_t)vertices.size() - 2);
        //                 indices.push_back((uint32_t)vertices.size() - 1);
        //                 break;
        //             }
        //             case 4:
        //             {
        //                 uint32_t offset = (uint32_t)vertices.size() - 4;
        //                 indices.push_back(1 + offset);
        //                 indices.push_back(2 + offset);
        //                 indices.push_back(3 + offset);

        //                 indices.push_back(3 + offset);
        //                 indices.push_back(0 + offset);
        //                 indices.push_back(1 + offset);
        //                 break;
        //             }
        //             default: break;
        //             }
        //             break;
        //         }
        //         case TagTypeOBJ::NONE: currentChar++; break;
        //         default: currentChar++; break;
        //         }
        //     }
        // }
        // stream.close();
        // return { vertices, indices };
    }
#endif
}
