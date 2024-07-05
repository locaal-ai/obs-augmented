#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include <assimp/scene.h>
#include <graphics/vec3.h>
#include <graphics/matrix4.h>
#include <glm/mat4x4.hpp>

glm::mat4 obs_to_glm(const matrix4 &m);
matrix4 createModelMatrix(vec3 position, vec3 rotation, vec3 scale);
matrix4 createViewMatrix(vec3 eye, vec3 target, vec3 up);
matrix4 calcNormalMatrix(const matrix4 &modelMatrix);
matrix4 createProjectionMatrix(float fovY, float aspectRatio, float nearZ,
			       float farZ);
void drawTriangle(const aiVector3D *vertices, const aiVector3D *normals);
void drawAssimpAsset(const aiScene *asset, const glm::mat4 &modelMatrix);

#endif /* RENDER_UTILS_H */
