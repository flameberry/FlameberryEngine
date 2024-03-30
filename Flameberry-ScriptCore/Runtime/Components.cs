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

    public class RigidBodyComponent : Component
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