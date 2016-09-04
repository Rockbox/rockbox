float sqrtf(float square)
{
    float x = square;

    /* famous doom reverse square root version */
    float xhalf = 0.5f*x;
    int i = *(int*)&x;
    i = 0x5f3759df - (i >> 1);
    x = *(float*)&i;
    x = x*(1.5f - xhalf*x*x);

    /* sqrt(x) = x * 1/sqrt(x) */
    return square * x;
}
