//
// Created by matthew on 4/23/18.
//

#ifndef PROJECTM_BURNINGMANSHAPE_H
#define PROJECTM_BURNINGMANSHAPE_H

#include "SvgShape.h"

class BurningManShape: public SvgShape
{
public:
    BurningManShape();
    virtual int svg_length();
    virtual struct svg_op *svg_data();
    virtual void release(struct svg_op *);  // noop if static, free() if dyamically allocated
};


#endif //PROJECTM_BURNINGMANSHAPE_H
