#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "build_tool/nob.h"

#define CC "clang"
#define SRC_FOLDER "src/"
#define BUILD_FOLDER "build/"
#define INCLUDE_FOLDER "deps/include"
#define LIB_FOLDER "deps/lib"
#define EXEC_NAME "Renderer"
// DEV, REL or DEB build
#define REL 

const char *flags[] = {
#ifdef DEB
    "-fsanitize=address,undefined",
    "-fsanitize-recover=address",
    "-fno-omit-frame-pointer",
    "-g",
    "-O0"
#endif
#ifdef DEV
    "-g",
    "-O2",
    "-fno-omit-frame-pointer",
    "-DNDEBUG"
#endif
#ifdef REL
    "-g",
    "-O3",
    "-ffast-math",
    "-DNDEBUG"
#endif
};

const char *src[] = {
    "main.c"
};

const char *libs[] = {
    "cpstd",
    "cplt",
    "m"
};

int main(int argc, char *argv[]) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) {
        return 1;
    }
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, CC);
    for (int i = 0; i < sizeof(flags) / sizeof(*flags); ++i) {
        nob_cmd_append(&cmd, flags[i]);
    }
    char src_paths[sizeof(src) / sizeof(*src)][1024];
    {
        int n = sizeof(src) / sizeof(*src);
        memset(src_paths, 0, n * 1024);
        for (int i = 0; i < n; ++i) {
            strcat(src_paths[i], SRC_FOLDER);
            strcat(src_paths[i], src[i]);
            nob_cmd_append(&cmd, src_paths[i]);
        }
    }
    nob_cmd_append(&cmd, "-o", BUILD_FOLDER EXEC_NAME);
    nob_cmd_append(&cmd, "-I" INCLUDE_FOLDER);
    nob_cmd_append(&cmd, "-L" LIB_FOLDER);
    char lib_paths[sizeof(libs) / sizeof(*libs)][1024];
    {
        int n = sizeof(libs) / sizeof(*libs);
        memset(lib_paths, 0, n * 1024);
        for (int i = 0; i < n; ++i) {
            strcat(lib_paths[i], "-l");
            strcat(lib_paths[i], libs[i]);
            nob_cmd_append(&cmd, lib_paths[i]);
        }
    }
    if (!nob_cmd_run(&cmd)) {
        return 1;
    }
}
