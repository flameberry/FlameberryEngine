#include "Scene.h"

#include "Core/Timer.h"
#include "Core/Profiler.h"
#include "Components.h"

#include "Physics/PhysicsEngine.h"
#include "PxPhysicsAPI.h"

#include "Scripting/ScriptEngine.h"

namespace Flameberry {
    Scene::Scene()
        : m_Registry(CreateRef<fbentt::registry>())
    {
    }

    Scene::Scene(const Ref<Scene>& other)
        : m_Registry(CreateRef<fbentt::registry>(*other->m_Registry)), m_Name(other->m_Name), m_ViewportSize(other->m_ViewportSize)
    {
        FBY_LOG("Copying Scene...");
    }

    Scene::Scene(const Scene& other)
        : m_Registry(CreateRef<fbentt::registry>(*other.m_Registry)), m_Name(other.m_Name), m_ViewportSize(other.m_ViewportSize)
    {
    }

    Scene::~Scene()
    {
        FBY_LOG("Deleting Scene...");
    }

    void Scene::OnStartRuntime()
    {
        m_IsRunning = true;

        // Update Cameras
        for (auto entity : m_Registry->view<CameraComponent>())
        {
            auto& cameraComp = m_Registry->get<CameraComponent>(entity);
            cameraComp.Camera.UpdateWithAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
        }

        // Create Script Actors
        for (auto entity : m_Registry->view<NativeScriptComponent>())
        {
            auto& nsc = m_Registry->get<NativeScriptComponent>(entity);
            nsc.Actor = nsc.InitScript();
            nsc.Actor->m_SceneRef = this;
            nsc.Actor->m_Entity = entity;

            nsc.Actor->OnInstanceCreated();
        }

        ScriptEngine::OnRuntimeStart(this);

        // Create Physics Context
        physx::PxSceneDesc sceneDesc(PhysicsEngine::GetTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
        sceneDesc.cpuDispatcher = PhysicsEngine::GetCPUDispatcher();
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

        // Create Physics Scene
        m_PxScene = PhysicsEngine::GetPhysics()->createScene(sceneDesc);

        // Create Physics Actors
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
                    physx::PxRigidStatic* staticBody = PhysicsEngine::GetPhysics()->createRigidStatic(transformMat);
                    rigidBody.RuntimeRigidBody = staticBody;
                    break;
                }
                case RigidBodyComponent::RigidBodyType::Dynamic: {
                    physx::PxRigidDynamic* dynamicBody = PhysicsEngine::GetPhysics()->createRigidDynamic(transformMat);
                    rigidBody.RuntimeRigidBody = dynamicBody;
                    break;
                }
            }

            FBY_ASSERT(rigidBody.RuntimeRigidBody, "Failed to create RigidBody!");

            physx::PxShape* shape = nullptr;

            if (auto* boxCollider = m_Registry->try_get<BoxColliderComponent>(entity); boxCollider)
            {
                auto geometry = physx::PxBoxGeometry(
                    0.5f * boxCollider->Size.x * transform.Scale.x,
                    0.5f * boxCollider->Size.y * transform.Scale.y,
                    0.5f * boxCollider->Size.z * transform.Scale.z
                );
                auto* material = PhysicsEngine::GetPhysics()->createMaterial(rigidBody.StaticFriction, rigidBody.DynamicFriction, rigidBody.Restitution);
                shape = PhysicsEngine::GetPhysics()->createShape(geometry, *material);
                boxCollider->RuntimeShape = shape;
            }

            if (auto* sphereCollider = m_Registry->try_get<SphereColliderComponent>(entity); sphereCollider)
            {
                auto geometry = physx::PxSphereGeometry(sphereCollider->Radius * glm::max(glm::max(transform.Scale.x, transform.Scale.y), transform.Scale.z));
                auto* material = PhysicsEngine::GetPhysics()->createMaterial(rigidBody.StaticFriction, rigidBody.DynamicFriction, rigidBody.Restitution);
                shape = PhysicsEngine::GetPhysics()->createShape(geometry, *material);
                sphereCollider->RuntimeShape = shape;
            }

            if (auto* capsuleCollider = m_Registry->try_get<CapsuleColliderComponent>(entity); capsuleCollider)
            {
                auto geometry = physx::PxCapsuleGeometry(capsuleCollider->Radius * glm::max(transform.Scale.x, transform.Scale.z), 0.5f * capsuleCollider->Height * transform.Scale.y);
                auto* material = PhysicsEngine::GetPhysics()->createMaterial(rigidBody.StaticFriction, rigidBody.DynamicFriction, rigidBody.Restitution);
                shape = PhysicsEngine::GetPhysics()->createShape(geometry, *material);

                switch (capsuleCollider->Axis)
                {
                    case AxisType::X:
                        break;
                    case AxisType::Y:
                    {
                        physx::PxTransform relativePose(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0, 0, 1)));
                        shape->setLocalPose(relativePose);
                        break;
                    }
                    case AxisType::Z:
                    {
                        physx::PxTransform relativePose(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(1, 0, 0)));
                        shape->setLocalPose(relativePose);
                        break;
                    }
                }
                capsuleCollider->RuntimeShape = shape;
            }

            if (shape)
            {
                // TODO: Add axis locking
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
        m_IsRunning = false;

        m_PxScene->release();

        ScriptEngine::OnRuntimeStop();

        // Delete Script Actors
        for (auto entity : m_Registry->view<NativeScriptComponent>())
        {
            auto& nsc = m_Registry->get<NativeScriptComponent>(entity);
            nsc.Actor->OnInstanceDeleted();
            nsc.DestroyScript(&nsc);
        }
    }

    void Scene::OnUpdateRuntime(float delta)
    {
        FBY_PROFILE_SCOPE("Scene::OnUpdateRuntime");

        // Update Native Scripts
        for (auto entity : m_Registry->view<NativeScriptComponent>())
        {
            auto& nsc = m_Registry->get<NativeScriptComponent>(entity);
            nsc.Actor->OnUpdate(delta);
        }

        // Update CSharp Scripts
        ScriptEngine::OnRuntimeUpdate(delta);

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

    void Scene::OnViewportResize(const glm::vec2& viewportSize)
    {
        m_ViewportSize = viewportSize;

        // TODO: ECS: Implement iterating over single pool without having the knowledge of entity
        for (auto entity : m_Registry->view<CameraComponent>())
        {
            auto& cameraComp = m_Registry->get<CameraComponent>(entity);
            // TODO: This is fishy, maybe update this only when runtime is started
            cameraComp.Camera.UpdateWithAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
        }
    }

    fbentt::entity Scene::GetPrimaryCameraEntity() const
    {
        for (auto entity : m_Registry->view<CameraComponent>())
        {
            auto& cameraComp = m_Registry->get<CameraComponent>(entity);
            if (cameraComp.IsPrimary)
                return entity;
        }
        return {};
    }

    void Scene::RenderScene(const glm::mat4& cameraMatrix)
    {
    }

    fbentt::entity Scene::CreateEntityWithTagAndParent(const std::string& tag, fbentt::entity parent)
    {
        auto entity = m_Registry->create();
        m_Registry->emplace<IDComponent>(entity);
        m_Registry->emplace<TagComponent>(entity).Tag = tag;
        m_Registry->emplace<TransformComponent>(entity);

        if (parent != fbentt::null)
        {
            m_Registry->emplace<RelationshipComponent>(entity);

            if (!m_Registry->has<RelationshipComponent>(parent))
                m_Registry->emplace<RelationshipComponent>(parent);

            auto& relation = m_Registry->get<RelationshipComponent>(entity);
            relation.Parent = parent;

            auto& parentRel = m_Registry->get<RelationshipComponent>(parent);
            if (parentRel.FirstChild == fbentt::null)
                parentRel.FirstChild = entity;
            else
            {
                auto sibling = parentRel.FirstChild;

                while (m_Registry->get<RelationshipComponent>(sibling).NextSibling != fbentt::null)
                    sibling = m_Registry->get<RelationshipComponent>(sibling).NextSibling;

                auto& siblingRel = m_Registry->get<RelationshipComponent>(sibling);
                siblingRel.NextSibling = entity;
                relation.PrevSibling = sibling;
            }
        }
        return entity;
    }

    void Scene::DestroyEntityTree(fbentt::entity entity)
    {
        if (entity == fbentt::null)
            return;

        if (m_Registry->has<RelationshipComponent>(entity))
        {
            auto& relation = m_Registry->get<RelationshipComponent>(entity);
            auto sibling = relation.FirstChild;
            while (sibling != fbentt::null)
            {
                auto temp = m_Registry->get<RelationshipComponent>(sibling).NextSibling;
                DestroyEntityTree(sibling);
                sibling = temp;
            }

            if (relation.Parent != fbentt::null)
            {
                auto& parentRel = m_Registry->get<RelationshipComponent>(relation.Parent);
                if (parentRel.FirstChild == entity)
                    parentRel.FirstChild = relation.NextSibling;
            }
            if (relation.PrevSibling != fbentt::null)
                m_Registry->get<RelationshipComponent>(relation.PrevSibling).NextSibling = relation.NextSibling;
            if (relation.NextSibling != fbentt::null)
                m_Registry->get<RelationshipComponent>(relation.NextSibling).PrevSibling = relation.PrevSibling;
        }
        m_Registry->destroy(entity);
    }

    void Scene::ReparentEntity(fbentt::entity entity, fbentt::entity destParent)
    {
        FBY_ASSERT(entity != fbentt::null, "Can't reparent null entity!");

        if (entity == destParent)
            return;

        if (destParent == fbentt::null)
        {
            if (IsEntityRoot(entity))
                return;

            auto& relation = m_Registry->get<RelationshipComponent>(entity);

            if (relation.PrevSibling == fbentt::null)
                m_Registry->get<RelationshipComponent>(relation.Parent).FirstChild = relation.NextSibling;
            else
                m_Registry->get<RelationshipComponent>(relation.PrevSibling).NextSibling = relation.NextSibling;
            if (relation.NextSibling != fbentt::null)
                m_Registry->get<RelationshipComponent>(relation.NextSibling).PrevSibling = relation.PrevSibling;

            relation.Parent = fbentt::null;
            relation.PrevSibling = fbentt::null;
            relation.NextSibling = fbentt::null;
        }
        else
        {
            if (IsEntityInHierarchy(destParent, entity))
                return;

            if (!m_Registry->has<RelationshipComponent>(entity))
                m_Registry->emplace<RelationshipComponent>(entity);

            if (!m_Registry->has<RelationshipComponent>(destParent))
                m_Registry->emplace<RelationshipComponent>(destParent);

            auto& relation = m_Registry->get<RelationshipComponent>(entity);

            auto oldParent = relation.Parent;
            if (oldParent != fbentt::null)
            {
                auto& oldParentRel = m_Registry->get<RelationshipComponent>(oldParent);
                if (oldParentRel.FirstChild == entity)
                    oldParentRel.FirstChild = relation.NextSibling;
            }
            if (relation.PrevSibling != fbentt::null)
                m_Registry->get<RelationshipComponent>(relation.PrevSibling).NextSibling = relation.NextSibling;
            if (relation.NextSibling != fbentt::null)
                m_Registry->get<RelationshipComponent>(relation.NextSibling).PrevSibling = relation.PrevSibling;

            auto& newParentRel = m_Registry->get<RelationshipComponent>(destParent);
            relation.NextSibling = newParentRel.FirstChild;
            relation.PrevSibling = fbentt::null;
            relation.Parent = destParent;

            if (relation.NextSibling != fbentt::null)
                m_Registry->get<RelationshipComponent>(relation.NextSibling).PrevSibling = entity;
            newParentRel.FirstChild = entity;
        }
    }

    bool Scene::Recursive_IsEntityInHierarchy(fbentt::entity key, fbentt::entity parent)
    {
        auto* relation = m_Registry->try_get<RelationshipComponent>(parent);
        auto sibling = parent;
        while (relation && sibling != fbentt::null)
        {
            if (sibling == key)
                return true;

            if (relation->FirstChild != fbentt::null && Recursive_IsEntityInHierarchy(key, relation->FirstChild))
                return true;

            sibling = relation->NextSibling;
            relation = m_Registry->try_get<RelationshipComponent>(sibling);
        }
        return false;
    }

    bool Scene::IsEntityInHierarchy(fbentt::entity key, fbentt::entity parent)
    {
        auto* relation = m_Registry->try_get<RelationshipComponent>(parent);
        return relation ? Recursive_IsEntityInHierarchy(key, relation->FirstChild) : false;
    }

    bool Scene::IsEntityRoot(fbentt::entity entity)
    {
        auto* rel = m_Registry->try_get<RelationshipComponent>(entity);
        return !rel || rel->Parent == fbentt::null;
    }

    template<typename... Component>
    static void CopyComponentIfExists(Scene* context, fbentt::entity dest, fbentt::entity src)
    {
        ([&]()
            {
                if (auto* comp = context->GetRegistry()->try_get<Component>(src); comp)
                    context->GetRegistry()->emplace<Component>(dest, *comp);
            }(), ...);
    }

    template<typename... Component>
    static void CopyComponentIfExists(ComponentList<Component...>, Scene* context, fbentt::entity dest, fbentt::entity src)
    {
        CopyComponentIfExists<Component...>(context, dest, src);
    }

    fbentt::entity Scene::DuplicateEntity(fbentt::entity src)
    {
        if (m_Registry->has<RelationshipComponent>(src))
        {
            fbentt::entity duplicateEntity = DuplicateEntityTree(src);

            auto& destRelation = m_Registry->get<RelationshipComponent>(duplicateEntity);
            auto& srcRelation = m_Registry->get<RelationshipComponent>(src);

            // Handling the `duplicateEntity`'s relation component
            if (srcRelation.Parent != fbentt::null)
            {
                fbentt::entity srcNextSibling = srcRelation.NextSibling;
                auto* srcNextSiblingRel = m_Registry->try_get<RelationshipComponent>(srcNextSibling);

                destRelation.Parent = srcRelation.Parent;
                destRelation.PrevSibling = src;
                destRelation.NextSibling = srcNextSibling;

                srcRelation.NextSibling = duplicateEntity;
                if (srcNextSiblingRel)
                    srcNextSiblingRel->PrevSibling = duplicateEntity;
            }
            return duplicateEntity;
        }
        return DuplicateSingleEntity(src);
    }

    fbentt::entity Scene::DuplicateSingleEntity(fbentt::entity src)
    {
        const auto destEntity = m_Registry->create();
        m_Registry->emplace<IDComponent>(destEntity);
        m_Registry->emplace<TagComponent>(destEntity, m_Registry->get<TagComponent>(src).Tag);

        CopyComponentIfExists(AllComponents{}, this, destEntity, src);
        return destEntity;
    }

    fbentt::entity Scene::DuplicateEntityTree(fbentt::entity src)
    {
        if (src == fbentt::null)
            return fbentt::null;

        const auto destEntity = DuplicateSingleEntity(src);
        auto& destRelation = m_Registry->emplace<RelationshipComponent>(destEntity);

        auto& srcRelation = m_Registry->get<RelationshipComponent>(src);
        fbentt::entity child = srcRelation.FirstChild;

        // Intermediate Variables
        fbentt::entity prevDestChild = fbentt::null;
        RelationshipComponent* prevDestChildRel = nullptr;

        while (child != fbentt::null)
        {
            // Create a copy of each children of `src`
            fbentt::entity destChild = DuplicateEntityTree(child);

            // Setup the Relationship Component of the copy of children of `src`
            auto& destChildRel = m_Registry->get<RelationshipComponent>(destChild);
            destChildRel.Parent = destEntity;
            destChildRel.PrevSibling = prevDestChild;
            if (prevDestChildRel = m_Registry->try_get<RelationshipComponent>(prevDestChild); prevDestChildRel)
                prevDestChildRel->NextSibling = destChild;

            // Set FirstChild variable of `destEntity`
            if (prevDestChild == fbentt::null)
                m_Registry->get<RelationshipComponent>(destEntity).FirstChild = destChild;

            prevDestChild = destChild;
            child = m_Registry->get<RelationshipComponent>(child).NextSibling;
        }
        return destEntity;
    }

}
