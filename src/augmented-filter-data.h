#ifndef FILTERDATA_H
#define FILTERDATA_H

#include <obs.h>
#include <graphics/vec3.h>
#include <graphics/matrix4.h>

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

	std::string assetPath;
	bool flipFaces;
	const aiScene *asset;
	matrix4 modelMatrix;
	matrix4 viewMatrix;
	matrix4 worldViewProjMatrix;
	matrix4 projectionMatrix;
	matrix4 normalMatrix;
	vec3 lightPosition;
	float fov;
	bool autoRotate;
	int depthFunction;
	int cullMode;
	bool depthTest;
	bool stencilTest;
	bool stencilWrite;
	int stencilFunction;
	int stencilDepthFunction;
	int stencilOpSide;
	int stencilOpFail;
	int stencilOpDepthFail;
	int stencilOpPass;
	int stencilClear;
	int clearMode;
	float depthClear;
	float depthFactor;
	float depthBias;
	int source_width = 0;
	int source_height = 0;

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
