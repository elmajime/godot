/**************************************************************************/
/*  feed_effects.cpp                                                      */
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

#include "feed_effects.h"

#ifdef ANDROID_ENABLED
#include <GLES3/gl3ext.h>
#endif

using namespace GLES3;

FeedEffects *FeedEffects::singleton = nullptr;

FeedEffects *FeedEffects::get_singleton() {
	return singleton;
}

FeedEffects::FeedEffects() {
	singleton = this;

	feed.shader.initialize();
	feed.shader_version = feed.shader.version_create();
	feed.shader.version_bind_shader(feed.shader_version, FeedShaderGLES3::MODE_DEFAULT);

	{ // Screen Triangle.
		glGenBuffers(1, &screen_triangle);
		glBindBuffer(GL_ARRAY_BUFFER, screen_triangle);

		const float qv[6] = {
			-1.0f,
			-1.0f,
			3.0f,
			-1.0f,
			-1.0f,
			3.0f,
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

FeedEffects::~FeedEffects() {
	singleton = nullptr;
	glDeleteBuffers(1, &screen_triangle);
	glDeleteVertexArrays(1, &screen_triangle_array);
	glDeleteBuffers(1, &quad);
	glDeleteVertexArrays(1, &quad_array);
	feed.shader.version_free(feed.shader_version);
}

void FeedEffects::copy_external_feed() {

	// Check if GL_OES_EGL_image_external_essl3 extension is supported
    const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
    if (!extensions || !strstr(extensions, "GL_OES_EGL_image_external_essl3")) {
		OS::get_singleton()->print("MCT_Godot : GL_OES_EGL_image_external_essl3 extension not supported!");
        // Handle the error
        return;
    }

	bool success = feed.shader.version_bind_shader(feed.shader_version, FeedShaderGLES3::MODE_DEFAULT, FeedShaderGLES3::USE_EXTERNAL_SAMPLER);
	if (!success) {
		OS::get_singleton()->print("MCT_Godot : FeedShaderGLES3 Could not bind version_bind_shader USE_EXTERNAL_SAMPLER");
		return;
	}

	draw_screen_triangle();
}

void FeedEffects::copy_depthmap(float midDepthMeters, float maxDepthMeters) {
	bool success = feed.shader.version_bind_shader(feed.shader_version, FeedShaderGLES3::MODE_DEFAULT, FeedShaderGLES3::COPY_DEPTHMAP);
	if (!success) {
		OS::get_singleton()->print("MCT_Godot : FeedShaderGLES3 Could not bind version_bind_shader COPY_DEPTHMAP");
		return;
	}

	feed.shader.version_set_uniform(FeedShaderGLES3::MID_DEPTH_METERS, midDepthMeters, feed.shader_version, FeedShaderGLES3::MODE_DEFAULT, FeedShaderGLES3::COPY_DEPTHMAP);
	feed.shader.version_set_uniform(FeedShaderGLES3::MAX_DEPTH_METERS, maxDepthMeters, feed.shader_version, FeedShaderGLES3::MODE_DEFAULT, FeedShaderGLES3::COPY_DEPTHMAP);

	draw_screen_triangle();
}

void FeedEffects::draw_screen_triangle() {
	glBindVertexArray(screen_triangle_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

void FeedEffects::draw_screen_quad() {
	glBindVertexArray(quad_array);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

#endif // GLES3_ENABLED
