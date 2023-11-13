using System;
using System.Reflection;
using System.Runtime.InteropServices;

using Flameberry.Runtime;

namespace Flameberry.Managed
{
	public static class ManagedActors
	{
		internal static Dictionary<ulong, Actor> s_ManagedActors = new();

		[UnmanagedCallersOnly]
		internal static void CreateActorWithEntityID(ulong ID, IntPtr className)
		{
			if (s_ManagedActors.ContainsKey(ID))
			{ 
				Console.WriteLine($"ERROR: A managed actor with ID: {ID} already exists!");
				return;
			}

            string? classNameStr = Marshal.PtrToStringAuto(className);
			if (classNameStr == null)
			{
				Console.WriteLine("ERROR: Cannot create Actor with null SubClass Name!");
				return;
			}

			Console.WriteLine($"INFO: Called CreateActorWithEntityID with Arguments: {ID}, {classNameStr}");

			Type? type = AppAssemblyManager.FindType(classNameStr);

			if (type == null)
			{
				Console.WriteLine($"ERROR: Failed to find Actor SubClass: {classNameStr}");
                return;
			}

			// Check if `type` is subclass of `Actor`
			if (IsSubclassOf(type, typeof(Actor)))
			{
				object? instance = Activator.CreateInstance(type);
				s_ManagedActors[ID] = (Actor)instance!;
			}
			else
			{
				Console.WriteLine($"ERROR: the following type: {type.FullName}, has a BaseType of {type.BaseType.FullName} is not a subclass of Actor!");
			}
		}

		[UnmanagedCallersOnly]
		internal static void DestroyAllActors()
		{
			s_ManagedActors.Clear();
		}

		[UnmanagedCallersOnly]
		internal static void DestroyActorWithID(ulong ID)
		{
		}

        [UnmanagedCallersOnly]
        internal static void InvokeOnCreateMethodOfActorWithID(ulong ID)
        {
            if (s_ManagedActors.TryGetValue(ID, out var actor))
            {
				actor.OnCreate();
                return;
            }
            Console.WriteLine($"ERROR: Failed to invoke `OnCreate()` method of Actor with ID: {ID}: Actor doesn't exist!");
        }

        [UnmanagedCallersOnly]
        internal static void InvokeOnUpdateMethodOfActorWithID(ulong ID, float param)
        {
            if (s_ManagedActors.TryGetValue(ID, out var actor))
            {
				actor.OnUpdate(param);
				return;
            }
			Console.WriteLine($"ERROR: Failed to invoke `OnUpdate()` method of Actor with ID: {ID}: Actor doesn't exist!");
        }

        // Utils
        internal static bool IsSubclassOf(Type childType, Type parentType)
        {
            return childType.IsSubclassOf(parentType) ||
				   childType.GetInterfaces().Any(i => i == parentType) ||
				   childType == parentType;
        }

    }
}

