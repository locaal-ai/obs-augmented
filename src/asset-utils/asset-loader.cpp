
#include <assimp/cimport.h>
#include <assimp/cexport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <obs.h>

#include "plugin-support.h"

// Function to calculate the bounding box of the scene
void calculateBoundingBox(const aiScene* scene, aiVector3D& min, aiVector3D& max)
{
    min = aiVector3D(1e10f, 1e10f, 1e10f);
    max = aiVector3D(-1e10f, -1e10f, -1e10f);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
        {
            const aiVector3D& vertex = mesh->mVertices[j];
            min.x = std::min(min.x, vertex.x);
            min.y = std::min(min.y, vertex.y);
            min.z = std::min(min.z, vertex.z);
            max.x = std::max(max.x, vertex.x);
            max.y = std::max(max.y, vertex.y);
            max.z = std::max(max.z, vertex.z);
        }
    }

	obs_log(LOG_INFO, "Bounding box min: (%f, %f, %f)", min.x, min.y, min.z);
	obs_log(LOG_INFO, "Bounding box max: (%f, %f, %f)", max.x, max.y, max.z);
}

// Function to rescale the scene to fit within a normalized size
void rescaleScene(aiScene* scene)
{
    aiVector3D min, max;
    calculateBoundingBox(scene, min, max);

    // Calculate the size of the bounding box
    aiVector3D size = max - min;
    float maxDimension = std::max(size.x, std::max(size.y, size.z));

    // Calculate the scaling factor to normalize the size
    float scaleFactor = 1.0f / maxDimension;

    // Apply the scaling transformation to all vertices
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
        {
            mesh->mVertices[j] *= scaleFactor;
        }
    }

	calculateBoundingBox(scene, min, max);
}

void adjustVertexOrder(aiMesh* mesh)
{
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace& face = mesh->mFaces[i];
        if (face.mNumIndices == 3)
        {
            // Swap the order of the second and third vertices
            std::swap(face.mIndices[1], face.mIndices[2]);
        }
    }
}

const aiScene *load_asset(const char *path)
{
	// Load asset from path using Assimp
	const aiScene *scene_in =
		aiImportFile(path, aiProcessPreset_TargetRealtime_Quality);

	// create a copy of the scene
	aiScene *scene;
	aiCopyScene(scene_in, &scene);

	// Rescale the scene to fit within a normalized size
	rescaleScene(scene);

	// adjust the vertex order
	// for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	// {
	// 	adjustVertexOrder(scene->mMeshes[i]);
	// }

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
