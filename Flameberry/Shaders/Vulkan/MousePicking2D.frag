#version 450

layout(location = 0) out int o_EntityIndex;
layout(location = 0) in flat int v_EntityIndex;

void main()
{
    o_EntityIndex = v_EntityIndex;
}
