#include "Project.h"

namespace Flameberry {
    
    Project::Project(const std::filesystem::path& projectDirectory, const ProjectConfig &config)
        : m_ProjectDirectory(projectDirectory), m_Config(config)
    {
    }
    
    Project::~Project() {}
    
}
