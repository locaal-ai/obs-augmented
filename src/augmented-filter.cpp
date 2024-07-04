#include <obs.h>
#include <assimp/cimport.h>

#include "augmented-filter.h"
#include "augmented-filter-data.h"
#include "plugin-support.h"
#include "asset-utils/asset-loader.h"
#include "asset-utils/asset-render.h"

void augmented_filter_activate(void *data) {}

void update_from_settings(augmented_filter_data *afd, obs_data_t *s)
{

	// get the rotation value
	float rotation = (float)obs_data_get_double(s, "rotation");
	// conver to radians
	rotation = (rotation * M_PI) / 180.0f;
	// get the x position value
	float x = (float)obs_data_get_double(s, "x");
	// get the y position value
	float y = (float)obs_data_get_double(s, "y");
	// get the z position value
	float z = (float)obs_data_get_double(s, "z");
	// get the scale value
	float scale = (float)obs_data_get_double(s, "scale");

	afd->fov = (float)obs_data_get_double(s, "fov") * (M_PI / 180.0f);
	afd->autoRotate = obs_data_get_bool(s, "auto_rotate");
	afd->depthFunction = (int)obs_data_get_int(s, "depth_function");
	afd->cullMode = (int)obs_data_get_int(s, "cull_mode");
	afd->depthTest = obs_data_get_bool(s, "depth_test");
	afd->stencilTest = obs_data_get_bool(s, "stencil_test");
	afd->stencilWrite = obs_data_get_bool(s, "stencil_write");
	afd->stencilFunction = (int)obs_data_get_int(s, "stencil_function");

	// update the model matrix
	afd->modelMatrix = aiMatrix4x4();
	afd->modelMatrix.a1 = scale;
	afd->modelMatrix.b2 = scale;
	afd->modelMatrix.c3 = scale;
	afd->modelMatrix.a4 = x;
	afd->modelMatrix.b4 = y;
	afd->modelMatrix.c4 = z;

	// update the rotation matrix
	aiMatrix4x4 rotationMatrix;
	rotationMatrix.a1 = cos(rotation);
	rotationMatrix.a3 = sin(rotation);
	rotationMatrix.c1 = -sin(rotation);
	rotationMatrix.c3 = cos(rotation);

	afd->modelMatrix = afd->modelMatrix * rotationMatrix;
}

void *augmented_filter_create(obs_data_t *settings, obs_source_t *filter)
{
	obs_log(LOG_INFO, "Augmented filter created");
	void *data = bmalloc(sizeof(struct augmented_filter_data));
	struct augmented_filter_data *afd = new (data) augmented_filter_data();

	afd->source = filter;
	afd->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);

	// get file path of the built in asset
	const char *assetPath = obs_module_file("assets/ghost.dae");
	afd->asset = load_asset(assetPath);

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

	gs_render_start(true);
	for (unsigned int i = 0; i < afd->asset->mNumMeshes; ++i) {
		drawAssimpMesh(afd->asset->mMeshes[i]);
	}
	afd->vbo = gs_render_save();

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
		gs_vertexbuffer_destroy(afd->vbo);
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
	return "Augmented Filter";
}

void augmented_filter_deactivate(void *data) {}

void augmented_filter_defaults(obs_data_t *s)
{
	obs_data_set_default_double(s, "rotation", 0.0);
	obs_data_set_default_double(s, "x", 0.0);
	obs_data_set_default_double(s, "y", 0.0);
	obs_data_set_default_double(s, "z", 0.0);
	obs_data_set_default_double(s, "scale", 1.0);
	obs_data_set_default_double(s, "fov", 60.0);
	obs_data_set_default_bool(s, "auto_rotate", true);
	obs_data_set_default_int(s, "depth_function", GS_LEQUAL);
	obs_data_set_default_int(s, "cull_mode", GS_BACK);
	obs_data_set_default_bool(s, "depth_test", true);
	obs_data_set_default_bool(s, "stencil_test", false);
	obs_data_set_default_bool(s, "stencil_write", false);
	obs_data_set_default_int(s, "stencil_function", GS_STENCIL_BOTH);
}

obs_properties_t *augmented_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

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

	return props;
}

void augmented_filter_remove(void *data, obs_source_t *source) {}

void augmented_filter_show(void *data) {}

void augmented_filter_hide(void *data) {}

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
	struct augmented_filter_data *afd =
		reinterpret_cast<augmented_filter_data *>(data);

	if (afd->isDisabled) {
		return;
	}

	if (!afd->asset) {
		return;
	}

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
