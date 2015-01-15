// Wavefront OBJ format parser

#include <stdio.h>
#include <stdlib.h>

#include "obj.h"

int obj_parse(const char *file_name, float** points_ptr, float** normals_ptr, int *face_count_ptr) {
    FILE* fp = fopen(file_name, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: could not find file %s\n", file_name);
        return 0;
    }

    // First count points in the file
    int vp_count = 0;
    int vn_count = 0;
    int face_count = 0;
    char line[1024];
    while (fgets(line, 1024, fp)) {
        if (line[0] == 'v') { // Vertex data
            if (line[1] == ' ') { // Vertex point
                vp_count++;
            } else if (line[1] == 'n') { // Normal
                vn_count++;
            }
        } else if (line[0] == 'f') { // Face
            face_count++;
        }
    }
    rewind(fp);

    float* vp_array = calloc(vp_count * 3, sizeof(float));
    float* vn_array = calloc(vn_count * 3, sizeof(float));
    int vp_index = 0;
    int vn_index = 0;
    int pt_index = 0;
    float* points = calloc(3 * face_count * 3, sizeof(float));
    float* normals = calloc(3 * face_count * 3, sizeof(float));
    *points_ptr = points;
    *normals_ptr = normals;
    *face_count_ptr = face_count;

    while (fgets(line, 1024, fp)) {
        if (line[0] == 'v') { // Vertex data
            if (line[1] == ' ') { // Vertex point
                float x = 0.f, y = 0.f, z = 0.f;
                sscanf(line, "v %f %f %f", &x, &y, &z);
                vp_array[vp_index++] = x;
                vp_array[vp_index++] = y;
                vp_array[vp_index++] = z;
            } else if (line[1] == 'n') { // Normal
                float x = 0.f, y = 0.f, z = 0.f;
                sscanf(line, "vn %f %f %f", &x, &y, &z);
                vn_array[vn_index++] = x;
                vn_array[vn_index++] = y;
                vn_array[vn_index++] = z;
            }
        } else if (line[0] == 'f') { // Face
            int vp[3], vn[3];
            sscanf(line, "f %i//%i %i//%i %i//%i",
            &vp[0], &vn[0], &vp[1], &vn[1], &vp[2], &vn[2]);
            for (int fi = 0; fi < 3; fi++) {
                // Point IDs start at 1
                int vpi = vp[fi] - 1;
                int vni = vn[fi] - 1;
                points[pt_index] = vp_array[vpi * 3];
                points[pt_index + 1] = vp_array[vpi * 3 + 1];
                points[pt_index + 2] = vp_array[vpi * 3 + 2];
                normals[pt_index] = vn_array[vni * 3];
                normals[pt_index + 1] = vn_array[vni * 3 + 1];
                normals[pt_index + 2] = vn_array[vni * 3 + 2];
                pt_index += 3;
            }
        }
    }
    fclose(fp);
    free(vp_array);
    return 1;
}

// int main() {
//     float* points;
//     int face_count = 0;
//     obj_parse("../cube.obj", &points, &face_count);
//     for (int i = 0; i < face_count; i++) {
//         printf("%.1f %.1f %.1f - %.1f %.1f %.1f - %.1f %.1f %.1f\n",
//                points[(i * 9)], points[(i * 9) + 1], points[(i * 9) + 2],
//                points[(i * 9) + 3], points[(i * 9) + 4], points[(i * 9) + 5],
//                points[(i * 9) + 6], points[(i * 9) + 7], points[(i * 9) + 8]);
//     }
//     printf("%d\n", face_count);
// }
