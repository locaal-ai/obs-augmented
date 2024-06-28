
#include "asset-render.h"
#include "augmented-filter-data.h"

#include <obs.h>
#include <graphics/graphics.h>

#include <assimp/scene.h>

bool render_asset_3d(augmented_filter_data *afd, const aiScene *scene)
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
	gs_viewport_push();
	gs_matrix_push();
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	if (!gs_texrender_begin(afd->texrender, width, height)) {
		return false;
	}
	struct vec4 background;
	vec4_zero(&background);
	gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);

	gs_perspective(120.0f, (float)width / (float)height,
		       1.0f / (float)(1 << 22), (float)(1 << 22));

	gs_vb_data *vb = gs_vbdata_create();
	vb->points =
		(vec3 *)bmalloc(sizeof(vec3) * scene->mMeshes[0]->mNumVertices);
	vb->colors = (uint32_t *)bmalloc(sizeof(uint32_t) *
					 scene->mMeshes[0]->mNumVertices);
	vb->num = scene->mMeshes[0]->mNumVertices;
	for (unsigned int i = 0; i < scene->mMeshes[0]->mNumVertices; i++) {
		vb->points[i].x = scene->mMeshes[0]->mVertices[i].x;
		vb->points[i].y = scene->mMeshes[0]->mVertices[i].y;
		vb->points[i].z = scene->mMeshes[0]->mVertices[i].z;
		vb->colors[i] = 0xFFFFFFFF;
	}
	gs_vertbuffer_t *vbo = gs_vertexbuffer_create(vb, GS_DYNAMIC);
	gs_vertexbuffer_flush(vbo);
	gs_load_vertexbuffer(vbo);
	gs_load_indexbuffer(NULL);

	gs_draw(GS_TRIS, 0, 0);

	gs_vertexbuffer_destroy(vbo);

	gs_matrix_identity();

	gs_texrender_end(afd->texrender);
	gs_blend_state_pop();
	gs_matrix_pop();
	gs_viewport_pop();

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