/* clang-format off */
#version 300 es

#[modes]

mode_default = 

#[specializations]

#[vertex]

layout(location = 0) in vec2 vertex_attrib;

out vec2 uv_interp;
out float odepth;
/* clang-format on */

uniform sampler2D source; // texunit:0
uniform int vertical_precision;
uniform int horizontal_precision;
// uniform mat4 inv_view_projection_matrix;

float depthInMillimeters(in sampler2D depthTexture, in vec2 depthUV) {
	// Depth is packed into the red and green components of its texture.
	// The texture is a normalized format, storing millimeters.
	vec2 packedDepthAndVisibility = texture(depthTexture, depthUV).xy;
	return dot(packedDepthAndVisibility.xy, vec2(255.0, 256.0 * 255.0));
}

float inverseLerp(float value, float minBound, float maxBound) {
	return clamp((value - minBound) / (maxBound - minBound), 0.0, 1.0);
}

void main() {
	// uv_interp = vertex_attrib * 0.5 + 0.5;
	// gl_Position = vec4(vertex_attrib, 1.0, 1.0);

    float point_size = 1.0;

    gl_PointSize = point_size;

    int index = gl_VertexID;
    vec2 index2D = vec2(float(index % horizontal_precision), float(index/horizontal_precision));

	vec2 screen_pixel_size = vec2(1.0, 1.0) / vec2(float(horizontal_precision), float(vertical_precision));
    vec2 pixelCoords = index2D * point_size;// * screen_pixel_size;

    // float screenSize = float(horizontal_precision) * float(vertical_precision);
    
    // vec2 normalized = pixelCoords / screenSize;
    vec2 normalized = vec2(pixelCoords.x / float(horizontal_precision), pixelCoords.y / float(vertical_precision));
    vec2 ndc = normalized * 2.0 - 1.0;
    
    uv_interp = normalized;
    float depth_mm = depthInMillimeters(source, uv_interp);
	float depth_meters = depth_mm * 0.001;
    float normalized_depth = inverseLerp(depth_meters, 0.0, 2.0);
    odepth = normalized_depth;

    // vec4 position = vec4(ndc, depth, 1.0);
    // gl_Position = position;

    // vec4 position = vec4(normalized*100.0, 0.0, 1.0); // grille depuis le centre vers bas gauche
    vec4 position = vec4(ndc, normalized_depth, 1.0);
    gl_Position = position;
}

/* clang-format off */
#[fragment]

layout(location = 0) out vec4 frag_color;
in vec2 uv_interp;
in float odepth;
/* clang-format on */

void main() {
	frag_color = vec4(uv_interp, odepth, 1.0);
	// frag_color = vec4(0.0, 1.0, 1.0, 1.0);
}
