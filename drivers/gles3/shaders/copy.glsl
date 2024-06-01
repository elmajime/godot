/* clang-format off */
#version 300 es

#[modes]

mode_default = #define MODE_SIMPLE_COPY
mode_copy_section = #define USE_COPY_SECTION \n#define MODE_SIMPLE_COPY
mode_copy_section_source = #define USE_COPY_SECTION \n#define MODE_SIMPLE_COPY \n#define MODE_COPY_FROM
mode_gaussian_blur = #define MODE_GAUSSIAN_BLUR
mode_mipmap = #define MODE_MIPMAP
mode_simple_color = #define MODE_SIMPLE_COLOR \n#define USE_COPY_SECTION
mode_cube_to_octahedral = #define CUBE_TO_OCTAHEDRAL \n#define USE_COPY_SECTION

#[specializations]

USE_EXTERNAL_SAMPLER = false
COPY_DEPTHMAP = false

#[vertex]

layout(location = 0) in vec2 vertex_attrib;

out vec2 uv_interp;
/* clang-format on */

#if defined(USE_COPY_SECTION) || defined(MODE_GAUSSIAN_BLUR)
// Defined in 0-1 coords.
uniform highp vec4 copy_section;
#endif
#if defined(MODE_GAUSSIAN_BLUR) || defined(MODE_COPY_FROM)
uniform highp vec4 source_section;
#endif

void main() {
	uv_interp = vertex_attrib * 0.5 + 0.5;
	gl_Position = vec4(vertex_attrib, 1.0, 1.0);

#if defined(USE_COPY_SECTION) || defined(MODE_GAUSSIAN_BLUR)
	gl_Position.xy = (copy_section.xy + uv_interp.xy * copy_section.zw) * 2.0 - 1.0;
#endif
#if defined(MODE_GAUSSIAN_BLUR) || defined(MODE_COPY_FROM)
	uv_interp = source_section.xy + uv_interp * source_section.zw;
#endif
}

/* clang-format off */
#[fragment]

in vec2 uv_interp;
/* clang-format on */
#ifdef MODE_SIMPLE_COLOR
uniform vec4 color_in;
#endif

#ifdef MODE_GAUSSIAN_BLUR
// Defined in 0-1 coords.
uniform highp vec2 pixel_size;
#endif

#ifdef CUBE_TO_OCTAHEDRAL
uniform samplerCube source_cube; // texunit:0

vec3 oct_to_vec3(vec2 e) {
	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	float t = max(-v.z, 0.0);
	v.xy += t * -sign(v.xy);
	return normalize(v);
}
#else 
#ifdef USE_EXTERNAL_SAMPLER
uniform samplerExternalOES source; // texunit:0
#else
uniform sampler2D source; // texunit:0
#endif
#endif

layout(location = 0) out vec4 frag_color;

const float midDepthMeters = 2.0; // was 8.0 in the example
const float maxDepthMeters = 65.0; // was 30.0 in the example

#define CLAMP01(T) clamp((T), 0.0, 1.0)
#define EASE(T) smoothstep(0.0, 1.0, (T))

vec3 CouleurRVB(in int Hex)
{
    // 0xABCDEF
    int AB = (Hex & 0x00FF0000) >> 16;
    int CD = (Hex & 0x0000FF00) >> 8;
    int EF = Hex & 0x000000FF;
    return pow(vec3(AB, CD, EF)/255.0, vec3(2.2));
}

vec3 CatmulRom(in float T, vec3 D, vec3 C, vec3 B, vec3 A)
{
    return 0.5 * ((2.0 * B) + (-A + C) * T + (2.0 * A - 5.0 * B + 4.0 * C - D) * T*T + (-A + 3.0 * B - 3.0 * C + D) *T*T*T);
}

vec3 ColorRamp_BSpline(in float T, vec4 A, in vec4 B, in vec4 C, in vec4 D)
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
    vec3 cA = CatmulRom(p.x, A.xyz, A.xyz, B.xyz, C.xyz);
    vec3 cB = CatmulRom(p.y, A.xyz, B.xyz, C.xyz, D.xyz);
    vec3 cC = CatmulRom(p.z, B.xyz, C.xyz, D.xyz, D.xyz);
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

// vec3 depthAsColor(in float x) {
// 	//return vec3(1.0 - x, x, 0.0);
// 	return colorRamp(x);
// }

void main() {
#ifdef MODE_SIMPLE_COPY
#ifdef COPY_DEPTHMAP
	// DepthMap
	float depth_mm = depthInMillimeters(source, uv_interp);
	float depth_meters = depth_mm * 0.001;

	float normalized_depth = inverseLerp(depth_meters, 0.0, midDepthMeters);
	// if (depth_meters < midDepthMeters) {
	// 	// Short-range depth (0m to 8m) maps to first half of the color palette;
	// 	normalized_depth = inverseLerp(depth_meters, 0.0, midDepthMeters) * 0.5;
	// } else {
	// 	// Long-range depth (8m to 30m) maps to second half of the color palette.
	// 	normalized_depth = inverseLerp(depth_meters, midDepthMeters, maxDepthMeters) * 0.5 + 0.5;
	// }

	vec4 A = vec4(CouleurRVB(0x6a2c70), 0.05);
    vec4 B = vec4(CouleurRVB(0xb83b5e), 0.22);
    vec4 C = vec4(CouleurRVB(0xf08a5d), 0.5);
    vec4 D = vec4(CouleurRVB(0xf9ed69), 0.9);

	vec4 depth_color = vec4(ColorRamp_BSpline(normalized_depth, A,B,C,D), 1.0);

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

#ifdef MODE_SIMPLE_COLOR
	frag_color = color_in;
#endif

// Efficient box filter from Jimenez: http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
// Approximates a Gaussian in a single pass.
#ifdef MODE_GAUSSIAN_BLUR
	vec4 A = textureLod(source, uv_interp + pixel_size * vec2(-1.0, -1.0), 0.0);
	vec4 B = textureLod(source, uv_interp + pixel_size * vec2(0.0, -1.0), 0.0);
	vec4 C = textureLod(source, uv_interp + pixel_size * vec2(1.0, -1.0), 0.0);
	vec4 D = textureLod(source, uv_interp + pixel_size * vec2(-0.5, -0.5), 0.0);
	vec4 E = textureLod(source, uv_interp + pixel_size * vec2(0.5, -0.5), 0.0);
	vec4 F = textureLod(source, uv_interp + pixel_size * vec2(-1.0, 0.0), 0.0);
	vec4 G = textureLod(source, uv_interp, 0.0);
	vec4 H = textureLod(source, uv_interp + pixel_size * vec2(1.0, 0.0), 0.0);
	vec4 I = textureLod(source, uv_interp + pixel_size * vec2(-0.5, 0.5), 0.0);
	vec4 J = textureLod(source, uv_interp + pixel_size * vec2(0.5, 0.5), 0.0);
	vec4 K = textureLod(source, uv_interp + pixel_size * vec2(-1.0, 1.0), 0.0);
	vec4 L = textureLod(source, uv_interp + pixel_size * vec2(0.0, 1.0), 0.0);
	vec4 M = textureLod(source, uv_interp + pixel_size * vec2(1.0, 1.0), 0.0);

	float weight = 0.5 / 4.0;
	float lesser_weight = 0.125 / 4.0;

	frag_color = (D + E + I + J) * weight;
	frag_color += (A + B + G + F) * lesser_weight;
	frag_color += (B + C + H + G) * lesser_weight;
	frag_color += (F + G + L + K) * lesser_weight;
	frag_color += (G + H + M + L) * lesser_weight;
#endif

#ifdef CUBE_TO_OCTAHEDRAL
	// Treat the UV coordinates as 0-1 encoded octahedral coordinates.
	vec3 dir = oct_to_vec3(uv_interp * 2.0 - 1.0);
	frag_color = texture(source_cube, dir);

#endif
}
