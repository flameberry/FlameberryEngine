using System;
using System.Runtime.CompilerServices;
namespace Flameberry.Managed
{
    public class InternalCalls
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern static void Entity_GetTranslation();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern static void Entity_SetTranslation();
    }
}

