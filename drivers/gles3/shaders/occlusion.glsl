/* clang-format off */
#version 300 es

#[modes]

mode_default = 

#[specializations]

USE_EXTERNAL_SAMPLER = false
COPY_DEPTHMAP = false

#[vertex]

layout(location = 0) in vec2 vertex_attrib;

out vec2 uv_interp;
// out float odepth;
/* clang-format on */

// uniform sampler2D source; // texunit:0
// uniform int nb_horizontal_points;
// uniform int nb_vertical_points;
// uniform float screen_point_size;
// uniform float max_depth;
// uniform mat4 inv_view_projection_matrix;

// float depthInMillimeters(in sampler2D depthTexture, in vec2 depthUV) {
// 	// Depth is packed into the red and green components of its texture.
// 	// The texture is a normalized format, storing millimeters.
// 	vec2 packedDepthAndVisibility = texture(depthTexture, depthUV).xy;
// 	return dot(packedDepthAndVisibility.xy, vec2(255.0, 256.0 * 255.0));
// }

// float inverseLerp(float value, float minBound, float maxBound) {
// 	return clamp((value - minBound) / (maxBound - minBound), 0.0, 1.0);
// }

void main() {
    // int nb_vertices = 6;

    // int index = gl_VertexID / nb_vertices;

    // vec2 index2D = vec2(float(index % nb_horizontal_points), float(index/nb_horizontal_points));

    // vec2 normalized_point_position = vec2(index2D.x / float(nb_horizontal_points), index2D.y / float(nb_vertical_points));

    // vec2 normalized = normalized_point_position + vertex_attrib * screen_point_size;

    // vec2 ndc = (normalized) * 2.0;

    // uv_interp = normalized;
    
	uv_interp = vertex_attrib * 0.5 + 0.5;

    // float depth_mm = depthInMillimeters(source, uv_interp);
	// float depth_meters = depth_mm * 0.001;
    // float normalized_depth = inverseLerp(depth_meters, 0.0, max_depth);
    // odepth = normalized_depth;

    // vec4 position = vec4(ndc, 0.0, 1.0);
    // gl_Position = position;

    
	gl_Position = vec4(vertex_attrib, 1.0, 1.0);
}

/* clang-format off */
#[fragment]

layout(location = 0) out vec4 frag_color;
in vec2 uv_interp;
// in float odepth;
/* clang-format on */
uniform sampler2D source; // texunit:0
#ifdef USE_EXTERNAL_SAMPLER
uniform samplerExternalOES sourceFrag; // texunit:1
#else
uniform sampler2D sourceFrag; // texunit:1
#endif

uniform float max_depth;
uniform int use_depth;
uniform int show_depthmap;

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
    vec4 color = texture(sourceFrag, uv_interp);
    if (use_depth == 1) {
        float depth_mm = depthInMillimeters(source, uv_interp);
        float depth_meters = depth_mm * 0.001;
        float normalized_depth = inverseLerp(depth_meters, 0.0, max_depth);
        float odepth = normalized_depth;

        gl_FragDepth = odepth;

        if (show_depthmap == 1) {
            color = vec4(odepth, 0.0, 0.0, 1.0);
        }
    }
    else {
        gl_FragDepth = 1.0;
    }

	frag_color = color;
}
