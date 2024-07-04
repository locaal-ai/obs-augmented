#ifndef ASSET_RENDER_H
#define ASSET_RENDER_H

#include "augmented-filter-data.h"

bool render_asset_3d(augmented_filter_data *afd);
void drawAssimpMesh(const aiMesh *mesh);

#endif /* ASSET_RENDER_H */
