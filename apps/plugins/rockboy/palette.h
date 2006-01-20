void pal_lock(byte n) ICODE_ATTR;
byte pal_getcolor(int c, int r, int g, int b) ICODE_ATTR;
void pal_release(byte n) ICODE_ATTR;
void pal_expire(void) ICODE_ATTR;
void pal_set332(void) ICODE_ATTR;
void vid_setpal(int i, int r, int g, int b) ICODE_ATTR;
