#version 450

layout (location = 0) out int o_EntityIndex;

layout (push_constant) uniform MousePickingPushConstant {
    mat4 u_ModelMatrix;
    int u_EntityIndex;
};

void main()
{
    o_EntityIndex = u_EntityIndex;
}