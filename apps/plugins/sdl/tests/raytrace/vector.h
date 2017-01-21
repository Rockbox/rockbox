#include <SDL.h>

typedef float scalar;

typedef struct vector_t {
    enum { RECT, SPH } type;
    union {
        struct {
            scalar x, y, z;
        } rect;
        struct {
            scalar r, elevation, azimuth;
        } sph;
    };
} vector;

scalar vect_abs(vector);

vector vect_mul(vector, scalar);
vector vect_add(vector, vector);
vector vect_sub(vector, vector);

vector vect_normalize(vector);
vector vect_negate(vector);

void vect_to_rect(vector*);
void vect_to_sph(vector*);

scalar vect_dot(vector v1, vector v2);
vector vect_cross(vector v1, vector v2);
