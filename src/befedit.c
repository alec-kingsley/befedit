#include "editor.h"

#define FILENAME "befedit.c"

int main(int argc, char **argv) {
    Editor *editor = editor_create();
    int i;
    Buffer *buffer;
    if (!editor) return 1;
    for (i = 1; i < argc; i++) {
        buffer = buffer_create(argv[i]);
        if (!buffer) {
            editor_destroy(editor);
            return 1;
        }
        editor_add_buffer(editor, buffer);
    }
    editor_run(editor);
    editor_destroy(editor);
    return 0;
}
