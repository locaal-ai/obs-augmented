#include "render-utils.h"
#include "plugin-support.h"
#include "augmented-filter-data.h"

#include <map>

#include <obs.h>

#include <graphics/graphics.h>

#define GLM_FORCE_LEFT_HANDED 1
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

matrix4 glm_to_obs(const glm::mat4 &m)
{
	matrix4 result;
	for (int i = 0; i < 4; i++) {
		result.x.ptr[i] = m[i][0];
		result.y.ptr[i] = m[i][1];
		result.z.ptr[i] = m[i][2];
		result.t.ptr[i] = m[i][3];
	}
	return result;
}

glm::mat4 obs_to_glm(const matrix4 &m)
{
	glm::mat4 result;
	for (int i = 0; i < 4; i++) {
		result[i][0] = m.x.ptr[i];
		result[i][1] = m.y.ptr[i];
		result[i][2] = m.z.ptr[i];
		result[i][3] = m.t.ptr[i];
	}
	return result;
}

matrix4 createModelMatrix(vec3 position, vec3 rotation, vec3 scale)
{
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix =
		glm::scale(modelMatrix, glm::vec3{scale.x, scale.y, scale.z});
	modelMatrix = glm::translate(
		modelMatrix, glm::vec3{position.x, position.y, position.z});
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z),
				  glm::vec3{0.0f, 0.0f, 1.0f});
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y),
				  glm::vec3{0.0f, 1.0f, 0.0f});
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x),
				  glm::vec3{1.0f, 0.0f, 0.0f});
	return glm_to_obs(modelMatrix);
}

matrix4 createViewMatrix(vec3 eye, vec3 target, vec3 up)
{
	glm::mat4 lookAtMatrix = glm::lookAt(glm::vec3{eye.x, eye.y, eye.z},
					     {target.x, target.y, target.z},
					     {up.x, up.y, up.z});
	return glm_to_obs(lookAtMatrix);
}

matrix4 calcNormalMatrix(const matrix4 &modelMatrix)
{
	glm::mat4 model = obs_to_glm(modelMatrix);
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	return glm_to_obs(normalMatrix);
}

matrix4 createProjectionMatrix(float fovY, float aspectRatio, float nearZ,
			       float farZ)
{
	glm::mat4 proj = glm::perspective(fovY, aspectRatio, nearZ, farZ);
	return glm_to_obs(proj);
}

// Function to draw a single triangle
void drawTriangle(const aiVector3D *vertices, const aiVector3D *normals)
{
	for (int i : {0, 2, 1}) {
		if (normals) {
			gs_normal3f(normals[i].x, normals[i].y, normals[i].z);
		}
		gs_vertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
		gs_color(0xFFFFFFFF);
	}
}

float getMinMeshDistanceToCamera(const aiMesh *mesh,
				 const glm::mat4 &modelMatrix)
{
	float minDistance = std::numeric_limits<float>::max();
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		const aiFace &face = mesh->mFaces[i];
		// find the distance of the triangle from the camera
		const unsigned int index = face.mIndices[0];
		const aiVector3D vertex = mesh->mVertices[index];
		// apply the model matrix to the vertex
		const glm::vec4 vertex4 =
			modelMatrix *
			glm::vec4{vertex.x, vertex.y, vertex.z, 1.0f};
		const float distance = glm::distance(
			glm::vec3{vertex4.x, vertex4.y, vertex4.z},
			glm::vec3{0.0f, 0.0f, 0.0f});
		minDistance = std::min(minDistance, distance);
	}
	return minDistance;
}

// Function to draw an Assimp mesh
void drawAssimpMesh(const aiMesh *mesh, const glm::mat4 &modelMatrix)
{
	std::multimap<float, unsigned int, std::less<float>> smap;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		const aiFace &face = mesh->mFaces[i];
		// find the distance of the triangle from the camera
		const unsigned int index = face.mIndices[0];
		const aiVector3D vertex = mesh->mVertices[index];
		// apply the model matrix to the vertex
		const glm::vec4 vertex4 =
			modelMatrix *
			glm::vec4{vertex.x, vertex.y, vertex.z, 1.0f};
		const float distance = glm::distance(
			glm::vec3{vertex4.x, vertex4.y, vertex4.z},
			glm::vec3{0.0f, 0.0f, 0.0f});
		smap.insert(std::make_pair(distance, i));
	}
	for (std::multimap<float, unsigned int,
			   std::less<float>>::const_iterator i = smap.begin();
	     i != smap.end(); ++i) {
		const aiFace &face = mesh->mFaces[i->second];
		if (face.mNumIndices == 3) {
			aiVector3D vertices[3];
			aiVector3D normals[3];

			for (int j : {0, 1, 2}) {
				const unsigned int index = face.mIndices[j];
				vertices[j] = mesh->mVertices[index];
				if (mesh->HasNormals()) {
					normals[j] = mesh->mNormals[index];
				}
			}

			drawTriangle(vertices,
				     mesh->HasNormals() ? normals : nullptr);
		}
	}
}

void drawAssimpAsset(const aiScene *asset, const glm::mat4 &modelMatrix)
{
	// sort meshes by distance to camera
	std::multimap<float, const aiMesh *, std::greater<float>> smap;
	for (unsigned int i = 0; i < asset->mNumMeshes; ++i) {
		const aiMesh *mesh = asset->mMeshes[i];
		const float minDistance =
			getMinMeshDistanceToCamera(mesh, modelMatrix);
		smap.insert(std::make_pair(minDistance, mesh));
	}
	for (std::multimap<float, const aiMesh *,
			   std::greater<float>>::const_iterator i =
		     smap.begin();
	     i != smap.end(); ++i) {
		drawAssimpMesh(i->second, modelMatrix);
	}
}
