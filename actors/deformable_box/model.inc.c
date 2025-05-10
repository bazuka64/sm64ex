  Vtx deformable_box_vertex[] = {
    {{{  -100,   -100,   -100}, 0, {   0,    0}, {0x00, 0x00, 0x00, 0x00}}},
    {{{  -100,   -100,   100}, 0, {   0,    0}, {0x00, 0x00, 0x00, 0x00}}},
    {{{  -100,   100,   -100}, 0, {   0,    0}, {0x00, 0x00, 0x00, 0x00}}},
    {{{  -100,   100,   100}, 0, {   0,    0}, {0x00, 0x00, 0x00, 0x00}}},
    {{{  100,   -100,   -100}, 0, {   0,    0}, {0x00, 0x00, 0x00, 0x00}}},
    {{{  100,   -100,   100}, 0, {   0,    0}, {0x00, 0x00, 0x00, 0x00}}},
    {{{  100,   100,   -100}, 0, {   0,    0}, {0x00, 0x00, 0x00, 0x00}}},
    {{{  100,   100,   100}, 0, {   0,    0}, {0x00, 0x00, 0x00, 0x00}}},
};

const Gfx deformable_box_dl[] = {
    gsSPVertex(deformable_box_vertex, 8, 0),
    
    gsSP1Triangle(0, 1, 2, 0),
    gsSP1Triangle(1, 3, 2, 0),
    gsSP1Triangle(4, 6, 5, 0),
    gsSP1Triangle(5, 6, 7, 0),
    gsSP1Triangle(0, 5, 1, 0),
    gsSP1Triangle(0, 4, 5, 0),
    gsSP1Triangle(2, 3, 7, 0),
    gsSP1Triangle(2, 7, 6, 0),
    gsSP1Triangle(0, 2, 6, 0),
    gsSP1Triangle(0, 6, 4, 0),
    gsSP1Triangle(1, 7, 3, 0),
    gsSP1Triangle(1, 5, 7, 0),
    
    gsSPEndDisplayList(),
};