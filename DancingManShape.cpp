//
// Created by matthew on 5/6/18.
//

#include "DancingManShape.h"


DancingManShape::DancingManShape()
{
    // TODO default skeleton
    memset(&this->skeleton, 0, sizeof(this->skeleton));
}


void DancingManShape::update(struct Skeleton &skel)
{
    // TODO copy constructor?
    memcpy(&this->skeleton, &skel, sizeof(this->skeleton));
}


int DancingManShape::svg_length()
{
    return 8;
}

struct svg_op *DancingManShape::svg_data()
{
    Skeleton &skel = this->skeleton;
    svg_op svg[8] =
    {
        M(skel.left_foot[0], skel.left_foot[1]),
        L(skel.left_hip[0],  skel.left_hip[1]),
        L(skel.left_shoulder[0],  skel.left_shoulder[1]),
        L(skel.left_wrist[0],  skel.left_wrist[1]),
        M(skel.right_foot[0], skel.right_foot[1]),
        L(skel.right_hip[0],  skel.right_hip[1]),
        L(skel.right_shoulder[0],  skel.right_shoulder[1]),
        L(skel.right_wrist[0],  skel.right_wrist[1])
    };
    svg_op *p = new svg_op[8];
    memcpy(p,svg,8*sizeof(svg_op));
    return p;
}

void DancingManShape::release(struct svg_op *data)
{
    delete[] data;
}
