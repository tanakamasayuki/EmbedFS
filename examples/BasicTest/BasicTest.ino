#include <EmbedFS.h>
#include "assets_embed.h"

void setup()
{
    Serial.begin(115200);
    delay(1000);

    if (!EmbedFS.begin(assets_file_names, assets_file_data, assets_file_sizes, assets_file_count))
    {
        Serial.println("EmbedFS Mount Failed");
        return;
    }
}

void loop()
{
    // File listing
    Serial.println("Embedded files:");
    for (int i = 0; i < assets_file_count; i++)
    {
        File file = EmbedFS.open(assets_file_names[i]);
        if (file)
        {
            Serial.printf("- path: %s name: %s size: %u bytes\n", file.path(), file.name(), (unsigned)file.size());
            while (file.available())
            {
                Serial.write(file.read());
            }
            file.close();
            Serial.println();
        }
    }

    // directory listing
    Serial.println("Directory listing of /directory:");
    File dir = EmbedFS.open("/directory");
    if (dir && dir.isDirectory())
    {
        dir.rewindDirectory();
        File entry;
        while ((entry = dir.openNextFile()))
        {
            Serial.printf("- path: %s name: %s size: %u bytes\n", entry.path(), entry.name(), (unsigned)entry.size());
            entry.close();
        }
        dir.close();
    }

    delay(10000);
}
