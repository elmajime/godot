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

void print_gl_error2(const char * in_message) {
	std::string error_value;
	switch (glGetError())
	{
	case GL_NO_ERROR:
		error_value = "";
		break;
	case GL_INVALID_ENUM:
		error_value = "GL_INVALID_ENUM";
		break;
	case GL_INVALID_VALUE:
		error_value = "GL_INVALID_VALUE";
		break;
	case GL_INVALID_OPERATION:
		error_value = "GL_INVALID_OPERATION";
		break;
	case GL_OUT_OF_MEMORY:
		error_value = "GL_OUT_OF_MEMORY";
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		error_value = "GL_INVALID_FRAMEBUFFER_OPERATION";
		break;
	default:
		break;
	}
	// if (! error_value.empty()) 
	{
		OS::get_singleton()->print((std::string("MCT_Godot : ") + std::string(in_message) + " " + error_value).c_str());
	}
}

OcclusionEffects *OcclusionEffects::singleton = nullptr;

OcclusionEffects *OcclusionEffects::get_singleton() {
	return singleton;
}

OcclusionEffects::OcclusionEffects() {
	singleton = this;

	occlusion.shader.initialize();
	occlusion.shader_version = occlusion.shader.version_create();
	occlusion.shader.version_bind_shader(occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	print_gl_error2("MCT Occ 1");

	{ // Screen Points.
		glGenBuffers(1, &screen_point_cloud);
		glBindBuffer(GL_ARRAY_BUFFER, screen_point_cloud);

		GLfloat qv[] = {
			0.0f,
			0.0f
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(qv), qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind

		glGenVertexArrays(1, &screen_point_cloud_array);
		glBindVertexArray(screen_point_cloud_array);
		glBindBuffer(GL_ARRAY_BUFFER, screen_point_cloud);
		glVertexAttribPointer(RS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(RS::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind
	}
	print_gl_error2("MCT Occ 2");
}

OcclusionEffects::~OcclusionEffects() {
	singleton = nullptr;
	glDeleteBuffers(1, &screen_point_cloud);
	glDeleteVertexArrays(1, &screen_point_cloud_array);
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

void OcclusionEffects::fill_z_buffer(uint8_t p_vertical_precision, uint8_t p_horizontal_precision, const float* p_inv_view_mat, const float* p_inv_proj_mat) {

	bool success = occlusion.shader.version_bind_shader(occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	print_gl_error2("MCT Occ 3");
	if (!success) {
		OS::get_singleton()->print("MCT_Godot : OcclusionShaderGLES3 Could not bind version_bind_shader");
		return;
	}
	OS::get_singleton()->print("MCT_Godot : p_vertical_precision %d", p_vertical_precision);
	OS::get_singleton()->print("MCT_Godot : p_horizontal_precision %d", p_horizontal_precision);

	Transform3D inv_view_mat = transform3D_from_mat4(p_inv_view_mat);
	Transform3D inv_proj_mat = transform3D_from_mat4(p_inv_proj_mat);

	Projection inv_view_proj_mat = inv_view_mat * inv_proj_mat;

	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::VERTICAL_PRECISION, (int)(p_vertical_precision), occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	print_gl_error2("MCT Occ 4");
	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::HORIZONTAL_PRECISION, (int)(p_horizontal_precision), occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	print_gl_error2("MCT Occ 5");
	// occlusion.shader.version_set_uniform(OcclusionShaderGLES3::INV_VIEW_PROJECTION_MATRIX, inv_view_proj_mat, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	// print_gl_error2("MCT Occ 6");

	draw_screen_point_cloud(p_vertical_precision*p_horizontal_precision);
	print_gl_error2("MCT Occ 7");
}

// void OcclusionEffects::copy_depthmap(float midDepthMeters, float maxDepthMeters) {
// 	bool success = occlusion.shader.version_bind_shader(occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT, OcclusionShaderGLES3::COPY_DEPTHMAP);
// 	if (!success) {
// 		OS::get_singleton()->print("MCT_Godot : Could not bind version_bind_shader COPY_DEPTHMAP");
// 		return;
// 	}

// 	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::MIDDEPTHMETERS, midDepthMeters, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
// 	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::MAXDEPTHMETERS, maxDepthMeters, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);

// 	draw_screen_point_cloud();
// }

void OcclusionEffects::draw_screen_point_cloud(uint16_t p_nbPoints) {
	glBindVertexArray(screen_point_cloud_array);
	print_gl_error2("MCT Occ 8");
	// glEnable(GL_PROGRAM_POINT_SIZE);
	// print_gl_error2("MCT Occ 9");
	glDrawArrays(GL_POINTS, 0, p_nbPoints);
	print_gl_error2("MCT Occ 10");
	// glDisable(GL_PROGRAM_POINT_SIZE);
	// print_gl_error2("MCT Occ 11");
	glBindVertexArray(0);
	print_gl_error2("MCT Occ 12");
}

#endif // GLES3_ENABLED
