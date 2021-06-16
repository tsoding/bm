#include "./fl.h"

File_Location file_location(String_View file_path, int line_number)
{
    return (File_Location) {
        .file_path = file_path,
        .line_number = line_number,
    };
}
