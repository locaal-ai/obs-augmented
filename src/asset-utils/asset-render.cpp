
#include "asset-render.h"
#include "augmented-filter-data.h"
#include "plugin-support.h"
#include "render-utils.h"

#include <obs.h>
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <util/platform.h>

#include <assimp/scene.h>

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

	gs_begin_scene();

	gs_blend_state_push();
	gs_enable_blending(false);
	// gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

	gs_enable_depth_test(afd->depthTest);
	gs_depth_function((gs_depth_test)afd->depthFunction);

	gs_cull_mode previous_cull_mode = gs_get_cull_mode();
	gs_set_cull_mode((gs_cull_mode)afd->cullMode);

	gs_enable_stencil_test(afd->stencilTest);
	gs_enable_stencil_write(afd->stencilWrite);
	gs_stencil_function((gs_stencil_side)afd->stencilFunction,
			    (gs_depth_test)afd->stencilDepthFunction);
	gs_stencil_op((gs_stencil_side)afd->stencilOpSide,
		      (gs_stencil_op_type)afd->stencilOpFail,
		      (gs_stencil_op_type)afd->stencilOpDepthFail,
		      (gs_stencil_op_type)afd->stencilOpPass);

	const bool previous_fb_enabled = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	gs_matrix_identity();

	vec3 LightColor;
	vec3_set(&LightColor, 1.0f, 1.0f, 1.0f); // White light

	// add matrices to the effect
	gs_effect_set_matrix4(gs_effect_get_param_by_name(afd->effect,
							  "ViewProj"),
			      &afd->projectionMatrix);
	gs_effect_set_matrix4(gs_effect_get_param_by_name(afd->effect,
							  "WorldViewProj"),
			      &afd->worldViewProjMatrix);
	gs_effect_set_matrix4(gs_effect_get_param_by_name(afd->effect,
							  "ModelMatrix"),
			      &afd->modelMatrix);
	gs_effect_set_matrix4(gs_effect_get_param_by_name(afd->effect,
							  "NormalMatrix"),
			      &afd->normalMatrix);
	gs_effect_set_vec3(gs_effect_get_param_by_name(afd->effect,
						       "LightPosition"),
			   &afd->lightPosition);
	gs_effect_set_vec3(gs_effect_get_param_by_name(afd->effect,
						       "LightColor"),
			   &LightColor);
	gs_effect_set_float(gs_effect_get_param_by_name(afd->effect,
							"depthFactor"),
			    afd->depthFactor);
	gs_effect_set_float(gs_effect_get_param_by_name(afd->effect,
							"depthBias"),
			    afd->depthBias);

	struct vec4 background;
	vec4_zero(&background);
	gs_clear(afd->clearMode, &background, afd->depthClear,
		 (uint8_t)afd->stencilClear);

	gs_render_start(true);
	drawAssimpAsset(afd->asset, obs_to_glm(afd->modelMatrix));
	afd->vbo = gs_render_save();

	gs_load_vertexbuffer(afd->vbo);

	while (gs_effect_loop(afd->effect, "Draw")) {
		gs_draw(GS_TRIS, 0, 0);
	}

	gs_render_stop(GS_TRIS);

	gs_blend_state_pop();
	gs_set_cull_mode(previous_cull_mode);
	gs_enable_framebuffer_srgb(previous_fb_enabled);
	gs_end_scene();
	gs_texrender_end(afd->texrender);

	gs_enable_depth_test(false);

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