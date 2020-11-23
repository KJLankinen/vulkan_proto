#include "renderer.h"
int main(int argc, char **argv) {
    if (argc < 2) {
        const char *defaultFilename = "input.json";
        printf("Usage:\n\"%s %s\", \nwhere %s is the file "
               "containing the inputs to the program.\n",
               argv[0], defaultFilename, defaultFilename);
        return 1;
    }
    vulkan_proto::Renderer renderer;
    renderer.run(argv[1]);
    return 0;
}
