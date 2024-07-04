
#include "asset-render.h"
#include "augmented-filter-data.h"

#include <obs.h>
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <util/platform.h>

#include <assimp/scene.h>

matrix4 createViewMatrix(vec3 eye, vec3 target, vec3 up)
{
	// Assuming you have a function to create a lookAt matrix, or you can manually create one.
	vec3 eye_to_target;
	vec3_sub(&eye_to_target, &target, &eye);
	vec3 zaxis; // The "forward" vector.
	vec3_norm(&zaxis, &eye_to_target);
	vec3 up_zaxis_cross;
	vec3_cross(&up_zaxis_cross, &up, &zaxis);
	vec3 xaxis; // The "right" vector.
	vec3_norm(&xaxis, &up_zaxis_cross);
	vec3 yaxis; // The "up" vector.
	vec3_cross(&yaxis, &zaxis, &xaxis);

	matrix4 result;
	result.x.ptr[0] = xaxis.x;
	result.x.ptr[1] = xaxis.y;
	result.x.ptr[2] = xaxis.z;
	result.x.ptr[3] = -vec3_dot(&xaxis, &eye);
	result.y.ptr[0] = yaxis.x;
	result.y.ptr[1] = yaxis.y;
	result.y.ptr[2] = yaxis.z;
	result.y.ptr[3] = -vec3_dot(&yaxis, &eye);
	result.z.ptr[0] = zaxis.x;
	result.z.ptr[1] = zaxis.y;
	result.z.ptr[2] = zaxis.z;
	result.z.ptr[3] = -vec3_dot(&zaxis, &eye);
	result.t.ptr[3] = 1.0f;

	return result;
}

matrix4 createProjectionMatrix(float fovY, float aspectRatio, float nearZ,
			       float farZ)
{
	const float tanHalfFovY = tanf(fovY / 2.0f);
	const float zRange = farZ - nearZ;

	matrix4 result;

	result.x.ptr[0] = 1.0f / (aspectRatio * tanHalfFovY);
	result.y.ptr[1] = 1.0f / tanHalfFovY;
	result.z.ptr[2] = farZ / zRange;
	result.z.ptr[3] = -(farZ * nearZ) / zRange;
	result.t.ptr[2] = 1.0f;
	result.t.ptr[3] = 0.0f;

	return result;
}

// Function to draw a single triangle
void drawTriangle(const aiVector3D *vertices, const aiVector3D *normals)
{
	for (int i : {0, 2, 1}) {
		if (normals) {
			gs_normal3f(normals[i].x, normals[i].y, normals[i].z);
		}
		gs_vertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
		gs_color(0xFFFFFFFF);
	}
}

// Function to draw an Assimp mesh
void drawAssimpMesh(const aiMesh *mesh)
{
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		const aiFace &face = mesh->mFaces[i];
		if (face.mNumIndices == 3) {
			aiVector3D vertices[3];
			aiVector3D normals[3];

			for (int j : {0, 1, 2}) {
				const unsigned int index = face.mIndices[j];
				vertices[j] = mesh->mVertices[index];
				if (mesh->HasNormals()) {
					normals[j] = mesh->mNormals[index];
				}
			}

			drawTriangle(vertices,
				     mesh->HasNormals() ? normals : nullptr);
		}
	}
}

bool render_asset_3d(augmented_filter_data *afd)
{
	obs_source_t *target = obs_filter_get_target(afd->source);
	if (!target) {
		return false;
	}
	uint32_t width = obs_source_get_base_width(target);
	uint32_t height = obs_source_get_base_height(target);
	if (width == 0 || height == 0) {
		return false;
	}
	gs_texrender_reset(afd->texrender);
	if (!gs_texrender_begin(afd->texrender, width, height)) {
		return false;
	}
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	struct vec4 background;
	vec4_zero(&background);

	// gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
	gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &background, 0.0f, 0);

	gs_enable_depth_test(afd->depthTest);
	gs_depth_function((gs_depth_test)afd->depthFunction);

	gs_cull_mode previous_cull_mode = gs_get_cull_mode();
	gs_set_cull_mode((gs_cull_mode)afd->cullMode);

	gs_enable_stencil_test(afd->stencilTest);
	gs_enable_stencil_write(afd->stencilWrite);
	gs_stencil_function((gs_stencil_side)afd->stencilFunction, GS_ALWAYS);

	gs_matrix_identity();
	// gs_reset_viewport();

	// apply afd->modelMatrix
	matrix4 modelMatrix;
	matrix4_identity(&modelMatrix);
	for (int i = 0; i < 4; i++) {
		modelMatrix.x.ptr[i] = (float)afd->modelMatrix[0][i];
		modelMatrix.y.ptr[i] = (float)afd->modelMatrix[1][i];
		modelMatrix.z.ptr[i] = (float)afd->modelMatrix[2][i];
		modelMatrix.t.ptr[i] = (float)afd->modelMatrix[3][i];
	}

	vec3 eye = {0.0f, 2.0f, 5.0f};
	vec3 target_v = {0.0f, 0.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};

	matrix4 viewMatrix = createViewMatrix(eye, target_v, up);

	float aspectRatio = (float)width / (float)height;
	matrix4 projectionMatrix =
		createProjectionMatrix(afd->fov, aspectRatio, 0.1f, 100.0f);

	matrix4 worldViewProjMatrix;
	matrix4_mul(&worldViewProjMatrix, &projectionMatrix, &viewMatrix);
	matrix4_mul(&worldViewProjMatrix, &worldViewProjMatrix, &modelMatrix);

	vec3 LightPosition;
	vec3_set(&LightPosition, 0.0f, 0.0f, 1.0f); // Light from the front
	vec3 LightColor;
	vec3_set(&LightColor, 1.0f, 1.0f, 1.0f); // White light

	// add matrices to the effect
	gs_effect_set_matrix4(gs_effect_get_param_by_name(afd->effect,
							  "ViewProj"),
			      &projectionMatrix);
	gs_effect_set_matrix4(gs_effect_get_param_by_name(afd->effect,
							  "WorldViewProj"),
			      &worldViewProjMatrix);
	gs_effect_set_matrix4(gs_effect_get_param_by_name(afd->effect,
							  "ModelMatrix"),
			      &modelMatrix);
	gs_effect_set_vec3(gs_effect_get_param_by_name(afd->effect,
						       "LightPosition"),
			   &LightPosition);
	gs_effect_set_vec3(gs_effect_get_param_by_name(afd->effect,
						       "LightColor"),
			   &LightColor);

	// gs_perspective(afd->fov, (float)width / (float)height, 0.1f, 100.0f);
	// gs_ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);

	while (gs_effect_loop(afd->effect, "Draw")) {
		gs_load_vertexbuffer(afd->vbo);
		gs_draw(GS_TRIS, 0, 0);
	}

	gs_blend_state_pop();
	gs_set_cull_mode(previous_cull_mode);
	gs_texrender_end(afd->texrender);

	return true;
}

/*
void effect_3d_draw_frame(struct effect_3d *context, uint32_t w, uint32_t h)
{
	const enum gs_color_space current_space = gs_get_color_space();
	float multiplier;
	const char *technique = get_tech_name_and_multiplier(
		current_space, context->space, &multiplier);

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_texture_t *tex = gs_texrender_get_texture(context->render);
	if (!tex)
		return;

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	gs_effect_set_texture_srgb(gs_effect_get_param_by_name(effect, "image"),
				   tex);
	gs_effect_set_float(gs_effect_get_param_by_name(effect, "multiplier"),
			    multiplier);

	while (gs_effect_loop(effect, technique))
		gs_draw_sprite(tex, 0, w, h);

	gs_enable_framebuffer_srgb(previous);
	gs_blend_state_pop();
}
*/