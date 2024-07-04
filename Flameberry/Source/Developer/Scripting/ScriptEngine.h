#pragma once

#include <string>

#include "ECS/Scene.h"

extern "C"
{
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoClassField MonoClassField;
	typedef struct _MonoString MonoString;
}

namespace Flameberry {

	enum class ScriptFieldType
	{
		Invalid = 0,
		Char,
		Byte,
		Short,
		Int,
		Long,
		UByte,
		UShort,
		UInt,
		ULong,
		Float,
		Double,
		Boolean,
		Vector2,
		Vector3,
		Vector4,
		Actor
	};

	struct ScriptField
	{
		ScriptFieldType Type;
		std::string Name;
		MonoClassField* ClassField;

		ScriptField(const ScriptFieldType& type, const std::string& name,
			MonoClassField* field)
			: Type(type), Name(name), ClassField(field) {}
	};

	struct ScriptFieldBuffer
	{
		template <typename T>
		T GetValue() const
		{
			static_assert(sizeof(T) <= 16, "Type size too large");
			return *(T*)m_Buffer;
		}

		template <typename T>
		void SetValue(const T& value)
		{
			static_assert(sizeof(T) <= 16, "Type size too large");
			memcpy(m_Buffer, &value, sizeof(T));
		}

		const uint8_t* GetBuffer() const { return m_Buffer; }

	private:
		// Editor-Specific buffer
		uint8_t
			m_Buffer[16]; // 16 is the max a field can be currently, i.e., a Vector4
	};

	// Map of the field index to the corresponding editor-specific buffer
	using ScriptFieldBufferMap = std::unordered_map<uint32_t, ScriptFieldBuffer>;

	class ManagedClass
	{
	public:
		ManagedClass(MonoImage* assemblyImage, const char* namespaceName,
			const char* className);

		MonoObject* CreateInstance();
		MonoMethod* GetClassMethod(const char* methodName, int paramCount);
		MonoMethod* TryGetClassMethod(const char* methodName, int paramCount);

		std::string GetFullName() const
		{
			return fmt::format("{}.{}", m_NamespaceName, m_ClassName);
		}
		const std::vector<ScriptField>& GetScriptFields() const
		{
			return m_ScriptFields;
		}

	private:
		const char* m_NamespaceName;
		const char* m_ClassName;

		std::vector<ScriptField> m_ScriptFields;

		MonoClass* m_Class;
		MonoImage* m_AssemblyImage;
	};

	class ManagedActor
	{
	public:
		ManagedActor(const Ref<ManagedClass>& managedClass, fbentt::entity entity);
		~ManagedActor();

		Ref<ManagedClass> GetManagedClass() const { return m_ManagedClass; }

		void CallOnCreateMethod();
		void CallOnUpdateMethod(float delta);
		void CallOnDestroyMethod();

		template <typename T>
		T GetFieldValue(const ScriptField& scriptField)
		{
			static_assert(sizeof(T) <= 16, "Type size too large");
			bool success = GetFieldValueInternal(scriptField, s_ScriptFieldBuffer);
			if (!success)
				return T();
			return *(T*)s_ScriptFieldBuffer;
		}

		template <typename T>
		void SetFieldValue(const ScriptField& scriptField, const T& value)
		{
			static_assert(sizeof(T) <= 16, "Type size too large");
			SetFieldValueInternal(scriptField, (void*)&value);
		}

	private:
		void SetFieldBuffer(const ScriptField& scriptField, void* buffer);
		bool GetFieldValueInternal(const ScriptField& scriptField, void* buffer);
		void SetFieldValueInternal(const ScriptField& scriptField, void* value);

	private:
		MonoMethod* m_Constructor = nullptr;
		MonoMethod* m_OnCreateMethod = nullptr;
		MonoMethod* m_OnUpdateMethod = nullptr;
		MonoMethod* m_OnDestroyMethod = nullptr;

		MonoObject* m_Instance;
		Ref<ManagedClass> m_ManagedClass;

		inline static uint8_t s_ScriptFieldBuffer[16];

		friend class ScriptEngine;
	};

	struct ScriptEngineData;

	class ScriptEngine
	{
	public:
		static void Init(const std::filesystem::path& appAssemblyPath);
		static void Shutdown();

		static void ReloadAppAssembly();
		static void LoadAppAssembly(const std::string& assemblyPath);
		static const std::unordered_map<std::string, Ref<ManagedClass>>&
		GetActorClassDictionary();

		static void OnRuntimeStart(const Scene* scene);
		static void OnRuntimeStop();
		static void OnRuntimeUpdate(float delta);

		static Ref<ManagedActor> GetManagedActor(fbentt::entity entity);
		static std::unordered_map<fbentt::entity::handle_type, ScriptFieldBufferMap>&
		GetLocalScriptFieldBufferMap();

	private:
		static void InitMono();
		static void LoadCoreAssembly();
		static void LoadAssembliesAndSetup();
		static void RegisterAllComponents();
	};

} // namespace Flameberry
