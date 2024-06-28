#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MT_ obs_module_text

void augmented_filter_activate(void *data);
void *augmented_filter_create(obs_data_t *settings, obs_source_t *filter);
void augmented_filter_update(void *data, obs_data_t *s);
void augmented_filter_destroy(void *data);
const char *augmented_filter_name(void *unused);
void augmented_filter_deactivate(void *data);
void augmented_filter_defaults(obs_data_t *s);
obs_properties_t *augmented_filter_properties(void *data);
void augmented_filter_remove(void *data, obs_source_t *source);
void augmented_filter_show(void *data);
void augmented_filter_hide(void *data);
void augmented_filter_video_tick(void *data, float seconds);
void augmented_filter_video_render(void *data, gs_effect_t *_effect);

const char *const PLUGIN_INFO_TEMPLATE =
	"<a href=\"https://github.com/occ-ai/obs-augmented/\">Augmented</a> (%1) by "
	"<a href=\"https://github.com/occ-ai\">OCC AI</a> ❤️ "
	"<a href=\"https://www.patreon.com/RoyShilkrot\">Support & Follow</a>";

#ifdef __cplusplus
}
#endif
