    .set noreorder

#include "pspstub.s"

    STUB_START "pluginlib",0x00090000,0x00010005
    STUB_FUNC  0xF9B12673,plugin_get_header
    STUB_END
