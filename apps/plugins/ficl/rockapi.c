#include "plugin.h"
#include "ficl.h"

/* rb.lcd_update ( -- ) */
static void ficlPrimitive_lcd_update(ficlVm *vm)
{
    (void)vm;
    rb->lcd_update();
}

/* rb.lcd_clear_display ( -- ) */
static void ficlPrimitive_lcd_clear_display(ficlVm *vm)
{
    (void)vm;
    rb->lcd_clear_display();
}

/* rb.lcd_getstringsize ( c-addr u -- w h ) */
static void ficlPrimitive_lcd_getstringsize(ficlVm *vm)
{
    int w, h;
    ficlStackPopUnsigned(vm->dataStack); /* len */
    char *s = (char *)ficlStackPopPointer(vm->dataStack);

    rb->lcd_getstringsize(s, &w, &h);

    ficlStackPushInteger(vm->dataStack, w);
    ficlStackPushInteger(vm->dataStack, h);
}

/* rb.lcd_putsxy ( x y c-addr u -- ) */
static void ficlPrimitive_lcd_putsxy(ficlVm *vm)
{
    ficlStackPopUnsigned(vm->dataStack); /* len */
    char *str = (char *)ficlStackPopPointer(vm->dataStack);
    int y = ficlStackPopInteger(vm->dataStack);
    int x = ficlStackPopInteger(vm->dataStack);

    rb->lcd_putsxy(x,y,str);
}

/* rb.lcd_puts ( x y c-addr u -- ) */
static void ficlPrimitive_lcd_puts(ficlVm *vm)
{
    ficlStackPopUnsigned(vm->dataStack); /* len */
    char *str = (char *)ficlStackPopPointer(vm->dataStack);
    int y = ficlStackPopInteger(vm->dataStack);
    int x = ficlStackPopInteger(vm->dataStack);

    rb->lcd_puts(x,y,str);
}

/* rb.lcd_puts_scroll ( x y c-addr u -- ) */
static void ficlPrimitive_lcd_puts_scroll(ficlVm *vm)
{
    ficlStackPopUnsigned(vm->dataStack); /* len */
    char *str = (char *)ficlStackPopPointer(vm->dataStack);
    int y = ficlStackPopInteger(vm->dataStack);
    int x = ficlStackPopInteger(vm->dataStack);

    rb->lcd_puts_scroll(x,y,str);
}

/* rb.lcd_update_rect ( x y width height -- ) */
static void ficlPrimitive_lcd_update_rect(ficlVm *vm)
{
    int height = ficlStackPopInteger(vm->dataStack);
    int width = ficlStackPopInteger(vm->dataStack);
    int y = ficlStackPopInteger(vm->dataStack);
    int x = ficlStackPopInteger(vm->dataStack);

    rb->lcd_update_rect(x, y, width, height);
}


/* rb.sleep ( u -- ) */
static void ficlPrimitive_sleep(ficlVm *vm)
{
    unsigned ticks = ficlStackPopUnsigned(vm->dataStack);
    rb->sleep(ticks);
}

void ficlSystemCompilePlatform(ficlSystem *system)
{
    ficlDictionary *dictionary = ficlSystemGetDictionary(system);

    FICL_SYSTEM_ASSERT(system, dictionary);

    ficlDictionarySetPrimitive(dictionary, "rb.lcd_update",   ficlPrimitive_lcd_update,     FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "rb.lcd_clear_display", ficlPrimitive_lcd_clear_display, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "rb.lcd_getstringsize", ficlPrimitive_lcd_getstringsize, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "rb.lcd_putsxy", ficlPrimitive_lcd_putsxy, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "rb.lcd_puts", ficlPrimitive_lcd_puts, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "rb.lcd_puts_scroll", ficlPrimitive_lcd_puts_scroll, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "rb.lcd_update_rect",   ficlPrimitive_lcd_update_rect, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "rb.sleep", ficlPrimitive_sleep, FICL_WORD_DEFAULT);
    return;
}

