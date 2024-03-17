/**************************************************************************/
/*  occlusion_effects.cpp                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifdef GLES3_ENABLED

#include "occlusion_effects.h"

#ifdef ANDROID_ENABLED
#include <GLES3/gl3ext.h>
#endif

#define GL_PROGRAM_POINT_SIZE 0x8642

using namespace GLES3;

OcclusionEffects *OcclusionEffects::singleton = nullptr;

OcclusionEffects *OcclusionEffects::get_singleton() {
	return singleton;
}

OcclusionEffects::OcclusionEffects() {
	singleton = this;

	occlusion.shader.initialize();
	occlusion.shader_version = occlusion.shader.version_create();
	occlusion.shader.version_bind_shader(occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);

	{ // Screen Triangle.
		glGenBuffers(1, &screen_triangle);
		glBindBuffer(GL_ARRAY_BUFFER, screen_triangle);

		const float qv[6] = {
			-1.0f,
			-1.0f,
			1.0f,
			-1.0f,
			-1.0f,
			1.0f,
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind

		glGenVertexArrays(1, &screen_triangle_array);
		glBindVertexArray(screen_triangle_array);
		glBindBuffer(GL_ARRAY_BUFFER, screen_triangle);
		glVertexAttribPointer(RS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RS::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind
	}

	{ // Screen Quad

		glGenBuffers(1, &quad);
		glBindBuffer(GL_ARRAY_BUFFER, quad);

		const float qv[12] = {
			-1.0f,
			-1.0f,
			1.0f,
			-1.0f,
			1.0f,
			1.0f,
			-1.0f,
			-1.0f,
			1.0f,
			1.0f,
			-1.0f,
			1.0f,
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind

		glGenVertexArrays(1, &quad_array);
		glBindVertexArray(quad_array);
		glBindBuffer(GL_ARRAY_BUFFER, quad);
		glVertexAttribPointer(RS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RS::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind
	}

}

OcclusionEffects::~OcclusionEffects() {
	singleton = nullptr;
	glDeleteBuffers(1, &screen_triangle);
	glDeleteVertexArrays(1, &screen_triangle_array);
	occlusion.shader.version_free(occlusion.shader_version);
}

Transform3D transform3D_from_mat4(const float* p_mat4) {
	Transform3D res;

	res.basis.rows[0][0] = p_mat4[0];
	res.basis.rows[1][0] = p_mat4[1];
	res.basis.rows[2][0] = p_mat4[2];
	// p_mat4[3] = 0;
	res.basis.rows[0][1] = p_mat4[4];
	res.basis.rows[1][1] = p_mat4[5];
	res.basis.rows[2][1] = p_mat4[6];
	// p_mat4[7] = 0;
	res.basis.rows[0][2] = p_mat4[8];
	res.basis.rows[1][2] = p_mat4[9];
	res.basis.rows[2][2] = p_mat4[10];
	// p_mat4[11] = 0;
	res.origin.x = p_mat4[12];
	res.origin.y = p_mat4[13];
	res.origin.z = p_mat4[14];
	// p_mat4[15] = 1;

	return res;
}

void OcclusionEffects::fill_z_buffer(bool p_use_depth, bool p_show_depthmap, float p_max_depth_meters) {
	bool success = occlusion.shader.version_bind_shader(occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT, OcclusionShaderGLES3::USE_EXTERNAL_SAMPLER);
	if (!success) {
		OS::get_singleton()->print("Godot : OcclusionShaderGLES3 Could not bind version_bind_shader USE_EXTERNAL_SAMPLER");
		return;
	}

	int16_t show_depthmap = p_show_depthmap ? 1 : 0;
	int16_t use_depth = p_use_depth ? 1 : 0;

	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::MAX_DEPTH, p_max_depth_meters, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::SHOW_DEPTHMAP, show_depthmap, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::USE_DEPTH, use_depth, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);

	draw_screen_quad();
}


void OcclusionEffects::draw_screen_triangle() {
	glBindVertexArray(screen_triangle_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

void OcclusionEffects::draw_screen_quad() {
	glBindVertexArray(quad_array);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

#endif // GLES3_ENABLED
