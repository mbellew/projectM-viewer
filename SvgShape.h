//
// Created by matthew on 4/23/18.
//

#ifndef PROJECTM_SVGSHAPE_H
#define PROJECTM_SVGSHAPE_H


struct svg_op
{
    char type;
    float data[6];
};
#define M(x,y) {'M',(float)x,(float)y}
#define l(x,y) {'l',(float)x,(float)y}
#define L(x,y) {'L',(float)x,(float)y}
#define c(x0,y0,x1,y1,x,y) {'c',(float)x0,(float)y0,(float)x1,(float)y1,(float)x,(float)y}
#define C(x0,y0,x1,y1,x,y) {'C',(float)x0,(float)y0,(float)x1,(float)y1,(float)x,(float)y}
#define Z() {'Z'}


class SvgShape : public Shape
{
public:
    SvgShape();
    virtual void Draw(RenderContext &context);
    virtual int svg_length() = 0;
    virtual struct svg_op *svg_data() = 0;
    virtual void release(struct svg_op *) = 0;  // noop if static, free() if dyamically allocated
};


#endif //PROJECTM_SVGSHAPE_H
