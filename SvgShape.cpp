//
// Created by matthew on 4/23/18.
//

#include "pmSDL.hpp"

const float PI = 3.1415927f;
const float TAU= (2.0 * PI);


SvgShape::SvgShape()
{
}


void drawLineCap(float *xy, float *color, float diameter)
{
    return;
    float x = xy[0], y = xy[1], radius=0.005; //diameter/2.0;
    const int edges = 20;
    float points[edges+2][2];
    float colors[edges+2][4];
    int p = 0;

    points[p][0] = x;
    points[p][1] = y;
    colors[p][0] = color[0]; colors[p][1] = color[1]; colors[p][2] = color[2]; colors[p][3] = color[3];
    p++;
    for(int i = 0; i <= edges; i++)
    {
        points[p][0] = x + radius * cos(i * TAU / edges);
        points[p][1] = y + radius * sin(i * TAU / edges);
        colors[p][0] = color[0]; colors[p][1] = color[1]; colors[p][2] = color[2]; colors[p][3] = color[3];
        p++;
    }
    glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2,GL_FLOAT,0,points);
    glColorPointer(4,GL_FLOAT,0,colors);
    glDrawArrays(GL_TRIANGLE_FAN,0,edges+2);
}


int cubic(int count, float (*points)[2], float *start, float *c1, float *c2, float *end)
{
    int p = 0;
    for (int i=1 ; i<=count ; i++)
    {
        float b = (float)i / (float)count;
        float a = 1.0-b;
        float p1[2] = { start[0] * a + c1[0]  * b, start[1] * a + c1[1]  * b };
        float p2[2] = { c2[0]    * a + end[0] * b,    c2[1] * a + end[1] * b };
        points[p][0] = p1[0] * a + p2[0] * b;
        points[p][1] = p1[1] * a + p2[1] * b;
        p++;
    }
    return count;
}


// TODO: just pass in the normals?
void drawPath2(float (*points)[2], int count, float thickness, bool closed)
{
    float T = thickness/2.0;
    float (*normals)[2] = new float[count][2];

    float *prevPoint = (float *)(points + count-1);
    for (int p=0 ; p<count ; p++)
    {
        float *point = (float *)(points + p);
        float *nextPoint = (float *)(points + ((p+1)%count));
        float dx = nextPoint[0] - prevPoint[0];
        float dy = nextPoint[1] - prevPoint[1];
        float l = sqrt(dx*dx+dy*dy);
        normals[p][0] = dy / -l;
        normals[p][1] = dx / l;
        prevPoint = point;
    }

    float (*vert)[2] = new float[2*(count+1)][2];
    int v = 0;

    for (int p=0 ; p<count ; p++)
    {
        float *point = (float *)(points + p);
        float *normal = (float *)(normals + p);
        vert[v][0] = point[0] - T*normal[0];
        vert[v][1] = point[1] - T*normal[1];
        v++;
        vert[v][0] = point[0] + T*normal[0];
        vert[v][1] = point[1] + T*normal[1];
        v++;
    }
    if (closed)
    {
        vert[v][0] = vert[0][0];
        vert[v][1] = vert[0][1];
        v++;
        vert[v][0] = vert[1][0];
        vert[v][1] = vert[1][1];
        v++;
    }
    delete[] normals;

    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vert);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, v);

    delete[] vert;
}


inline void normalizeV(float *V)
{
    float l = sqrt(V[0]*V[0]+V[1]*V[1]);
    if (l!=0)
    {
        V[0] /= l;
        V[1] /= l;
    }
}

void drawPath(float (*points)[2], int count, float thickness, bool closed)
{
    float T = thickness/2.0;
    float (*normals)[2] = new float[count][2];

    int start = closed ? 0 : 1;
    int end = closed ? count : count-1;
    for (int p=start ; p<end ; p++)
    {
        float *point = (float *)(points + p);
        float *prevPoint = (float *)(points + (p+count-1)%count);
        float *nextPoint = (float *)(points + ((p+1)%count));
        float u[2] = {prevPoint[0]-point[0], prevPoint[1]-point[1]};
        normalizeV(u);
        float v[2] = {nextPoint[0]-point[0], nextPoint[1]-point[1]};
        normalizeV(v);
        float dot = u[0]*v[0] + u[1]*v[1];
        float theta = acos(dot);
        if (u[1]*v[0] > u[0]*v[1])
            theta = TAU-theta;
        float s = sin(theta/2.0), c = cos(theta/2.0);
        normals[p][0] = u[0]*c - u[1]*s;
        normals[p][1] = u[0]*s + u[1]*c;
    }
    // fix up first and last
    if (!closed) {
        float v[2] = {points[0][0] - points[1][0], points[0][1] - points[1][1]};
        normalizeV(v);
        normals[0][0] = -v[1];
        normals[0][1] = v[0];
    }
    if (!closed)
    {
        float v[2] = {points[count-1][0] - points[count-2][0], points[count-1][1]-points[count-2][1]};
        normalizeV(v);
        normals[count-1][0] = v[1];
        normals[count-1][1] = -v[0];
    }

    float (*vert)[2] = new float[2*(count+1)][2];
    int v = 0;

    for (int p=0 ; p<count ; p++)
    {
        float *point = (float *)(points + p);
        float *normal = (float *)(normals + p);
        if (normal[0]==0.0 and normal[1]==0.0)
            continue;
        vert[v][0] = point[0] + T*normal[0];
        vert[v][1] = point[1] + T*normal[1];
        v++;
        vert[v][0] = point[0] - T*normal[0];
        vert[v][1] = point[1] - T*normal[1];
        v++;
    }
    if (closed)
    {
        vert[v][0] = vert[0][0];
        vert[v][1] = vert[0][1];
        v++;
        vert[v][0] = vert[1][0];
        vert[v][1] = vert[1][1];
        v++;
    }
    delete[] normals;

    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vert);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, v);

    delete[] vert;
}




inline void drawClosedPath(float (*points)[2], int count, float thickness)
{
    drawPath(points, count, thickness, true);
}


inline void drawOpenPath(float (*points)[2], int count, float thickness)
{
    drawPath(points, count, thickness, false);
}



void SvgShape::Draw(RenderContext &context)
{
    svg_op *data = svg_data();
    int length = svg_length();
    float scale = radius;

    int currentPos=0;
    float points[500][2];
    float color[4] = {border_r, border_g, border_b, border_a*masterAlpha};

    if ( additive==0)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color[0], color[1], color[2], color[3]);

    float pos_x = x, pos_y = 1.0-y;
    float lineWidth = 0.05 * scale;

    for ( int i=0 ; i<length ;i++)
    {
        svg_op *op = data+i;
        switch (op->type)
        {
            case 'M':
                if (currentPos > 0)
                {
                    drawOpenPath(points, currentPos, lineWidth);
                    currentPos = 0;
                }
                points[currentPos][0] = op->data[0]*scale+pos_x;
                points[currentPos][1] = op->data[1]*scale+pos_y;
                //drawLineCap(points[currentPos], color, lineWidth);
                currentPos++;
                break;
            case 'Z':
                //points[currentPos][0] = points[0][0];
                //points[currentPos][1] = points[0][1];
                //drawLineCap(points[currentPos],  color, lineWidth);
                //currentPos++;
                drawClosedPath(points, currentPos, lineWidth);
                currentPos = 0;
                break;
            case 'L':
                points[currentPos][0] = op->data[0]*scale+pos_x;
                points[currentPos][1] = op->data[1]*scale+pos_y;
                //drawLineCap(points[currentPos],  color, lineWidth);
                currentPos++;
                break;
            case 'l':
                points[currentPos][0] = points[currentPos-1][0] + op->data[0]*scale;
                points[currentPos][1] = points[currentPos-1][1] + op->data[1]*scale;
                //drawLineCap(points[currentPos], color, lineWidth);
                currentPos++;
                break;
            case 'C':
            {
                float start[2] = {points[currentPos - 1][0], points[currentPos - 1][1]};
                float c1[2]  = {op->data[0] * scale + pos_x, op->data[1] * scale + pos_y};
                float c2[2]  = {op->data[2] * scale + pos_x, op->data[3] * scale + pos_y};
                float end[2] = {op->data[4] * scale + pos_x, op->data[5] * scale + pos_y};
                currentPos += cubic(8, points + currentPos, start, c1, c2, end);
                break;
            }
            case 'c':
            {
                float start[2] = {points[currentPos-1][0], points[currentPos-1][1] };
                float c1[2] =    { start[0] + op->data[0]*scale, start[1] + op->data[1]*scale };
                float c2[2] =    { start[0] + op->data[2]*scale, start[1] + op->data[3]*scale };
                float end[2] =   { start[0] + op->data[4]*scale, start[1] + op->data[5]*scale };
                currentPos += cubic(8, points+currentPos, start, c1, c2, end);
                break;
            }
        }
    }
    if (currentPos > 0)
    {
        drawOpenPath(points, currentPos, lineWidth);
        currentPos = 0;
    }

    release(data);
    return;
}

