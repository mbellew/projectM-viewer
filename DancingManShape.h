//
// Created by matthew on 5/6/18.
//

#ifndef PROJECTM_DANCINGMANSHAPE_H
#define PROJECTM_DANCINGMANSHAPE_H

#include "pmSDL.hpp"
#include "SvgShape.h"

typedef float floatPair[2];

struct Skeleton
{
    floatPair left_foot;
    floatPair left_knee;
    floatPair left_hip;
    floatPair left_shoulder;
    floatPair left_elbow;
    floatPair left_wrist;

    floatPair right_foot;
    floatPair right_knee;
    floatPair right_hip;
    floatPair right_shoulder;
    floatPair right_elbow;
    floatPair right_wrist;

    void scale_by(float s, float x, float y)
    {
        float *fp = left_foot;
        for (int i=0 ; i<12 ; i++, fp += 2)
        {
            fp[0] = fp[0] * s + x;
            fp[1] = 1.0 - (fp[1] * s + y);
        }
    }
};


class DancingManShape : public SvgShape
{
public:
    DancingManShape();

    void update(struct Skeleton &skel);

    virtual int svg_length();
    virtual struct svg_op *svg_data();
    virtual void release(struct svg_op *);  // noop if static, free() if dyamically allocated

private:
    Skeleton skeleton;
};

#endif //PROJECTM_DANCINGMANSHAPE_H
