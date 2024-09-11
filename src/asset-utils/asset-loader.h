#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <assimp/scene.h>

const aiScene *load_asset(const char *path, const bool flipFaces);

#endif /* ASSET_LOADER_H */
