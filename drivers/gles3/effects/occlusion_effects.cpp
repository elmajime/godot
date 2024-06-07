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

	print_gl_error2("MCT Occ 2");
}

OcclusionEffects::~OcclusionEffects() {
	singleton = nullptr;
	glDeleteBuffers(1, &screen_point_cloud);
	glDeleteVertexArrays(1, &screen_point_cloud_array);
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

	// bool success = occlusion.shader.version_bind_shader(occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	// print_gl_error2("MCT Occ 3");
	// if (!success) {
	// 	OS::get_singleton()->print("MCT_Godot : OcclusionShaderGLES3 Could not bind version_bind_shader");
	// 	return;
	// }

	bool success = occlusion.shader.version_bind_shader(occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT, OcclusionShaderGLES3::USE_EXTERNAL_SAMPLER);
	if (!success) {
		OS::get_singleton()->print("MCT_Godot : OcclusionShaderGLES3 Could not bind version_bind_shader USE_EXTERNAL_SAMPLER");
		return;
	}
	print_gl_error2("MCT Occ 3");

	// int32_t nb_horizontal_points = p_point_size >= p_width ? 1 : p_width / p_point_size;
	// int32_t nb_vertical_points = p_point_size >= p_height ? 1 : p_height / p_point_size;
	// int32_t nb_points = nb_horizontal_points*nb_vertical_points;
	// float screen_point_size = p_point_size / p_width;

	int16_t show_depthmap = p_show_depthmap ? 1 : 0;
	int16_t use_depth = p_use_depth ? 1 : 0;

	// OS::get_singleton()->print("MCT_Godot : p_point_size %f", p_point_size);
	// OS::get_singleton()->print("MCT_Godot : nb_horizontal_points %d", nb_horizontal_points);
	// OS::get_singleton()->print("MCT_Godot : nb_vertical_points %d", nb_vertical_points);
	// OS::get_singleton()->print("MCT_Godot : nb_points %d", nb_points);
	// OS::get_singleton()->print("MCT_Godot : screen_point_size %f", screen_point_size);
	OS::get_singleton()->print("MCT_Godot : p_max_depth_meters %f", p_max_depth_meters);
	OS::get_singleton()->print("MCT_Godot : p_show_depthmap %f", show_depthmap);

	// occlusion.shader.version_set_uniform(OcclusionShaderGLES3::NB_HORIZONTAL_POINTS, nb_horizontal_points, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	// print_gl_error2("MCT Occ 4");
	// occlusion.shader.version_set_uniform(OcclusionShaderGLES3::NB_VERTICAL_POINTS, nb_vertical_points, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	// print_gl_error2("MCT Occ 5");
	// occlusion.shader.version_set_uniform(OcclusionShaderGLES3::SCREEN_POINT_SIZE, screen_point_size, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::MAX_DEPTH, p_max_depth_meters, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	print_gl_error2("MCT Occ 5");
	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::SHOW_DEPTHMAP, show_depthmap, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	print_gl_error2("MCT Occ 6");
	occlusion.shader.version_set_uniform(OcclusionShaderGLES3::USE_DEPTH, use_depth, occlusion.shader_version, OcclusionShaderGLES3::MODE_DEFAULT);
	print_gl_error2("MCT Occ 7");

	// draw_screen_point_cloud(nb_points);
	draw_screen_quad(1);
	print_gl_error2("MCT Occ 7");
}

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

void OcclusionEffects::draw_screen_triangle(uint16_t p_nbPoints) {
	glBindVertexArray(screen_triangle_array);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 3, p_nbPoints);
	glBindVertexArray(0);
}

void OcclusionEffects::draw_screen_quad(uint16_t p_nbPoints) {
	glBindVertexArray(quad_array);
	glDrawArrays(GL_TRIANGLES, 0, p_nbPoints*6);
	// glDrawArraysInstanced(GL_TRIANGLES, 0, 6, p_nbPoints);
	glBindVertexArray(0);
}

#endif // GLES3_ENABLED
