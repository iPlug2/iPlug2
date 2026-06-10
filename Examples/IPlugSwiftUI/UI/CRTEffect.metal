#include <metal_stdlib>
#include <SwiftUI/SwiftUI.h>
using namespace metal;

// https://gist.github.com/Priva28/c4becef12fd8dd399cc769f2c7a5c246

float2 distort(float2 uv, float strength) {
    float2 dist = 0.5 - uv;
    uv.x = (uv.x - dist.y * dist.y * dist.x * strength);
    uv.y = (uv.y - dist.x * dist.x * dist.y * strength);
    return uv;
}

[[stitchable]]
half4 crteffect(float2 position, SwiftUI::Layer layer, float time, float2 size) {
    float2 uv = position / size;

    uv = distort(uv, 0.3);
    
    // render color
    half4 col;
    col.r = layer.sample(float2(uv.x, uv.y) * size).r;
    col.g = layer.sample(float2(uv.x, uv.y) * size).g;
    col.b = layer.sample(float2(uv.x, uv.y) * size).b;
    col.a = layer.sample(float2(uv.x, uv.y) * size).a;
    
    // brighten image
    col *= half4(0.95, 1.05, 0.95, 1);
    col *= 2.8;

    // add scan lines
    float scans = clamp(0.35 + 0.35 * sin(3.5 * time + uv.y * size.y * 1.5), 0.0, 1.0);
    float s = pow(scans, 1.7);
    float sn = 0.4 + 0.7 * s;
    col = col * half4(sn, sn, sn, 1);

    // normalise color
    col *= 1.0 + 0.01 * sin(110.0 * time);
    float c = clamp((fmod(position.x, 2.0) - 1.0) * 2.0, 0.0, 1.0);
    col *= 1.0 - 0.65 * half4(c, c, c, 1);

    return col;
}
