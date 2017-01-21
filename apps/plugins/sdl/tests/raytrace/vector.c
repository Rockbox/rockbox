#include "vector.h"

void vect_to_rect(vector *v)
{
    if(v->type == RECT)
        return;
    switch(v->type)
    {
    case SPH:
    {
        vector old = *v;
        v->type = RECT;
        v->rect.x = old.sph.r * cos(old.sph.elevation) * sin(old.sph.azimuth);
        v->rect.y = old.sph.r * sin(old.sph.elevation);
        v->rect.z = old.sph.r * cos(old.sph.elevation) * cos(old.sph.azimuth);
        break;
    }
    }
}

void vect_to_sph(vector *v)
{
    if(v->type == SPH)
        return;
    switch(v->type)
    {
    case RECT:
    {
        vector old = *v;
        v->type = SPH;
        v->sph.r = vect_abs(old);
        v->sph.elevation = atan2(old.rect.y, sqrt(old.rect.x*old.rect.x + old.rect.z*old.rect.z));
        v->sph.azimuth = atan2(old.rect.z, old.rect.x);
        break;
    }
    }
}

scalar vect_abs(vector v)
{
    switch(v.type)
    {
    case SPH:
        return fabs(v.sph.r);
    case RECT:
        return sqrt(v.rect.x * v.rect.x + v.rect.y * v.rect.y + v.rect.z * v.rect.z);
    }
}

vector vect_mul(vector v, scalar s)
{
    switch(v.type)
    {
    case SPH:
        v.sph.r *= s;
        break;
    case RECT:
        v.rect.x *= s;
        v.rect.y *= s;
        v.rect.z *= s;
        break;
    }
    return v;
}

vector vect_add(vector v1, vector v2)
{
    int old_type = v1.type;
    vect_to_rect(&v1);
    vect_to_rect(&v2);
    v1.rect.x += v2.rect.x;
    v1.rect.y += v2.rect.y;
    v1.rect.z += v2.rect.z;

    if(old_type == SPH)
        vect_to_sph(&v1);
    return v1;
}

vector vect_negate(vector v)
{
    switch(v.type)
    {
    case SPH:
        v.sph.r = -v.sph.r;
        break;
    case RECT:
        v.rect.x = -v.rect.x;
        v.rect.y = -v.rect.y;
        v.rect.z = -v.rect.z;
        break;
    }
    return v;
}

/* v1 = v1 - v2 */
vector vect_sub(vector v1, vector v2)
{
    v2 = vect_negate(v2);
    return vect_add(v1, v2);
}

scalar vect_dot(vector v1, vector v2)
{
    vect_to_rect(&v1);
    vect_to_rect(&v2);
    return v1.rect.x * v2.rect.x + v1.rect.y * v2.rect.y + v1.rect.z * v2.rect.z;
}

vector vect_normalize(vector v)
{
    switch(v.type)
    {
    case RECT:
    {
        scalar a = vect_abs(v);
        v = vect_mul(v, 1./a);
        break;
    }
    case SPH:
        v.sph.r = 1;
        break;
    }
    return v;
}

vector vect_cross(vector v1, vector v2)
{
    vect_to_rect(&v1);
    vect_to_rect(&v2);
    vector ret = { RECT, { v1.rect.y * v2.rect.z - v1.rect.z * v2.rect.y,
                           v1.rect.z * v2.rect.x - v1.rect.x * v2.rect.z,
                           v1.rect.x * v2.rect.y - v1.rect.y * v2.rect.x, } };
    return ret;
}
