#include <EmbedFS.h>
#include "assets_embed.h"

EmbedFSClass EmbedFS;

void setup()
{
    EmbedFS.begin(assets_file_names, assets_file_data, assets_file_sizes, assets_file_count);
}

void loop()
{
}
