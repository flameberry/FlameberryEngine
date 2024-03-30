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

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void TransformComponent_GetRotation(ulong entityID, out Vector3 rotation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void TransformComponent_SetRotation(ulong entityID, ref Vector3 rotation);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void TransformComponent_GetScale(ulong entityID, out Vector3 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void TransformComponent_SetScale(ulong entityID, ref Vector3 scale);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_GetIsPrimary(ulong entityID, out bool isPrimary);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_SetIsPrimary(ulong entityID, ref bool isPrimary);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_GetProjectionType(ulong entityID, out ProjectionType projectionType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_SetProjectionType(ulong entityID, ref ProjectionType projectionType);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_GetFOVOrZoom(ulong entityID, out float projectionType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_SetFOVOrZoom(ulong entityID, ref float projectionType);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_GetNear(ulong entityID, out float near);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_SetNear(ulong entityID, ref float near);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_GetFar(ulong entityID, out float far);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void CameraComponent_SetFar(ulong entityID, ref float far);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void SkyLightComponent_GetColor(ulong entityID, out Vector3 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void SkyLightComponent_SetColor(ulong entityID, ref Vector3 color);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void SkyLightComponent_GetIntensity(ulong entityID, out float intensity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void SkyLightComponent_SetIntensity(ulong entityID, ref float intensity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void SkyLightComponent_GetEnableSkyMap(ulong entityID, out bool enableSkyMap);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void SkyLightComponent_SetEnableSkyMap(ulong entityID, ref bool enableSkyMap);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void SkyLightComponent_GetEnableReflections(ulong entityID, out bool enableReflections);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void SkyLightComponent_SetEnableReflections(ulong entityID, ref bool enableReflections);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void DirectionalLightComponent_GetColor(ulong entityID, out Vector3 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void DirectionalLightComponent_SetColor(ulong entityID, ref Vector3 color);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void DirectionalLightComponent_GetIntensity(ulong entityID, out float intensity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void DirectionalLightComponent_SetIntensity(ulong entityID, ref float intensity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void DirectionalLightComponent_GetLightSize(ulong entityID, out float lightSize);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void DirectionalLightComponent_SetLightSize(ulong entityID, ref float lightSize);
    }
}

