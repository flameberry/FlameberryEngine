#include "Scene.h"

#include "Core/Timer.h"
#include "Core/Profiler.h"
#include "Components.h"

#include "Physics/Physics.h"
#include "PxPhysicsAPI.h"

namespace Flameberry {
    Scene::Scene()
        : m_Registry(std::make_shared<fbentt::registry>())
    {
    }

    Scene::Scene(const std::shared_ptr<Scene>& other)
        : m_Registry(std::make_shared<fbentt::registry>(*other->m_Registry)), m_Name(other->m_Name), m_Environment(other->m_Environment)
    {
        FL_LOG("Copying Scene...");
    }

    Scene::~Scene()
    {
        FL_LOG("Deleting Scene...");
    }

    void Scene::OnStartRuntime()
    {
        physx::PxSceneDesc sceneDesc(PhysicsContext::GetTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
        sceneDesc.cpuDispatcher = PhysicsContext::GetCPUDispatcher();
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

        // Create Scene
        m_PxScene = PhysicsContext::GetPhysics()->createScene(sceneDesc);

        // Create Actors
        for (auto entity : m_Registry->view<TransformComponent, RigidBodyComponent>())
        {
            auto [transform, rigidBody] = m_Registry->get<TransformComponent, RigidBodyComponent>(entity);

            const auto quat = glm::quat(transform.Rotation);
            const auto transformMat = physx::PxTransform(
                physx::PxVec3(transform.Translation.x, transform.Translation.y, transform.Translation.z),
                physx::PxQuat(quat.x, quat.y, quat.z, quat.w)
            );
            switch (rigidBody.Type)
            {
                case RigidBodyComponent::RigidBodyType::Static: {
                        physx::PxRigidStatic* staticBody = PhysicsContext::GetPhysics()->createRigidStatic(transformMat);
                        rigidBody.RuntimeRigidBody = staticBody;
                        break;
                    }
                case RigidBodyComponent::RigidBodyType::Dynamic: {
                        physx::PxRigidDynamic* dynamicBody = PhysicsContext::GetPhysics()->createRigidDynamic(transformMat);
                        rigidBody.RuntimeRigidBody = dynamicBody;
                        break;
                    }
            }

            FL_ASSERT(rigidBody.RuntimeRigidBody, "Failed to create RigidBody!");

            physx::PxShape* shape = nullptr;

            if (auto* boxCollider = m_Registry->try_get<BoxColliderComponent>(entity); boxCollider)
            {
                auto geometry = physx::PxBoxGeometry(
                    0.5f * boxCollider->Size.x * transform.Scale.x,
                    0.5f * boxCollider->Size.y * transform.Scale.y,
                    0.5f * boxCollider->Size.z * transform.Scale.z
                );
                auto* material = PhysicsContext::GetPhysics()->createMaterial(rigidBody.StaticFriction, rigidBody.DynamicFriction, rigidBody.Restitution);
                shape = PhysicsContext::GetPhysics()->createShape(geometry, *material);
                boxCollider->RuntimeShape = shape;
            }

            if (auto* sphereCollider = m_Registry->try_get<SphereColliderComponent>(entity); sphereCollider)
            {
                auto geometry = physx::PxSphereGeometry(sphereCollider->Radius * glm::max(glm::max(transform.Scale.x, transform.Scale.y), transform.Scale.z));
                auto* material = PhysicsContext::GetPhysics()->createMaterial(rigidBody.StaticFriction, rigidBody.DynamicFriction, rigidBody.Restitution);
                shape = PhysicsContext::GetPhysics()->createShape(geometry, *material);
                sphereCollider->RuntimeShape = shape;
            }

            if (shape)
            {
                physx::PxRigidBody* rigidBodyRuntimePtr = (physx::PxRigidBody*)rigidBody.RuntimeRigidBody;
                rigidBodyRuntimePtr->attachShape(*shape);
                if (rigidBody.Type == RigidBodyComponent::RigidBodyType::Dynamic)
                    physx::PxRigidBodyExt::updateMassAndInertia(*rigidBodyRuntimePtr, rigidBody.Density);
                m_PxScene->addActor(*rigidBodyRuntimePtr);
                shape->release();
            }
        }
    }

    void Scene::OnStopRuntime()
    {
        m_PxScene->release();
    }

    void Scene::OnUpdateRuntime(float delta)
    {
        // for (auto entity : m_Registry->view<TransformComponent>()) {
        //     m_Registry->get<TransformComponent>(entity).Rotation.y += 2.0f * delta;
        // }

        FL_PROFILE_SCOPE("Scene::OnUpdateRuntime");

        // Update Physics
        m_PxScene->simulate(delta);
        m_PxScene->fetchResults(true);

        for (auto entity : m_Registry->view<RigidBodyComponent>())
        {
            auto [transform, rigidBody] = m_Registry->get<TransformComponent, RigidBodyComponent>(entity);
            physx::PxRigidBody* rigidBodyRuntimePtr = (physx::PxRigidBody*)rigidBody.RuntimeRigidBody;

            physx::PxTransform globalTransform = rigidBodyRuntimePtr->getGlobalPose();
            transform.Translation = { globalTransform.p.x, globalTransform.p.y , globalTransform.p.z };
            transform.Rotation = glm::eulerAngles(glm::quat(globalTransform.q.w, globalTransform.q.x, globalTransform.q.y, globalTransform.q.z));
        }
    }

    void Scene::RenderScene(const glm::mat4& cameraMatrix)
    {
    }
}
