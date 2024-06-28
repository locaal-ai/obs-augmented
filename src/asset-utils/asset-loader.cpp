
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <obs.h>

#include "plugin-support.h"

const aiScene *load_asset(const char *path)
{
	// Load asset from path using Assimp
	const aiScene *scene =
		aiImportFile(path, aiProcessPreset_TargetRealtime_MaxQuality);

	if (!scene) {
		obs_log(LOG_ERROR, "Failed to load asset: %s\n",
			aiGetErrorString());
		return nullptr;
	}

	// print asset info
	obs_log(LOG_INFO, "Asset: %s", scene->mName.C_Str());
	obs_log(LOG_INFO, "Number of meshes: %d", scene->mNumMeshes);
	obs_log(LOG_INFO, "Number of materials: %d", scene->mNumMaterials);
	obs_log(LOG_INFO, "Number of textures: %d", scene->mNumTextures);

	// print mesh info
	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		aiMesh *mesh = scene->mMeshes[i];
		obs_log(LOG_INFO, "Mesh %d: %s", i, mesh->mName.C_Str());
		obs_log(LOG_INFO, "Number of vertices: %d", mesh->mNumVertices);
		obs_log(LOG_INFO, "Number of faces: %d", mesh->mNumFaces);
	}

	// print material info
	for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial *material = scene->mMaterials[i];
		obs_log(LOG_INFO, "Material %d: %s", i,
			material->GetName().C_Str());
	}

	// print texture info
	for (unsigned int i = 0; i < scene->mNumTextures; i++) {
		aiTexture *texture = scene->mTextures[i];
		obs_log(LOG_INFO, "Texture %d: %s", i,
			texture->mFilename.C_Str());
	}

	return scene;
}
