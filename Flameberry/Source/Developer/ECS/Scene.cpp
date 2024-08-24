#include "Scene.h"

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/MassProperties.h>

#include "Core/Assert.h"
#include "Core/Profiler.h"
#include "Components.h"

#include "ECS/ecs.hpp"
#include "Physics/Physics.h"
#include "Physics/InterfaceImpls.h"

namespace Flameberry {

	Scene::Scene()
		: m_Registry(CreateRef<fbentt::registry>())
	{
		// Ensure that registry is empty before adding the world node
		// because the world entity should be the entity with index 0
		FBY_ASSERT(m_Registry->empty(), "Registry should be empty before adding the world entity");

		m_WorldEntity = m_Registry->create();
		m_Registry->emplace<IDComponent>(m_WorldEntity);
		m_Registry->emplace<TagComponent>(m_WorldEntity, "World");
	}

	Scene::Scene(const Ref<Scene>& other)
		: m_Registry(CreateRef<fbentt::registry>(*other->m_Registry))
		, m_Name(other->m_Name)
		, m_ViewportSize(other->m_ViewportSize)
		, m_WorldEntity(other->m_WorldEntity)
	{
		FBY_TRACE("Copying Scene...");
	}

	Scene::Scene(const Scene& other)
		: m_Registry(CreateRef<fbentt::registry>(*other.m_Registry))
		, m_Name(other.m_Name)
		, m_ViewportSize(other.m_ViewportSize)
		, m_WorldEntity(other.m_WorldEntity)
	{
		FBY_TRACE("Copying Scene...");
	}

	Scene::~Scene()
	{
		FBY_TRACE("Deleting Scene...");
	}

	void Scene::OnStartRuntime()
	{
		m_IsRuntimeActive = true;

		// Update Cameras
		for (auto& cameraComp : m_Registry->view<CameraComponent>())
			cameraComp.Camera.UpdateWithAspectRatio(m_ViewportSize.x / m_ViewportSize.y);

		// Create Script Actors
		for (auto entity : m_Registry->group<NativeScriptComponent>())
		{
			auto& nsc = m_Registry->get<NativeScriptComponent>(entity);
			nsc.Actor = nsc.InitScript();
			nsc.Actor->m_SceneRef = this;
			nsc.Actor->m_Entity = entity;

			nsc.Actor->OnInstanceCreated();
		}

		ScriptEngine::OnRuntimeStart(this);
		OnPhysicsStart();
	}

	void Scene::OnStopRuntime()
	{
		m_IsRuntimeActive = m_IsRuntimePaused = false;

		ScriptEngine::OnRuntimeStop();
		OnPhysicsStop();

		// Delete Script Actors
		for (auto& nsc : m_Registry->view<NativeScriptComponent>())
		{
			nsc.Actor->OnInstanceDeleted();
			nsc.DestroyScript(&nsc);
		}
	}

	void Scene::OnUpdateRuntime(float delta)
	{
		FBY_PROFILE_SCOPE("Scene::OnUpdateRuntime");

		if (!ShouldStep())
			return;

		// Update Native Scripts
		for (auto& nsc : m_Registry->view<NativeScriptComponent>())
			nsc.Actor->OnUpdate(delta);

		OnPhysicsSimulate(delta);
		ScriptEngine::OnRuntimeUpdate(delta);
	}

	void Scene::OnStartSimulation()
	{
		OnPhysicsStart();
	}

	void Scene::OnUpdateSimulation(float delta)
	{
		FBY_PROFILE_SCOPE("Scene::OnUpdateSimulation");

		if (!ShouldStep())
			return;

		OnPhysicsSimulate(delta);
	}

	void Scene::OnStopSimulation()
	{
		OnPhysicsStop();
	}

	void Scene::OnPhysicsStart()
	{
		for (auto entity : m_Registry->group<TransformComponent, RigidBodyComponent>())
		{
			const auto& [transform, rigidBody] = m_Registry->get<TransformComponent, RigidBodyComponent>(entity);

			JPH::ShapeRefC shapeRef;

			if (auto* boxColliderComponent = m_Registry->try_get<BoxColliderComponent>(entity))
			{
				FBY_ASSERT(shapeRef == nullptr, "Multiple type of colliders are not allowed on the same entity");

				const glm::vec3 boxColliderSize = 0.5f * boxColliderComponent->Size * transform.Scale;
				JPH::BoxShapeSettings boxShapeSettings(JPH::Vec3(boxColliderSize.x, boxColliderSize.y, boxColliderSize.z));
				boxShapeSettings.SetEmbedded();

				// Create the shape
				JPH::ShapeSettings::ShapeResult boxShapeResult = boxShapeSettings.Create();
				shapeRef = boxShapeResult.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
			}

			if (auto* sphereColliderComponent = m_Registry->try_get<SphereColliderComponent>(entity))
			{
				FBY_ASSERT(shapeRef == nullptr, "Multiple type of colliders are not allowed on the same entity");

				const float sphereColliderRadius = sphereColliderComponent->Radius * glm::max(glm::max(transform.Scale.x, transform.Scale.y), transform.Scale.z);
				JPH::SphereShapeSettings sphereShapeSettings(sphereColliderRadius);
				sphereShapeSettings.SetEmbedded();

				// Create the shape
				JPH::ShapeSettings::ShapeResult sphereShapeResult = sphereShapeSettings.Create();
				shapeRef = sphereShapeResult.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
			}

			if (auto* capsuleColliderComponent = m_Registry->try_get<CapsuleColliderComponent>(entity))
			{
				FBY_ASSERT(shapeRef == nullptr, "Multiple type of colliders are not allowed on the same entity");

				const float sphereColliderRadius = capsuleColliderComponent->Radius * glm::max(transform.Scale.x, transform.Scale.z);
				const float sphereColliderHalfHeight = 0.5f * capsuleColliderComponent->Height * transform.Scale.y;
				JPH::CapsuleShapeSettings capsuleShapeSettings(sphereColliderHalfHeight, sphereColliderRadius);
				capsuleShapeSettings.SetEmbedded();

				// Create the shape
				JPH::ShapeSettings::ShapeResult capsuleShapeResult = capsuleShapeSettings.Create();
				shapeRef = capsuleShapeResult.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
			}

			JPH::ObjectLayer objectLayer;
			JPH::EMotionType motionType;

			switch (rigidBody.Type)
			{
				case RigidBodyComponent::RigidBodyType::Static:
					objectLayer = Layers::NON_MOVING;
					motionType = JPH::EMotionType::Static;
					break;
				case RigidBodyComponent::RigidBodyType::Kinematic:
					objectLayer = Layers::MOVING;
					motionType = JPH::EMotionType::Kinematic;
					break;
				case RigidBodyComponent::RigidBodyType::Dynamic:
					objectLayer = Layers::MOVING;
					motionType = JPH::EMotionType::Dynamic;
					break;
			}

			const auto quat = glm::quat(transform.Rotation);

			JPH::BodyCreationSettings bodyCreationSettings(
				shapeRef.GetPtr(),
				JPH::RVec3(transform.Translation.x, transform.Translation.y, transform.Translation.z),
				JPH::Quat(quat.x, quat.y, quat.z, quat.w),
				motionType,
				objectLayer);

			JPH::Body* body = PhysicsManager::GetBodyInterface().CreateBody(bodyCreationSettings);

			PhysicsManager::GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate); // TODO: To Activate or Not?
			PhysicsManager::GetBodyInterface().SetFriction(body->GetID(), (rigidBody.StaticFriction + rigidBody.DynamicFriction) / 2.0f);
			PhysicsManager::GetBodyInterface().SetRestitution(body->GetID(), rigidBody.Restitution);

			rigidBody.RuntimeRigidBody = body;
		}

		// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
		// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
		// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
		PhysicsManager::OptimizeBroadPhase();
	}

	void Scene::OnPhysicsSimulate(float delta)
	{
		// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
		const int cCollisionSteps = 1;

		// Step the world
		PhysicsManager::Update(delta, cCollisionSteps);

		for (auto entity : m_Registry->group<RigidBodyComponent>())
		{
			auto [transform, rigidBody] = m_Registry->get<TransformComponent, RigidBodyComponent>(entity);
			JPH::Body* rigidBodyRuntimePtr = (JPH::Body*)rigidBody.RuntimeRigidBody;
			JPH::RVec3 position = PhysicsManager::GetBodyInterface().GetCenterOfMassPosition(rigidBodyRuntimePtr->GetID());
			JPH::Quat quat = PhysicsManager::GetBodyInterface().GetRotation(rigidBodyRuntimePtr->GetID());

			transform.Translation = { position.GetX(), position.GetY(), position.GetZ() };
			transform.Rotation = glm::eulerAngles(glm::quat(quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ()));
		}
	}

	void Scene::OnPhysicsStop()
	{
		for (auto& rigidBody : m_Registry->view<RigidBodyComponent>())
		{
			JPH::Body* body = (JPH::Body*)rigidBody.RuntimeRigidBody;
			PhysicsManager::GetBodyInterface().RemoveBody(body->GetID());
			PhysicsManager::GetBodyInterface().DestroyBody(body->GetID());
		}
	}

	bool Scene::ShouldStep()
	{
		if (m_IsRuntimePaused)
		{
			if (m_StepFrames <= 0)
				return false;
			else
				m_StepFrames--;
		}

		return true;
	}

	void Scene::OnViewportResize(const glm::vec2& viewportSize)
	{
		m_ViewportSize = viewportSize;

		for (auto& cameraComp : m_Registry->view<CameraComponent>())
		{
			// TODO: This is fishy, maybe update this only when runtime is started
			cameraComp.Camera.UpdateWithAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
		}
	}

	fbentt::entity Scene::GetPrimaryCameraEntity() const
	{
		for (auto entity : m_Registry->group<CameraComponent>())
		{
			auto& cameraComp = m_Registry->get<CameraComponent>(entity);
			if (cameraComp.IsPrimary)
				return entity;
		}
		return {};
	}

	fbentt::entity Scene::CreateEntityWithParent(fbentt::entity parent)
	{
		if (parent == fbentt::null)
			parent = m_WorldEntity;

		const auto entity = m_Registry->create();

		// Handle relations to make `entity` a child of `parent`
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

		return entity;
	}

	fbentt::entity Scene::CreateEntityWithTagAndParent(const std::string& tag, fbentt::entity parent)
	{
		const fbentt::entity entity = CreateEntityWithParent(parent);
		m_Registry->emplace<IDComponent>(entity);
		m_Registry->emplace<TagComponent>(entity, tag);

		return entity;
	}

	fbentt::entity Scene::CreateEntityWithTagTransformAndParent(const std::string& tag, fbentt::entity parent)
	{
		const auto entity = CreateEntityWithTagAndParent(tag, parent);
		m_Registry->emplace<TransformComponent>(entity);

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

		// Whose responsibility should this be? Should the caller ensure to set destParent to atleast WorldEntity?
		if (destParent == fbentt::null)
			destParent = m_WorldEntity;

		// Cannot reparent if entity and parent are the same,
		// or entity is WorldEntity which is a fixed entity that can't be moved
		if (entity == destParent || entity == m_WorldEntity)
			return;

		// Cannot reparent if hierarchy is something like this
		// entity-001
		//		entity-002
		//		destParent
		//		...
		if (IsEntityInHierarchy(destParent, entity))
			return;

		// TODO: Shouldn't all entities have RelationshipComponent? As they all are children of WorldEntity
		if (!m_Registry->has<RelationshipComponent>(entity))
			m_Registry->emplace<RelationshipComponent>(entity);

		// TODO: Shouldn't all entities have RelationshipComponent? As they all are children of WorldEntity
		if (!m_Registry->has<RelationshipComponent>(destParent))
			m_Registry->emplace<RelationshipComponent>(destParent);

		auto& relation = m_Registry->get<RelationshipComponent>(entity);
		const auto oldParent = relation.Parent;

		// No need to proceed further if `entity` is already a child of `destParent`
		if (oldParent == destParent)
			return;

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

	template <typename... Component>
	static void CopyComponentIfExists(Scene* context, fbentt::entity dest, fbentt::entity src)
	{
		([&]()
			{
				if (auto* comp = context->GetRegistry()->try_get<Component>(src); comp)
					context->GetRegistry()->emplace<Component>(dest, *comp);
			}(),
			...);
	}

	template <typename... Component>
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
		return DuplicatePureEntity(src);
	}

	fbentt::entity Scene::DuplicatePureEntity(fbentt::entity src)
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

		const auto destEntity = DuplicatePureEntity(src);
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

} // namespace Flameberry
