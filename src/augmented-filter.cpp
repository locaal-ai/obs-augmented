#include <obs.h>
#include <assimp/cimport.h>
#include <graphics/graphics.h>
#include <graphics/matrix4.h>

#include "augmented-filter.h"
#include "augmented-filter-data.h"
#include "plugin-support.h"
#include "asset-utils/asset-loader.h"
#include "asset-utils/asset-render.h"
#include "asset-utils/render-utils.h"

void augmented_filter_activate(void *data)
{
	UNUSED_PARAMETER(data);
}

void build_projection_matrix(augmented_filter_data *afd)
{
	const float aspectRatio =
		(float)afd->source_width / (float)afd->source_height;
	afd->projectionMatrix =
		createProjectionMatrix(afd->fov, aspectRatio, 0.1f, 100.0f);
}

void update_from_settings(augmented_filter_data *afd, obs_data_t *s)
{
	// get file path of the built in asset
	const char *assetPathFromSettings = obs_data_get_string(s, "asset");
	if (afd->assetPath != assetPathFromSettings) {
		afd->assetPath = assetPathFromSettings;
		char *assetPath = obs_module_file(assetPathFromSettings);
		if (!assetPath) {
			obs_log(LOG_ERROR, "Failed to get asset path");
			afd->isDisabled = true;
			return;
		}
		afd->asset = load_asset(assetPath);
		bfree(assetPath);
		if (!afd->asset) {
			obs_log(LOG_ERROR, "Failed to load asset: %s",
				assetPath);
			afd->isDisabled = true;
			return;
		}
	}

	// get the rotation value
	const float rotation = (float)obs_data_get_double(s, "rotation");
	// get the x position value
	const float x = (float)obs_data_get_double(s, "x");
	// get the y position value
	const float y = (float)obs_data_get_double(s, "y");
	// get the z position value
	const float z = (float)obs_data_get_double(s, "z");
	// get the scale value
	const float scale = (float)obs_data_get_double(s, "scale");

	float new_fov = (float)obs_data_get_double(s, "fov") * (M_PI / 180.0f);
	afd->autoRotate = obs_data_get_bool(s, "auto_rotate");
	afd->depthFunction = (int)obs_data_get_int(s, "depth_function");
	afd->cullMode = (int)obs_data_get_int(s, "cull_mode");
	afd->depthTest = obs_data_get_bool(s, "depth_test");
	afd->stencilTest = obs_data_get_bool(s, "stencil_test");
	afd->stencilWrite = obs_data_get_bool(s, "stencil_write");
	afd->stencilFunction = (int)obs_data_get_int(s, "stencil_function");
	afd->stencilDepthFunction =
		(int)obs_data_get_int(s, "stencil_function_depth_test");
	afd->stencilOpSide = (int)obs_data_get_int(s, "stencil_op_side");
	afd->stencilOpFail = (int)obs_data_get_int(s, "stencil_op_fail");
	afd->stencilOpDepthFail = (int)obs_data_get_int(s, "stencil_op_z_fail");
	afd->stencilOpPass = (int)obs_data_get_int(s, "stencil_op_pass");
	afd->stencilClear = (int)obs_data_get_int(s, "stencil_clear");

	afd->clearMode = (int)obs_data_get_int(s, "clear_mode");
	afd->depthClear = (float)obs_data_get_double(s, "depth_clear");
	afd->depthFactor = (float)obs_data_get_double(s, "depth_factor");
	afd->depthBias = (float)obs_data_get_double(s, "depth_bias");

	afd->modelMatrix = createModelMatrix({x, y, z}, {0.0f, rotation, 0.0f},
					     {scale, scale, scale});

	const vec3 eye = {0.0f, 2.0f, 5.0f};
	const vec3 target_v = {0.0f, 0.0f, 0.0f};
	const vec3 up = {0.0f, 1.0f, 0.0f};

	afd->viewMatrix = createViewMatrix(eye, target_v, up);
	afd->normalMatrix = calcNormalMatrix(afd->modelMatrix);

	if (new_fov != afd->fov) {
		afd->fov = new_fov;
		build_projection_matrix(afd);
	}

	// update the world view projection matrix
	matrix4_mul(&afd->worldViewProjMatrix, &afd->projectionMatrix,
		    &afd->viewMatrix);
	matrix4_mul(&afd->worldViewProjMatrix, &afd->worldViewProjMatrix,
		    &afd->modelMatrix);

	// get the light position
	const float light_x = (float)obs_data_get_double(s, "light_x");
	const float light_y = (float)obs_data_get_double(s, "light_y");
	const float light_z = (float)obs_data_get_double(s, "light_z");
	afd->lightPosition = {light_x, light_y, light_z};
}

void *augmented_filter_create(obs_data_t *settings, obs_source_t *filter)
{
	obs_log(LOG_INFO, "Augmented filter created");
	void *data = bmalloc(sizeof(struct augmented_filter_data));
	struct augmented_filter_data *afd = new (data) augmented_filter_data();

	afd->source = filter;
	afd->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);

	// load the effect
	char *effectPathPtr = obs_module_file("effects/draw3d.effect");
	if (!effectPathPtr) {
		obs_log(LOG_ERROR,
			"Failed to get effect path: effects/draw3d.effect");
		afd->isDisabled = true;
		return afd;
	}
	obs_log(LOG_INFO, "Effect path: %s", effectPathPtr);
	obs_enter_graphics();
	afd->effect = gs_effect_create_from_file(effectPathPtr, nullptr);
	bfree(effectPathPtr);
	if (!afd->effect) {
		obs_log(LOG_ERROR,
			"Failed to load effect: effects/draw3d.effect");
		afd->isDisabled = true;
		return afd;
	}

	obs_leave_graphics();

	update_from_settings(afd, settings);

	return afd;
}

void augmented_filter_update(void *data, obs_data_t *s)
{
	struct augmented_filter_data *afd =
		reinterpret_cast<augmented_filter_data *>(data);

	if (afd->isDisabled) {
		return;
	}

	update_from_settings(afd, s);
}

void augmented_filter_destroy(void *data)
{
	obs_log(LOG_INFO, "Augmented filter destroyed");

	struct augmented_filter_data *afd =
		reinterpret_cast<augmented_filter_data *>(data);

	if (afd) {
		afd->isDisabled = true;

		if (afd->asset) {
			aiReleaseImport(afd->asset);
		}

		obs_enter_graphics();
		// gs_vertexbuffer_destroy(afd->vbo);
		gs_texrender_destroy(afd->texrender);
		if (afd->stagesurface) {
			gs_stagesurface_destroy(afd->stagesurface);
		}
		obs_leave_graphics();
		afd->~augmented_filter_data();
		bfree(afd);
	}
}

const char *augmented_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Augmented Filter";
}

void augmented_filter_deactivate(void *data)
{
	UNUSED_PARAMETER(data);
}

void augmented_filter_defaults(obs_data_t *s)
{
	obs_data_set_default_string(s, "asset", "assets/crown.dae");
	obs_data_set_default_double(s, "rotation", 0.0);
	obs_data_set_default_double(s, "x", 0.0);
	obs_data_set_default_double(s, "y", 0.0);
	obs_data_set_default_double(s, "z", 0.0);
	obs_data_set_default_double(s, "scale", 1.0);
	obs_data_set_default_double(s, "fov", 60.0);
	obs_data_set_default_bool(s, "auto_rotate", true);
	obs_data_set_default_double(s, "light_x", 0.0);
	obs_data_set_default_double(s, "light_y", 0.0);
	obs_data_set_default_double(s, "light_z", 1.0);
	obs_data_set_default_int(s, "depth_function", GS_LEQUAL);
	obs_data_set_default_int(s, "cull_mode", GS_BACK);
	obs_data_set_default_bool(s, "depth_test", true);
	obs_data_set_default_bool(s, "stencil_test", false);
	obs_data_set_default_bool(s, "stencil_write", false);
	obs_data_set_default_int(s, "stencil_function", GS_STENCIL_BOTH);
	obs_data_set_default_int(s, "stencil_function_depth_test", GS_LEQUAL);
	obs_data_set_default_int(s, "stencil_op_side", GS_STENCIL_BOTH);
	obs_data_set_default_int(s, "stencil_op_fail", GS_KEEP);
	obs_data_set_default_int(s, "stencil_op_z_fail", GS_KEEP);
	obs_data_set_default_int(s, "stencil_op_pass", GS_KEEP);
	obs_data_set_default_int(s, "stencil_clear", 0);
	obs_data_set_default_int(s, "clear_mode",
				 GS_CLEAR_COLOR | GS_CLEAR_DEPTH);
	obs_data_set_default_double(s, "depth_clear", 0.0);
	obs_data_set_default_double(s, "depth_factor", 1.0);
	obs_data_set_default_double(s, "depth_bias", 0.0);
}

obs_properties_t *augmented_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	// add asset selection combo box
	obs_property_t *asset_list = obs_properties_add_list(
		props, "asset", "Asset", OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(asset_list, "Crown", "assets/crown.dae");
	obs_property_list_add_string(asset_list, "Ghost", "assets/ghost.dae");
	obs_property_list_add_string(asset_list, "Crown_02_v1dot4",
				     "assets/Crown_02_v1dot4.dae");
	obs_property_list_add_string(asset_list, "Crown_02_v1dot5",
				     "assets/Crown_02_v1dot5.dae");
	obs_property_list_add_string(asset_list, "Crown_03_v1dot4_PBRcol",
				     "assets/Crown_03_v1dot4_PBRcol.dae");
	obs_property_list_add_string(asset_list, "Crown_04_v1dot4_PBRref",
				     "assets/Crown_04_v1dot4_PBRref.dae");
	obs_property_list_add_string(asset_list, "Crown_05_blend",
				     "assets/Crown_05_blend.dae");

	// add slider for rotation
	obs_properties_add_float_slider(props, "rotation", "Rotation", -180.0,
					180.0, 0.1);
	// add slider for x position
	obs_properties_add_float_slider(props, "x", "X Position", -5.0, 5.0,
					0.01);
	// add slider for y position
	obs_properties_add_float_slider(props, "y", "Y Position", -5.0, 5.0,
					0.01);
	// add slider for z position
	obs_properties_add_float_slider(props, "z", "Z Position", -5.0, 5.0,
					0.01);
	// add slider for scale
	obs_properties_add_float_slider(props, "scale", "Scale", 0.0, 2.0,
					0.01);
	// add slider for field of view angle
	obs_properties_add_float_slider(props, "fov", "Field of View", 0.0,
					180.0, 0.1);
	// add a checkbox for auto rotate
	obs_properties_add_bool(props, "auto_rotate", "Auto Rotate");

	// add a slider for light position x
	obs_properties_add_float_slider(props, "light_x", "Light X", -5.0, 5.0,
					0.01);
	// add a slider for light position y
	obs_properties_add_float_slider(props, "light_y", "Light Y", -5.0, 5.0,
					0.01);
	// add a slider for light position z
	obs_properties_add_float_slider(props, "light_z", "Light Z", -5.0, 5.0,
					0.01);

	// add a selection for gs_depth_function
	obs_property_t *list = obs_properties_add_list(props, "depth_function",
						       "Depth Function",
						       OBS_COMBO_TYPE_LIST,
						       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Never", GS_NEVER);
	obs_property_list_add_int(list, "Less", GS_LESS);
	obs_property_list_add_int(list, "Equal", GS_EQUAL);
	obs_property_list_add_int(list, "LessEqual", GS_LEQUAL);
	obs_property_list_add_int(list, "Greater", GS_GREATER);
	obs_property_list_add_int(list, "NotEqual", GS_NOTEQUAL);
	obs_property_list_add_int(list, "GreaterEqual", GS_GEQUAL);
	obs_property_list_add_int(list, "Always", GS_ALWAYS);
	// add a selection for gs_set_cull_mode
	list = obs_properties_add_list(props, "cull_mode", "Cull Mode",
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "None", GS_NEITHER);
	obs_property_list_add_int(list, "Front", GS_FRONT);
	obs_property_list_add_int(list, "Back", GS_BACK);
	// add checkbos for depth test
	obs_properties_add_bool(props, "depth_test", "Depth Test");
	// add checkbos for stencil test
	obs_properties_add_bool(props, "stencil_test", "Stencil Test");
	// add checkbos for stencil write
	obs_properties_add_bool(props, "stencil_write", "Stencil Write");
	// add a selection for gs_stencil_function
	list = obs_properties_add_list(props, "stencil_function",
				       "Stencil Function", OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Both", GS_STENCIL_BOTH);
	obs_property_list_add_int(list, "Front", GS_STENCIL_FRONT);
	obs_property_list_add_int(list, "Back", GS_STENCIL_BACK);
	// add selectino for clear mode
	list = obs_properties_add_list(props, "clear_mode", "Clear Mode",
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Color", GS_CLEAR_COLOR);
	obs_property_list_add_int(list, "Depth", GS_CLEAR_DEPTH);
	obs_property_list_add_int(list, "Stencil", GS_CLEAR_STENCIL);
	obs_property_list_add_int(list, "Color and Depth",
				  GS_CLEAR_COLOR | GS_CLEAR_DEPTH);
	obs_property_list_add_int(list, "Color and Stencil",
				  GS_CLEAR_COLOR | GS_CLEAR_STENCIL);
	obs_property_list_add_int(list, "Depth and Stencil",
				  GS_CLEAR_DEPTH | GS_CLEAR_STENCIL);
	obs_property_list_add_int(list, "Color, Depth and Stencil",
				  GS_CLEAR_COLOR | GS_CLEAR_DEPTH |
					  GS_CLEAR_STENCIL);
	// add selection for stencil function depth test
	list = obs_properties_add_list(props, "stencil_function_depth_test",
				       "Stencil Function", OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Never", GS_NEVER);
	obs_property_list_add_int(list, "Less", GS_LESS);
	obs_property_list_add_int(list, "Equal", GS_EQUAL);
	obs_property_list_add_int(list, "LessEqual", GS_LEQUAL);
	obs_property_list_add_int(list, "Greater", GS_GREATER);
	obs_property_list_add_int(list, "NotEqual", GS_NOTEQUAL);
	obs_property_list_add_int(list, "GreaterEqual", GS_GEQUAL);
	obs_property_list_add_int(list, "Always", GS_ALWAYS);
	// add selection for stencil op side
	list = obs_properties_add_list(props, "stencil_op_side",
				       "Stencil Op Side", OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Front", GS_STENCIL_FRONT);
	obs_property_list_add_int(list, "Back", GS_STENCIL_BACK);
	obs_property_list_add_int(list, "Both", GS_STENCIL_BOTH);
	// add selection stencil op fail
	list = obs_properties_add_list(props, "stencil_op_fail",
				       "Stencil Op Fail", OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Keep", GS_KEEP);
	obs_property_list_add_int(list, "Zero", GS_ZERO);
	obs_property_list_add_int(list, "Replace", GS_REPLACE);
	obs_property_list_add_int(list, "Increment", GS_INCR);
	obs_property_list_add_int(list, "Decrement", GS_DECR);
	obs_property_list_add_int(list, "Invert", GS_INVERT);
	// add selection stencil op z fail
	list = obs_properties_add_list(props, "stencil_op_z_fail",
				       "Stencil Op Depth Fail",
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Keep", GS_KEEP);
	obs_property_list_add_int(list, "Zero", GS_ZERO);
	obs_property_list_add_int(list, "Replace", GS_REPLACE);
	obs_property_list_add_int(list, "Increment", GS_INCR);
	obs_property_list_add_int(list, "Decrement", GS_DECR);
	obs_property_list_add_int(list, "Invert", GS_INVERT);
	// add selection stencil op z fail
	list = obs_properties_add_list(props, "stencil_op_pass",
				       "Stencil Op Pass", OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "Keep", GS_KEEP);
	obs_property_list_add_int(list, "Zero", GS_ZERO);
	obs_property_list_add_int(list, "Replace", GS_REPLACE);
	obs_property_list_add_int(list, "Increment", GS_INCR);
	obs_property_list_add_int(list, "Decrement", GS_DECR);
	obs_property_list_add_int(list, "Invert", GS_INVERT);
	// add slider for stencil clear value
	obs_properties_add_int_slider(props, "stencil_clear", "Stencil Clear",
				      0, 255, 1);

	// add slider for depth clear value
	obs_properties_add_float_slider(props, "depth_clear", "Depth Clear",
					-2.0, 2.0, 0.01);
	// add slider for depth factor
	obs_properties_add_float_slider(props, "depth_factor", "Depth Factor",
					0.0, 10.0, 0.01);
	// add slider for depth bias
	obs_properties_add_float_slider(props, "depth_bias", "Depth Bias", -1.0,
					1.0, 0.01);

	return props;
}

void augmented_filter_remove(void *data, obs_source_t *source)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(source);
}

void augmented_filter_show(void *data)
{
	UNUSED_PARAMETER(data);
}

void augmented_filter_hide(void *data)
{
	UNUSED_PARAMETER(data);
}

void augmented_filter_video_tick(void *data, float seconds)
{
	// update the rotation based on the time
	struct augmented_filter_data *afd =
		reinterpret_cast<augmented_filter_data *>(data);

	if (afd->isDisabled) {
		return;
	}

	if (!afd->autoRotate) {
		return;
	}

	obs_data_t *settings = obs_source_get_settings(afd->source);
	// get the rotation value
	float rotation = (float)obs_data_get_double(settings, "rotation");
	// update the rotation value
	rotation += 30.0f * seconds;
	// set the new rotation value
	obs_data_set_double(settings, "rotation", rotation);
	// update the source settings
	update_from_settings(afd, settings);
	// free the settings
	obs_data_release(settings);
}

void augmented_filter_video_render(void *data, gs_effect_t *_effect)
{
	UNUSED_PARAMETER(_effect);

	struct augmented_filter_data *afd =
		reinterpret_cast<augmented_filter_data *>(data);

	if (afd->isDisabled) {
		return;
	}

	if (!afd->asset) {
		return;
	}

	if (afd->source_width == 0 || afd->source_height == 0) {
		afd->source_width = obs_source_get_base_width(afd->source);
		afd->source_height = obs_source_get_base_height(afd->source);
		build_projection_matrix(afd);
	}

	gs_enable_depth_test(afd->depthTest);
	gs_depth_function((gs_depth_test)afd->depthFunction);

	render_asset_3d(afd);

	obs_source_t *target = obs_filter_get_target(afd->source);
	obs_source_video_render(target);

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_texture_t *tex = gs_texrender_get_texture(afd->texrender);
	if (!tex)
		return;

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	gs_effect_set_texture_srgb(gs_effect_get_param_by_name(effect, "image"),
				   tex);

	const uint32_t w = obs_source_get_base_width(afd->source);
	const uint32_t h = obs_source_get_base_height(afd->source);

	while (gs_effect_loop(effect, "Draw"))
		gs_draw_sprite(tex, 0, w, h);

	gs_enable_framebuffer_srgb(previous);
	gs_blend_state_pop();
}
