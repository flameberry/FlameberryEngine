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

        public Vector3 Rotation
        {
            get
            {
                InternalCalls.TransformComponent_GetRotation(Actor.ID, out Vector3 rotation);
                return rotation;
            }
            set
            {
                InternalCalls.TransformComponent_SetRotation(Actor.ID, ref value);
            }
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.TransformComponent_GetScale(Actor.ID, out Vector3 scale);
                return scale;
            }
            set
            {
                InternalCalls.TransformComponent_SetScale(Actor.ID, ref value);
            }
        }
    }

    public enum ProjectionType : byte { Orthographic = 0, Perspective };

    public class CameraComponent : Component
    {
        public ProjectionType ProjectionType
        {
            get
            {
                InternalCalls.CameraComponent_GetProjectionType(Actor.ID, out ProjectionType projectionType);
                return projectionType;
            }
            set
            {
                InternalCalls.CameraComponent_SetProjectionType(Actor.ID, ref value);
            }
        }
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