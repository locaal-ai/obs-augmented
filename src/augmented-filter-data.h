#ifndef FILTERDATA_H
#define FILTERDATA_H

#include <obs.h>

#include <opencv2/core.hpp>

#include <assimp/scene.h>

#include <mutex>
#include <string>

/**
  * @brief The filter_data struct
  *
  * This struct is used to store the base data needed for ORT filters.
  *
*/
struct augmented_filter_data {
	obs_source_t *source;
	gs_texrender_t *texrender;
	gs_stagesurf_t *stagesurface;
	gs_effect_t *effect;
	gs_vertbuffer_t *vbo;

	const aiScene *asset;
	aiMatrix4x4 modelMatrix;
	float fov;
	bool autoRotate;
	int depthFunction;
	int cullMode;
	bool depthTest;
	bool stencilTest;
	bool stencilWrite;
	int stencilFunction;

	cv::Mat inputBGRA;
	cv::Mat outputPreviewBGRA;
	cv::Mat outputMask;

	bool isDisabled;
	bool preview;

	std::mutex inputBGRALock;
	std::mutex outputLock;
	std::mutex modelMutex;
};

#endif /* FILTERDATA_H */
