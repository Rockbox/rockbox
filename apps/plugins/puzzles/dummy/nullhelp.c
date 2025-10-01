#include "help.h"

const char help_text[] __attribute__((weak)) = "";
const char quick_help_text[] __attribute__((weak)) = "";
const unsigned short help_text_len __attribute__((weak)) = 0, quick_help_text_len __attribute__((weak)) = 0, help_text_words __attribute__((weak)) = 0;
struct style_text help_text_style[] __attribute__((weak)) = {};

const bool help_text_valid __attribute__((weak)) = false;
const bool quick_help_valid __attribute__((weak)) = false;
