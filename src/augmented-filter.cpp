#include <obs.h>
#include <assimp/cimport.h>

#include "augmented-filter.h"
#include "augmented-filter-data.h"
#include "plugin-support.h"
#include "asset-utils/asset-loader.h"
#include "asset-utils/asset-render.h"

void augmented_filter_activate(void *data) {}

void *augmented_filter_create(obs_data_t *settings, obs_source_t *filter)
{
	obs_log(LOG_INFO, "Augmented filter created");
	void *data = bmalloc(sizeof(struct augmented_filter_data));
	struct augmented_filter_data *afd = new (data) augmented_filter_data();

	afd->source = filter;
	afd->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);

	// get file path of the built in asset
	const char *assetPath = obs_module_file("assets/crown.dae");
	afd->asset = load_asset(assetPath);

	return afd;
}

void augmented_filter_update(void *data, obs_data_t *s) {}

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

void augmented_filter_defaults(obs_data_t *s) {}

obs_properties_t *augmented_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	return props;
}

void augmented_filter_remove(void *data, obs_source_t *source) {}

void augmented_filter_show(void *data) {}

void augmented_filter_hide(void *data) {}

void augmented_filter_video_tick(void *data, float seconds) {}

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

	render_asset_3d(afd, afd->asset);
}
