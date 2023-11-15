using System;
namespace Flameberry.Managed
{
	public static class Utils
	{
        internal static bool IsSubclassOf(Type childType, Type parentType)
        {
            return childType.IsSubclassOf(parentType) ||
                   childType.GetInterfaces().Any(i => i == parentType) ||
                   childType == parentType;
        }
    }
}

