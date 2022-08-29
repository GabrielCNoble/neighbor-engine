#ifndef ED_BRUSH_DEFS_H
#define ED_BRUSH_DEFS_H



//struct ed_bsp_polygon_t
//{
//    struct ed_bsp_polygon_t *next;
//    struct ed_bsp_polygon_t *prev;
//    struct ed_face_polygon_t *face_polygon;
//    struct ds_list_t vertices;
//    uint32_t used;
//    uint32_t model_start;
//    uint32_t model_count;
//    uint32_t index;
//    vec3_t normal;
//    vec3_t point;
//};

//struct ed_bsp_node_t
//{
//    struct ed_bsp_node_t *front;
//    struct ed_bsp_node_t *back;
//    struct ed_bsp_polygon_t *splitter;
//    uint32_t index;
//};

//enum ED_SPLITTER_SIDES
//{
//    ED_SPLITTER_SIDE_FRONT = 0,
//    ED_SPLITTER_SIDE_BACK = 1,
//    ED_SPLITTER_SIDE_STRADDLE = 2,
//    ED_SPLITTER_SIDE_ON_FRONT = 3,
//    ED_SPLITTER_SIDE_ON_BACK = 4,
//};



//struct ed_brush_batch_t
//{
//    struct r_batch_t batch;
//    uint32_t index;
//};
//
//struct ed_poly_edge_t
//{
//    struct ed_poly_edge_t *next;
//    struct ed_poly_edge_t *prev;
//    struct ed_edge_t *edge;
//    struct ed_face_polygon_t *polygon;
//};
//
//struct ed_vert_edge_t
//{
//    struct ed_vert_edge_t *next;
//    struct ed_vert_edge_t *prev;
//    struct ed_edge_t *edge;
//    struct ed_vert_t *vert;
//};
//
//struct ed_vert_t
//{
//    struct ed_vert_edge_t *edges;
//    vec3_t vert;
//    uint32_t index;
//    uint32_t s_index;
//};
//
//struct ed_edge_t
//{
//    uint32_t index;
//    uint32_t s_index;
//    struct ed_edge_t *init_next;
//    struct ed_pickable_t *pickable;
//
//    struct
//    {
//        struct ed_edge_t *next;
//        struct ed_edge_t *prev;
//        struct ed_face_polygon_t *polygon;
//
//    }polygons[2];
//
//    struct ed_vert_edge_t verts[2];
//
//    struct ed_brush_t *brush;
//    uint32_t model_start_flag;
//    uint32_t model_start;
//};
//
//struct ed_face_polygon_t
//{
//    struct ed_edge_t *edges;
//    struct ed_edge_t *last_edge;
//    struct ed_face_polygon_t *next;
//    struct ed_face_polygon_t *prev;
//    struct ed_face_t *face;
//    struct ed_brush_t *brush;
//    uint32_t index;
//    uint32_t s_index;
//
//    uint32_t edge_count;
//
//    vec3_t tangent;
//    vec3_t normal;
//    vec3_t center;
//    vec2_t center_uv;
//};
//
//enum ED_FACE_FLAGS
//{
//    ED_FACE_FLAG_GEOMETRY_MODIFIED = 1,
//    ED_FACE_FLAG_MATERIAL_MODIFIED = 1 << 1,
//};
//
//struct ed_face_t
//{
//    struct ed_face_t *next;
//    struct ed_face_t *prev;
//    struct ed_brush_t *brush;
//    uint32_t index;
//
//    struct ed_pickable_t *pickable;
//
//    struct r_material_t *material;
////    struct ed_brush_batch_t *material;
//    struct ed_face_polygon_t *polygons;
//    struct ed_face_polygon_t *last_polygon;
//    struct ed_bsp_polygon_t *clipped_polygons;
//
//    uint32_t clipped_vert_count;
//    uint32_t clipped_index_count;
//    uint32_t clipped_polygon_count;
//
//    uint32_t flags;
//
//    vec3_t center;
//    vec2_t tex_coords_scale;
//    float tex_coords_rot;
//};
//
//struct ed_vert_transform_t
//{
//    vec3_t translation;
//    vec3_t rotation;
//    uint32_t index;
//};
//
//enum ED_BRUSH_FLAGS
//{
//    ED_BRUSH_FLAG_GEOMETRY_MODIFIED = 1,
//    ED_BRUSH_FLAG_MATERIAL_MODIFIED = 1 << 1,
//};
//
//enum ED_BRUSH_UPDATE_FLAGS
//{
//    ED_BRUSH_UPDATE_FLAG_TRANSFORM_VERTS = 1,
//    ED_BRUSH_UPDATE_FLAG_FACE_POLYGONS = 1 << 1,
//    ED_BRUSH_UPDATE_FLAG_CLIPPED_POLYGONS = 1 << 2,
//    ED_BRUSH_UPDATE_FLAG_UV_COORDS = 1 << 3,
//    ED_BRUSH_UPDATE_FLAG_DRAW_GEOMETRY = 1 << 4,
//};
//
//struct ed_brush_t
//{
//    struct ed_brush_t *next;
//    struct ed_brush_t *prev;
//    struct ed_brush_t *last;
//
//    struct ed_brush_t *main_brush;
//    struct ed_pickable_t *pickable;
//
//    mat3_t orientation;
//    vec3_t position;
//    uint32_t index;
//    struct ed_face_t *faces;
//    struct ed_face_t *last_face;
//    struct ds_slist_t vertices;
//    struct ds_list_t vert_transforms;
//    struct r_model_t *model;
//    struct e_entity_t *entity;
//    uint32_t entity_index;
//
//    uint32_t face_count;
//    uint32_t polygon_count;
//    uint32_t edge_count;
//
//    uint32_t clipped_vert_count;
//    uint32_t clipped_index_count;
//    uint32_t clipped_polygon_count;
//    uint32_t flags;
//    uint32_t update_flags;
//
//    uint32_t modified_index;
//};
//
//struct ed_vert_record_t
//{
//    vec3_t vert;
//    /* used by the deserializer to map between a serialized vertex
//    and the allocated vertex */
//    uint32_t d_index;
//};
//
//struct ed_edge_record_t
//{
//    uint64_t polygons[2];
//    uint64_t vertices[2];
//    /* used by the deserializer to map between a serialized edge
//    and the allocated edge */
//    uint32_t d_index;
//};
//
////struct ed_polygon_edge_record_t
////{
////    size_t edge;
////};
//
//struct ed_polygon_record_t
//{
//    uint64_t edge_count;
//    uint64_t edges[];
//};
//
//struct ed_face_record_t
//{
//    uint64_t polygon_start;
//    uint64_t polygon_count;
//
//    vec2_t uv_scale;
//    float uv_rot;
//
//    char material[32];
//};
//
//struct ed_brush_record_t
//{
//    vec3_t position;
//    mat3_t orientation;
//
//    uint64_t record_size;
//
//    uint64_t face_start;
//    uint64_t face_count;
//
//    uint64_t edge_start;
//    uint64_t edge_count;
//
//    uint64_t vert_start;
//    uint64_t vert_count;
//
//    uint32_t uuid;
//};
//
//struct ed_brush_section_t
//{
//    uint64_t brush_record_start;
//    uint64_t brush_record_count;
//
//    size_t reserved[32];
//};

#endif




