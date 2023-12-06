#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ecs.hpp"
#include "Asset/Asset.h"
#include "Renderer/GenericCamera.h"
#include "Actor.h"

namespace Flameberry {
    struct IDComponent
    {
        UUID ID;
        IDComponent(UUID id) : ID(id) {}
        IDComponent() = default;
    };

    struct TransformComponent
    {
        glm::vec3 Translation, Rotation, Scale;
        TransformComponent() : Translation(0.0f), Rotation(0.0f), Scale(1.0f) {};
        glm::mat4 GetTransform() const
        {
            return glm::translate(glm::mat4(1.0f), Translation)
                * glm::toMat4(glm::quat(Rotation))
                * glm::scale(glm::mat4(1.0f), Scale);
        }
    };

    struct TagComponent
    {
        std::string Tag;
        TagComponent(const std::string& tag = "Default_Entity") : Tag(tag) {}
    };

    struct CameraComponent
    {
        GenericCamera Camera;
        bool IsPrimary = false;
    };

    struct SkyLightComponent
    {
        glm::vec3 Color{ 0.0f };
        float Intensity = 0.2f;
        bool EnableSkyMap = false, EnableReflections = false;

        AssetHandle SkyMap = 0;
    };

    // struct MaterialTable {
    //     std::unordered_map<uint32_t, AssetHandle> Table;

    //     // TODO: Use to potentially determine if submesh has an overriden material present in the Table without having to look it up by hashing everytime
    //     std::vector<uint64_t> SubmeshBitmask;
    // };

    typedef std::unordered_map<uint32_t, AssetHandle> MaterialTable;
    struct MeshComponent
    {
        AssetHandle MeshHandle;

        // This stores the materials that are used for rendering instead of the default ones which are loaded from the mesh source file
        MaterialTable OverridenMaterialTable;

        MeshComponent(AssetHandle meshHandle = 0) : MeshHandle(meshHandle) {}
    };

    struct DirectionalLightComponent
    {
        glm::vec3 Color;
        float Intensity, LightSize;

        DirectionalLightComponent() : Color(1.0f), Intensity(10.0f), LightSize(20.0f) {}
    };

    struct PointLightComponent
    {
        glm::vec3 Color;
        float Intensity;

        PointLightComponent() : Color(1.0f), Intensity(10.0f) {}
    };

    struct RelationshipComponent
    {
        fbentt::entity Parent{};
        fbentt::entity FirstChild{};
        fbentt::entity PrevSibling{};
        fbentt::entity NextSibling{};

        RelationshipComponent()
            : Parent(fbentt::null), FirstChild(fbentt::null), PrevSibling(fbentt::null), NextSibling(fbentt::null)
        {
        }

        RelationshipComponent(const RelationshipComponent& dest)
            : Parent(dest.Parent), FirstChild(dest.FirstChild), PrevSibling(dest.PrevSibling), NextSibling(dest.NextSibling)
        {
        }
    };

    struct NativeScriptComponent
    {
        Flameberry::Actor* Actor;

        Flameberry::Actor* (*InitScript)();
        void (*DestroyScript)(NativeScriptComponent*);

        template<typename T>
        void Bind()
        {
            InitScript = []() { return static_cast<Flameberry::Actor*>(new T()); };
            DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Actor; nsc->Actor = nullptr; };
        }
    };

    struct RigidBodyComponent
    {
        enum class RigidBodyType : uint8_t { Static = 0, Dynamic };
        RigidBodyType Type = RigidBodyType::Static;

        float Density = 10.0f;
        float StaticFriction = 0.5f, DynamicFriction = 0.7f, Restitution = 0.1f;

        void* RuntimeRigidBody = nullptr;
    };

    struct BoxColliderComponent
    {
        glm::vec3 Size{ 1.0f };

        void* RuntimeShape = nullptr;
    };

    struct SphereColliderComponent
    {
        float Radius = 1.0f;

        void* RuntimeShape = nullptr;
    };

    struct CapsuleColliderComponent
    {
        enum class AxisType : uint8_t { X = 0, Y, Z };
        AxisType Axis = AxisType::Y;
        float Radius = 0.5f, Height = 1.0f;

        void* RuntimeShape = nullptr;
    };

    template <typename... Component>
    struct ComponentList {};

    using AllComponents = ComponentList<TransformComponent, CameraComponent, SkyLightComponent, MeshComponent, DirectionalLightComponent, PointLightComponent, NativeScriptComponent, RigidBodyComponent, BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent>;

}
