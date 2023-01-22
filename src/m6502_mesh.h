#ifndef M6502_MESH_H
#define M6502_MESH_H

#include <stdint.h>

typedef struct seg_vertex_t
{
	uint16_t x, y;
	uint16_t nodeID;
} seg_vertex_t;

extern seg_vertex_t seg_vertices_0[19128];
extern seg_vertex_t seg_vertices_1[75657];
extern seg_vertex_t seg_vertices_2[315];
extern seg_vertex_t seg_vertices_3[35697];
extern seg_vertex_t seg_vertices_4[18195];
extern seg_vertex_t seg_vertices_5[101094];

#endif // M6502_MESH_H
