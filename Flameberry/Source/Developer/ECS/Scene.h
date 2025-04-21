#pragma once

#include "ecs.hpp"
#include "Renderer/StaticMesh.h"
#include "Asset/Asset.h"

namespace Flameberry {

	class Scene : public Asset
	{
	public:
		Scene();
		~Scene();

		explicit Scene(const Ref<Scene>& other);
		explicit Scene(const Scene& other);
		const Scene& operator=(const Scene& other) const = delete;

		void OnStartRuntime();
		void OnUpdateRuntime(float delta);
		void OnStopRuntime();

		void OnStartSimulation();
		void OnUpdateSimulation(float delta);
		void OnStopSimulation();

		void OnViewportResize(const glm::vec2& viewportSize);

		FEntity CreateEntityWithTagAndParent(const std::string& tag, FEntity parent);
		FEntity CreateEntityWithTagTransformAndParent(const std::string& tag, FEntity parent);
		void DestroyEntityTree(FEntity entity);
		void ReparentEntity(FEntity entity, FEntity destParent);
		bool IsEntityInHierarchy(FEntity key, FEntity parent);
		FEntity DuplicateEntity(FEntity src);
		FEntity DuplicatePureEntity(FEntity src);
		FEntity DuplicateEntityTree(FEntity src);

		bool IsRuntimeActive() const { return m_IsRuntimeActive; }
		bool IsRuntimePaused() const { return m_IsRuntimePaused; }

		void SetRuntimePaused(bool value) { m_IsRuntimePaused = value; }
		void Step(int steps) { m_StepFrames = steps; }

		inline std::string GetName() const { return m_Name; }
		inline Ref<FRegistry> GetRegistry() const { return m_Registry; }
		inline FEntity GetWorldEntity() const { return m_WorldEntity; }
		inline bool IsWorldEntity(FEntity entity) const { return m_WorldEntity == entity; }
		FEntity GetPrimaryCameraEntity() const;

		FBY_DECLARE_ASSET_TYPE(AssetType::Scene);

	private:
		/**
		 * Creates an entity as a child of the given parent
		 * If parent is null, entity will be created with the world entity being it's parent
		 * This is the only way an entity should be created in the scene
		 */
		FEntity CreateEntityWithParent(FEntity parent);

		/**
		 * Recursively checks if the given `entity` is in the lower heirarchy of the given `parent`
		 */
		bool Recursive_IsEntityInHierarchy(FEntity key, FEntity parent);

		void OnPhysicsStart();
		void OnPhysicsSimulate(float delta);
		void OnPhysicsStop();

		/**
		 * Handle Pausing and Stepping
		 */
		bool ShouldStep();

	private:
		Ref<FRegistry> m_Registry;
		FEntity m_WorldEntity = {};

		std::string m_Name = "Untitled";
		glm::vec2 m_ViewportSize = { 1280, 720 };

		bool m_IsRuntimeActive = false, m_IsRuntimePaused = false;
		int m_StepFrames = 0;

		// friend class SceneHierarchyPanel;
		// friend class InspectorPanel;
		friend class SceneSerializer;
		// friend class SceneRenderer;
	};

} // namespace Flameberry
