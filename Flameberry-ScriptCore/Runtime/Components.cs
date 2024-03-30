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
        public Vector3 Color
        {
            get
            {
                InternalCalls.SkyLightComponent_GetColor(Actor.ID, out Vector3 color);
                return color;
            }
            set
            {
                InternalCalls.SkyLightComponent_SetColor(Actor.ID, ref value);
            }
        }

        public float Intensity
        {
            get
            {
                InternalCalls.SkyLightComponent_GetIntensity(Actor.ID, out float intensity);
                return intensity;
            }
            set
            {
                InternalCalls.SkyLightComponent_SetIntensity(Actor.ID, ref value);
            }
        }

        public bool EnableSkyMap
        {
            get
            {
                InternalCalls.SkyLightComponent_GetEnableSkyMap(Actor.ID, out bool enableSkyMap);
                return enableSkyMap;
            }
            set
            {
                InternalCalls.SkyLightComponent_SetEnableSkyMap(Actor.ID, ref value);
            }
        }

        public bool EnableReflections
        {
            get
            {
                InternalCalls.SkyLightComponent_GetEnableReflections(Actor.ID, out bool enableReflections);
                return enableReflections;
            }
            set
            {
                InternalCalls.SkyLightComponent_SetEnableReflections(Actor.ID, ref value);
            }
        }
    }

    public class MeshComponent : Component
    {
    }

    public class DirectionalLightComponent : Component
    {
        public Vector3 Color
        {
            get
            {
                InternalCalls.DirectionalLightComponent_GetColor(Actor.ID, out Vector3 color);
                return color;
            }
            set
            {
                InternalCalls.DirectionalLightComponent_SetColor(Actor.ID, ref value);
            }
        }

        public float Intensity
        {
            get
            {
                InternalCalls.DirectionalLightComponent_GetIntensity(Actor.ID, out float intensity);
                return intensity;
            }
            set
            {
                InternalCalls.DirectionalLightComponent_SetIntensity(Actor.ID, ref value);
            }
        }

        public float LightSize
        {
            get
            {
                InternalCalls.DirectionalLightComponent_GetLightSize(Actor.ID, out float lightSize);
                return lightSize;
            }
            set
            {
                InternalCalls.DirectionalLightComponent_SetLightSize(Actor.ID, ref value);
            }
        }
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