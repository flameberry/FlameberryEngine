using System.Numerics;
using System.Runtime.InteropServices;
using Flameberry.Managed;

namespace Flameberry.Runtime
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct ComponentInfo
    {
        public IntPtr Name;
        public int HashCode;

        internal ComponentInfo(IntPtr name, int hashcode)
        {
            Name = name;
            HashCode = hashcode;
        }
    }

    internal static class ComponentManager
    {
        [UnmanagedCallersOnly]
        internal static int GetComponentHashCode(IntPtr assemblyQualifiedName)
        {
            string? assemblyQualifiedNameStr = Marshal.PtrToStringAuto(assemblyQualifiedName);
            if (assemblyQualifiedNameStr == null)
            {
                Console.WriteLine("ERROR: Failed to GetComponentHashCode: Assembly Qualified Name is null!");
                return -1;
            }

            Type? componentType = Type.GetType(assemblyQualifiedNameStr);

            if (componentType == null)
            {
                Console.WriteLine($"ERROR: Failed to GetComponentHashCode: Could not find Component Type with name: {assemblyQualifiedNameStr}!");
                return -1;
            }
            return componentType.GetHashCode();
        }
    }

    public abstract class Component
    {
        public Actor Actor { get; internal set; }
    }

    public class TransformComponent : Component
    {
        public Vector3 Translation
        {
            get
            {
                unsafe { return InternalCallStorage.TransformComponent_GetTranslation(Actor.ID); }
            }
            set
            {
                unsafe { InternalCallStorage.TransformComponent_SetTranslation(Actor.ID, (Vector3*)&value); }
            }
        }

    }

    public class CameraComponent : Component
    {
    }

    public class SkyLightComponent : Component
    {
    }

    public class MeshComponent : Component
    {
    }

    public class DirectionalLightComponent : Component
    {
    }

    public class PointLightComponent : Component
    {
    }

    public class NativeScriptComponent : Component
    {
    }

    public class ScriptComponent : Component
    {
    }

    public class RigidBodyComponent: Component
    {
    }

    public class BoxColliderComponent : Component
    {
    }

    public class SphereColliderComponent : Component
    {
    }

    public class CapsuleColliderComponent : Component
    {
    }
}
