using System;
using System.Numerics;
using System.Transactions;
using Flameberry.Managed;

namespace Flameberry
{
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
                InternalCalls.TransformComponent_GetTranslation(Actor.ID, out Vector3 translation);
                return translation;
            }
            set
            {
                InternalCalls.TransformComponent_SetTranslation(Actor.ID, ref value);
            }
        }
    }
}