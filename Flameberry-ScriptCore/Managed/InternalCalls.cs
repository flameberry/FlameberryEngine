using System.Numerics;
using System.Runtime.CompilerServices;
namespace Flameberry.Managed
{
    public class InternalCalls
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static bool Entity_HasComponent(ulong entityID, Type componentType);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void TransformComponent_GetTranslation(ulong entityID, out Vector3 translation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void TransformComponent_SetTranslation(ulong entityID, ref Vector3 translation);
    }
}

