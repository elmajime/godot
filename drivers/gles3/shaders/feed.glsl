/* clang-format off */
#version 300 es

#[modes]

mode_default = #define MODE_SIMPLE_COPY

#[specializations]

USE_EXTERNAL_SAMPLER = false
COPY_DEPTHMAP = false

#[vertex]

layout(location = 0) in vec2 vertex_attrib;

out vec2 uv_interp;
/* clang-format on */

void main() {
	uv_interp = vertex_attrib * 0.5 + 0.5;
	gl_Position = vec4(vertex_attrib, 1.0, 1.0);
}

/* clang-format off */
#[fragment]

in vec2 uv_interp;
/* clang-format on */
#ifdef USE_EXTERNAL_SAMPLER
uniform samplerExternalOES source; // texunit:0
#else
uniform sampler2D source; // texunit:0
#endif

layout(location = 0) out vec4 frag_color;

#ifdef COPY_DEPTHMAP
uniform highp float mid_depth_meters;
uniform highp float max_depth_meters;

// const float mid_depth_meters = 2.0; // was 8.0 in the example
// const float max_depth_meters = 65.0; // was 30.0 in the example

#define CLAMP01(T) clamp((T), 0.0, 1.0)
#define EASE(T) smoothstep(0.0, 1.0, (T))

vec3 colorRGB(in int Hex)
{
    // 0xABCDEF
    int AB = (Hex & 0x00FF0000) >> 16;
    int CD = (Hex & 0x0000FF00) >> 8;
    int EF = Hex & 0x000000FF;
    return pow(vec3(AB, CD, EF)/255.0, vec3(2.2));
}

vec3 catmulRom(in float T, vec3 D, vec3 C, vec3 B, vec3 A)
{
    return 0.5 * ((2.0 * B) + (-A + C) * T + (2.0 * A - 5.0 * B + 4.0 * C - D) * T*T + (-A + 3.0 * B - 3.0 * C + D) *T*T*T);
}

vec3 colorRamp_BSpline(in float T, vec4 A, in vec4 B, in vec4 C, in vec4 D)
{
    // Distances = 
    float AB = B.w-A.w;
    float BC = C.w-B.w;
    float CD = D.w-C.w;
 
    // Intervales :
    float iAB = CLAMP01((T-A.w)/AB);
    float iBC = CLAMP01((T-B.w)/BC);
    float iCD = CLAMP01((T-C.w)/CD);
    
    // Pondérations :
    vec4 p = vec4(1.0-iAB, iAB-iBC, iBC-iCD, iCD);
    vec3 cA = catmulRom(p.x, A.xyz, A.xyz, B.xyz, C.xyz);
    vec3 cB = catmulRom(p.y, A.xyz, B.xyz, C.xyz, D.xyz);
    vec3 cC = catmulRom(p.z, B.xyz, C.xyz, D.xyz, D.xyz);
    vec3 cD = D.xyz;

    if(T < B.w) return cA.xyz;
    if(T < C.w) return cB.xyz;
    if(T < D.w) return cC.xyz;
    return cD.xyz;
}

float depthInMillimeters(in sampler2D depthTexture, in vec2 depthUV) {
	// Depth is packed into the red and green components of its texture.
	// The texture is a normalized format, storing millimeters.
	vec2 packedDepthAndVisibility = texture(depthTexture, depthUV).xy;
	return dot(packedDepthAndVisibility.xy, vec2(255.0, 256.0 * 255.0));
}

float inverseLerp(float value, float minBound, float maxBound) {
	return clamp((value - minBound) / (maxBound - minBound), 0.0, 1.0);
}
#endif //COPY_DEPTHMAP

void main() {
#ifdef MODE_SIMPLE_COPY
#ifdef COPY_DEPTHMAP
	// Colors from DepthMap
	float depth_mm = depthInMillimeters(source, uv_interp);
	float depth_meters = depth_mm * 0.001;

	float normalized_depth = inverseLerp(depth_meters, 0.0, mid_depth_meters);
	// float normalized_depth = 0.0;
	// if (depth_meters < mid_depth_meters) {
	// 	// Short-range depth (0m to 8m) maps to first half of the color palette;
	// 	normalized_depth = inverseLerp(depth_meters, 0.0, mid_depth_meters) * 0.5;
	// } else {
	// 	// Long-range depth (8m to 30m) maps to second half of the color palette.
	// 	normalized_depth = inverseLerp(depth_meters, mid_depth_meters, max_depth_meters) * 0.5 + 0.5;
	// }

	vec4 A = vec4(colorRGB(0x6a2c70), 0.05);
    vec4 B = vec4(colorRGB(0xb83b5e), 0.22);
    vec4 C = vec4(colorRGB(0xf08a5d), 0.5);
    vec4 D = vec4(colorRGB(0xf9ed69), 0.9);

	vec4 depth_color = vec4(colorRamp_BSpline(normalized_depth, A,B,C,D), 1.0);

	// vec4 depth_color = vec4(uv_interp.x, uv_interp.y, 0.0, 1.0);
	// vec4 depth_color = vec4(depthAsColor(normalized_depth), 1.0);

	// Invalid depth (pixels with value 0) mapped to black.
    depth_color.rgb *= sign(depth_meters);
	frag_color = depth_color;   

#else
	// Background camera feed
	vec4 color = texture(source, uv_interp);
	frag_color = color;
#endif // COPY_DEPTHMAP
#endif
}
