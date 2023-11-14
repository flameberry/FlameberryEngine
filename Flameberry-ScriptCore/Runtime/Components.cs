using System;
using System.Runtime.InteropServices;
using static System.Runtime.InteropServices.JavaScript.JSType;

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
        private static List<IntPtr> s_ComponentNameStorage = new();
        private static IntPtr s_ComponentInfoStoragePtr;

        private static Type[] s_ComponentTypes = {
            typeof(TransformComponent),
            typeof(CameraComponent),
            typeof(SkyLightComponent),
            typeof(MeshComponent),
            typeof(DirectionalLightComponent),
            typeof(PointLightComponent),
            typeof(NativeScriptComponent),
            typeof(ScriptComponent),
            typeof(RigidBodyComponent),
            typeof(BoxColliderComponent),
            typeof(SphereColliderComponent),
            typeof(CapsuleColliderComponent)
        };

        [UnmanagedCallersOnly]
        internal static int GetComponentCount()
        {
            return s_ComponentTypes.Length;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetComponentHashes()
        {
            s_ComponentInfoStoragePtr = Marshal.AllocHGlobal(Marshal.SizeOf<ComponentInfo>() * s_ComponentTypes.Length);

            try
            {
                // Copy the array to unmanaged memory
                for (int i = 0; i < s_ComponentTypes.Length; i++)
                {
                    s_ComponentNameStorage.Add(Marshal.StringToCoTaskMemAuto(s_ComponentTypes[i].ToString()));
                    ComponentInfo component = new(s_ComponentNameStorage[i], s_ComponentTypes[i].GetHashCode());

                    Console.WriteLine($"CSharp: HashCode of {s_ComponentTypes[i]} is {s_ComponentTypes[i].GetHashCode()}");

                    IntPtr elementPtr = IntPtr.Add(s_ComponentInfoStoragePtr, i * Marshal.SizeOf<ComponentInfo>());
                    Marshal.StructureToPtr(component, elementPtr, false);
                }
                return s_ComponentInfoStoragePtr;
            }
            catch
            {
                // Free allocated memory in case of an exception
                foreach (IntPtr namePtr in s_ComponentNameStorage)
                {
                    Marshal.FreeCoTaskMem(namePtr);
                }
                s_ComponentNameStorage.Clear();
                Marshal.FreeHGlobal(s_ComponentInfoStoragePtr);
                throw;
            }
        }

        [UnmanagedCallersOnly]
        internal static void FreeComponentStorageMemory()
        {
            foreach (IntPtr namePtr in s_ComponentNameStorage)
            {
                Marshal.FreeCoTaskMem(namePtr);
            }
            s_ComponentNameStorage.Clear();
            Marshal.FreeHGlobal(s_ComponentInfoStoragePtr);
        }
    }

    public abstract class Component
    {
        internal Actor Actor;
    }

    public class TransformComponent : Component
    {
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
