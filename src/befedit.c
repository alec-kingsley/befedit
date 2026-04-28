#include "editor.h"
#include <time.h>

#define FILENAME "befedit.c"

int main(int argc, char **argv) {
    Editor *editor = editor_create();
    int i;
    srand(time(NULL));
    if (!editor) return 1;
    for (i = 1; i < argc; i++) {
        editor_open(editor, argv[i]);
    }
    editor_run(editor);
    editor_destroy(editor);
    return 0;
}
