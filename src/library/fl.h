#ifndef FL_H_
#define FL_H_

#include "./sv.h"

typedef struct {
    String_View file_path;
    int line_number;
} File_Location;

File_Location file_location(String_View file_path, int line_number);

#define FL_Fmt SV_Fmt":%d"
#define FL_Arg(location) SV_Arg((location).file_path), (location).line_number

#endif // FL_H_
