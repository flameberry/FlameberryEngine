#version 450

const float g_Near = 0.01;
const float g_Far = 100.0;

layout(location = 0) in vec3 v_NearPoint;
layout(location = 1) in vec3 v_FarPoint;
layout(location = 2) in mat4 v_ViewMatrix;
layout(location = 6) in mat4 v_ProjectionMatrix;

layout(location = 0) out vec4 o_FragColor;

float ComputeDepth(vec3 pos)
{
    vec4 clipSpacePos = v_ProjectionMatrix * v_ViewMatrix * vec4(pos.xyz, 1.0);
    return (clipSpacePos.z / clipSpacePos.w);
}

float ComputeLinearDepth(vec3 pos)
{
    vec4 clipSpacePos = v_ProjectionMatrix * v_ViewMatrix * vec4(pos.xyz, 1.0);
    float clipSpaceDepth = (clipSpacePos.z / clipSpacePos.w) * 2.0 - 1.0; // put back between -1 and 1
    float linearDepth = (2.0 * g_Near * g_Far) / (g_Far + g_Near - clipSpaceDepth * (g_Far - g_Near)); // get linear value between 0.01 and 100
    return linearDepth / g_Far; // normalize
}

float PristineGrid(in vec2 uv, vec2 lineWidth)
{
    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);
    vec2 uvDeriv = vec2(length(vec2(ddx.x, ddy.x)), length(vec2(ddx.y, ddy.y)));
    bvec2 invertLine = bvec2(lineWidth.x > 0.5, lineWidth.y > 0.5);
    vec2 targetWidth = vec2(
            invertLine.x ? 1.0 - lineWidth.x : lineWidth.x,
            invertLine.y ? 1.0 - lineWidth.y : lineWidth.y
        );
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * 1.5;
    vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
    gridUV.x = invertLine.x ? gridUV.x : 1.0 - gridUV.x;
    gridUV.y = invertLine.y ? 1.0 - gridUV.y : gridUV.y;
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);

    grid2 *= clamp(targetWidth / drawWidth, 0.0, 1.0);
    grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));
    grid2.x = invertLine.x ? 1.0 - grid2.x : grid2.x;
    grid2.y = invertLine.y ? 1.0 - grid2.y : grid2.y;
    return mix(grid2.x, 1.0, grid2.y);
}

void main()
{
    float t = -v_NearPoint.y / (v_FarPoint.y - v_NearPoint.y);
    vec3 fragPos3D = v_NearPoint + t * (v_FarPoint - v_NearPoint);

    gl_FragDepth = ComputeDepth(fragPos3D);

    float linearDepth = ComputeLinearDepth(fragPos3D);
    float fading = max(0, (0.5 - linearDepth));

    // Compute UV coordinates for the grid
    vec2 gridUV = fragPos3D.xz;

    vec2 lineWidth = vec2(0.02);
    float gridIntensity = PristineGrid(gridUV * 0.5, lineWidth);
    // gridIntensity += PristineGrid(gridUV * 5.0, vec2(0.1));

    // Determine the color of the fragment based on grid intensity
    vec4 gridColor = vec4(vec3(0.4), gridIntensity);
    // vec4 gridColor = mix(vec4(0, 0, 0, 1), vec4(1, 1, 1, 1), gridIntensity);

    o_FragColor = gridColor * float(t > 0);
    o_FragColor.a *= fading;
}
