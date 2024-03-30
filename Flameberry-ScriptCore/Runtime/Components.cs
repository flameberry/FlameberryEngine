using System.Numerics;
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
        public bool IsPrimary
        {
            get
            {
                InternalCalls.CameraComponent_GetIsPrimary(Actor.ID, out bool isPrimary);
                return isPrimary;
            }
            set
            {
                InternalCalls.CameraComponent_SetIsPrimary(Actor.ID, ref value);
            }
        }

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

        public float FOVOrZoom
        {
            get
            {
                InternalCalls.CameraComponent_GetFOVOrZoom(Actor.ID, out float fovOrZoom);
                return fovOrZoom;
            }
            set
            {
                InternalCalls.CameraComponent_SetFOVOrZoom(Actor.ID, ref value);
            }
        }

        public float Near
        {
            get
            {
                InternalCalls.CameraComponent_GetNear(Actor.ID, out float near);
                return near;
            }
            set
            {
                InternalCalls.CameraComponent_SetNear(Actor.ID, ref value);
            }
        }


        public float Far
        {
            get
            {
                InternalCalls.CameraComponent_GetFar(Actor.ID, out float far);
                return far;
            }
            set
            {
                InternalCalls.CameraComponent_SetFar(Actor.ID, ref value);
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