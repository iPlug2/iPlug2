#include <metal_stdlib>

using namespace metal;

/// - Parameter position: The user-space coordinate of the current pixel.
/// - Parameter color: The current color of the pixel.
/// - Parameter size: The size of the whole image, in user-space.
/// - Parameter waveform: A pointer to an array of float values representing the waveform.
/// - Parameter count: The number of elements in the waveform array.
[[ stitchable ]] half4 oscilloscope(float2 position, half4 color, float2 size, device const float *waveform, int count) {
    // Normalize the pixel coordinate to the [0, 1] range
    float2 uv = position / size;
    
    // Background color (black)
    half4 bgColor = half4(0.0, 0.0, 0.0, 1.0);

    // If there are fewer than 2 samples, nothing to draw.
    if(count < 2) {
        return bgColor;
    }

    // Find the minimum distance from the pixel (uv) to any segment of the waveform.
    float minDist = 1.0;
    for (int i = 0; i < count - 1; i++) {
        float x0 = float(i) / float(count - 1);
        float x1 = float(i + 1) / float(count - 1);

        // Map waveform sample values from [-1, 1] to [0, 1].
        float y0 = waveform[i] * 0.5 + 0.5;
        float y1 = waveform[i+1] * 0.5 + 0.5;

        float2 a = float2(x0, y0);
        float2 b = float2(x1, y1);
        float2 v = b - a;
        float2 w = uv - a;

        // Compute projection factor and clamp to segment.
        float t = clamp(dot(w, v) / max(dot(v, v), 1e-5), 0.0, 1.0);
        float2 proj = a + t * v;

        // Compute distance from current pixel to the segment.
        float d = length(uv - proj);
        minDist = min(minDist, d);
    }

    // Standard deviation (sigma) values for the core and the glow.
    const float coreSigma = 0.005;   // Core line (narrow falloff)
    const float glowSigma = 0.002;    // Outer glow (wider falloff)

    // Calculate Gaussian intensities for core and glow.
    float coreIntensity = exp(- (minDist * minDist) / (2.0 * coreSigma * coreSigma));
    float glowIntensity = exp(- (minDist * minDist) / (2.0 * glowSigma * glowSigma));

    // Combine the two effects (with a slight boost for the glow).
    float combinedIntensity = saturate(coreIntensity + 0.5 * glowIntensity);

    // Introduce a subtle sine-based modulation for dynamic pulsation.
    float mod = sin(uv.x * 15.0 + 6.2831 * uv.y) * 0.1 + 0.9;
    float finalIntensity = saturate(combinedIntensity * mod);

    // Blend between core green and a cyan-green tone for the glow.
    half4 coreColor = half4(0.0, 1.0, 0.0, 1.0);
    half4 edgeColor = half4(0.0, 1.0, 0.5, 1.0);
    
    // Determine mix factor based on the distance using smoothstep.
    float mixFactor = smoothstep(coreSigma, glowSigma, minDist);
    half4 finalColor = mix(coreColor, edgeColor, mixFactor);

    return finalColor * finalIntensity;
}
