#ifndef PATH_H_
#define PATH_H_

#include "./sv.h"
#include "./arena.h"

String_View file_name_of_path(const char *begin);
String_View path_join(Arena *arena, String_View base, String_View file_path);
bool path_file_exist(const char *file_path);
char *shift(int *argc, char ***argv);

#endif // PATH_H_
