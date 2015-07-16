#ifndef OBJ_H
#define OBJ_H

int obj_parse(const char *file_name, float** points_ptr, float** normals_ptr, int *face_count_ptr);
void obj_write(const char *file_name, int component_count, int point_count, float* points);

#endif // OBJ_H
