#include "augmented-filter.h"

struct obs_source_info augmented_filter_info = {
	.id = "augmented_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = augmented_filter_name,
	.create = augmented_filter_create,
	.destroy = augmented_filter_destroy,
	.get_defaults = augmented_filter_defaults,
	.get_properties = augmented_filter_properties,
	.update = augmented_filter_update,
	.activate = augmented_filter_activate,
	.deactivate = augmented_filter_deactivate,
	.filter_remove = augmented_filter_remove,
	.show = augmented_filter_show,
	.hide = augmented_filter_hide,
	.video_tick = augmented_filter_video_tick,
	.video_render = augmented_filter_video_render,
};
