#version 450

layout (location = 0) out int o_EntityID;

layout (push_constant) uniform MousePickingPushConstant {
    mat4 u_ModelMatrix;
    int u_EntityID;
};

void main()
{
    o_EntityID = u_EntityID;
}