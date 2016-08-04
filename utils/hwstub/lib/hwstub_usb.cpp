/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Amaury Pouly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "hwstub.hpp"
#include "hwstub_usb.hpp"
#include <cstring> /* for memcpy */

namespace hwstub {
namespace usb {

const uint8_t VR_GET_CPU_INFO = 0;
const uint8_t VR_SET_DATA_ADDRESS = 1;
const uint8_t VR_SET_DATA_LENGTH = 2;
const uint8_t VR_FLUSH_CACHES = 3;
const uint8_t VR_PROGRAM_START1 = 4;
const uint8_t VR_PROGRAM_START2 = 5;

/**
 * Context
 */

context::context(libusb_context *ctx, bool cleanup_ctx)
    :m_usb_ctx(ctx), m_cleanup_ctx(cleanup_ctx)
{
}

context::~context()
{
    if(m_cleanup_ctx)
        libusb_exit(m_usb_ctx);
}

std::shared_ptr<context> context::create(libusb_context *ctx, bool cleanup_ctx,
    std::string *error)
{
    (void) error;
    if(ctx == nullptr)
        libusb_init(nullptr);
    // NOTE: can't use make_shared() because of the protected ctor */
    return std::shared_ptr<context>(new context(ctx, cleanup_ctx));
}

libusb_context *context::native_context()
{
    return m_usb_ctx;
}

libusb_device *context::from_ctx_dev(ctx_dev_t dev)
{
    return reinterpret_cast<libusb_device*>(dev);
}

hwstub::context::ctx_dev_t context::to_ctx_dev(libusb_device *dev)
{
    return static_cast<ctx_dev_t>(dev);
}

error context::fetch_device_list(std::vector<ctx_dev_t>& list, void*& ptr)
{
    libusb_device **usb_list;
    ssize_t ret = libusb_get_device_list(m_usb_ctx, &usb_list);
    if(ret < 0)
        return error::ERROR;
    ptr = (void *)usb_list;
    list.clear();
    for(int i = 0; i < ret; i++)
        if(device::is_hwstub_dev(usb_list[i]))
            list.push_back(to_ctx_dev(usb_list[i]));
    return error::SUCCESS;
}

void context::destroy_device_list(void *ptr)
{
    /* remove all references */
    libusb_free_device_list((libusb_device **)ptr, 1);
}

error context::create_device(ctx_dev_t dev, std::shared_ptr<hwstub::device>& hwdev)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    hwdev.reset(new device(shared_from_this(), from_ctx_dev(dev)));
    return error::SUCCESS;
}

bool context::match_device(ctx_dev_t dev, std::shared_ptr<hwstub::device> hwdev)
{
    device *udev = dynamic_cast<device*>(hwdev.get());
    return udev != nullptr && udev->native_device() == dev;
}

/**
 * Device
 */
device::device(std::shared_ptr<hwstub::context> ctx, libusb_device *dev)
    :hwstub::device(ctx), m_dev(dev)
{
    libusb_ref_device(dev);
}

device::~device()
{
    libusb_unref_device(m_dev);
}

libusb_device *device::native_device()
{
    return m_dev;
}

bool device::is_hwstub_dev(libusb_device *dev)
{
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *config = nullptr;
    int intf = 0;
    if(libusb_get_device_descriptor(dev, &dev_desc) != 0)
        goto Lend;
    if(libusb_get_config_descriptor(dev, 0, &config) != 0)
        goto Lend;
    /* Try to find Rockbox hwstub interface or a JZ device */
    if(rb_handle::find_intf(&dev_desc, config, intf) ||
            jz_handle::is_boot_dev(&dev_desc, config))
    {
        libusb_free_config_descriptor(config);
        return true;
    }
Lend:
    if(config)
        libusb_free_config_descriptor(config);
    return false;
}

error device::open_dev(std::shared_ptr<hwstub::handle>& handle)
{
    int intf = -1;
    /* open the device */
    libusb_device_handle *h;
    int err = libusb_open(m_dev, &h);
    if(err != LIBUSB_SUCCESS)
        return error::ERROR;
    /* fetch some descriptors */
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *config = nullptr;
    if(libusb_get_device_descriptor(m_dev, &dev_desc) != 0)
        goto Lend;
    if(libusb_get_config_descriptor(m_dev, 0, &config) != 0)
        goto Lend;
    /* Try to find Rockbox hwstub interface */
    if(rb_handle::find_intf(&dev_desc, config, intf))
    {
        libusb_free_config_descriptor(config);
        /* create the handle */
        // NOTE: can't use make_shared() because of the protected ctor */
        handle.reset(new rb_handle(shared_from_this(), h, intf));
    }
    /* Maybe this is a JZ device ? */
    else if(jz_handle::is_boot_dev(&dev_desc, config))
    {
        libusb_free_config_descriptor(config);
        /* create the handle */
        // NOTE: can't use make_shared() because of the protected ctor */
        handle.reset(new jz_handle(shared_from_this(), h));
    }
    else
    {
        libusb_free_config_descriptor(config);
        return error::ERROR;
    }
    /* the class will perform some probing on creation: check that it actually worked */
    if(handle->valid())
        return error::SUCCESS;
    /* abort */
    handle.reset(); // will close the libusb handle
    return error::ERROR;

Lend:
    if(config)
        libusb_free_config_descriptor(config);
    libusb_close(h);
    return error::ERROR;
}

bool device::has_multiple_open() const
{
    /* libusb only allows one handle per device */
    return false;
}

uint8_t device::get_bus_number()
{
    return libusb_get_bus_number(native_device());
}

uint8_t device::get_address()
{
    return libusb_get_device_address(native_device());
}

uint16_t device::get_vid()
{
    /* NOTE: doc says it's cached so it should always succeed */
    struct libusb_device_descriptor dev_desc;
    libusb_get_device_descriptor(native_device(), &dev_desc);
    return dev_desc.idVendor;
}

uint16_t device::get_pid()
{
    /* NOTE: doc says it's cached so it should always succeed */
    struct libusb_device_descriptor dev_desc;
    libusb_get_device_descriptor(native_device(), &dev_desc);
    return dev_desc.idProduct;
}

/**
 * USB handle
 */
handle::handle(std::shared_ptr<hwstub::device> dev, libusb_device_handle *handle)
    :hwstub::handle(dev), m_handle(handle)
{
    set_timeout(std::chrono::milliseconds(100));
}

handle::~handle()
{
    libusb_close(m_handle);
}

error handle::interpret_libusb_error(int err)
{
    if(err >= 0)
        return error::SUCCESS;
    if(err == LIBUSB_ERROR_NO_DEVICE)
        return error::DISCONNECTED;
    else
        return error::USB_ERROR;
}

error handle::interpret_libusb_error(int err, size_t expected_val)
{
    if(err < 0)
        return interpret_libusb_error(err);
    if((size_t)err != expected_val)
        return error::ERROR;
    return error::SUCCESS;
}

error handle::interpret_libusb_size(int err, size_t& out_siz)
{
    if(err < 0)
        return interpret_libusb_error(err);
    out_siz = (size_t)err;
    return error::SUCCESS;
}

void handle::set_timeout(std::chrono::milliseconds ms)
{
    m_timeout = ms.count();
}

/**
 * Rockbox Handle
 */

rb_handle::rb_handle(std::shared_ptr<hwstub::device> dev,
        libusb_device_handle *handle, int intf)
    :hwstub::usb::handle(dev, handle), m_intf(intf), m_transac_id(0), m_buf_size(1)
{
    m_probe_status = error::SUCCESS;
    /* claim interface */
    if(libusb_claim_interface(m_handle, m_intf) != 0)
        m_probe_status = error::PROBE_FAILURE;
    /* check version */
    if(m_probe_status == error::SUCCESS)
    {
        struct hwstub_version_desc_t ver_desc;
        m_probe_status = get_version_desc(ver_desc);
        if(m_probe_status == error::SUCCESS)
        {
            if(ver_desc.bMajor != HWSTUB_VERSION_MAJOR)
                m_probe_status = error::PROBE_FAILURE;
        }
    }
    /* get buffer size */
    if(m_probe_status == error::SUCCESS)
    {
        struct hwstub_layout_desc_t layout_desc;
        m_probe_status = get_layout_desc(layout_desc);
        if(m_probe_status == error::SUCCESS)
            m_buf_size = layout_desc.dBufferSize;
    }
}

rb_handle::~rb_handle()
{
}

size_t rb_handle::get_buffer_size()
{
    return m_buf_size;
}

error rb_handle::status() const
{
    error err = handle::status();
    if(err == error::SUCCESS)
        err = m_probe_status;
    return err;
}

error rb_handle::get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz)
{
    return interpret_libusb_size(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        LIBUSB_REQUEST_GET_DESCRIPTOR, desc << 8, m_intf, (unsigned char *)buf, buf_sz, m_timeout),
        buf_sz);
}

error rb_handle::get_dev_log(void *buf, size_t& buf_sz)
{
    return interpret_libusb_size(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        HWSTUB_GET_LOG, 0, m_intf, (unsigned char *)buf, buf_sz, m_timeout), buf_sz);
}

error rb_handle::exec_dev(uint32_t addr, uint16_t flags)
{
    struct hwstub_exec_req_t exec;
    exec.dAddress = addr;
    exec.bmFlags = flags;
    return  interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        HWSTUB_EXEC, 0, m_intf, (unsigned char *)&exec, sizeof(exec), m_timeout), sizeof(exec));
}

error rb_handle::read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic)
{
    struct hwstub_read_req_t read;
    read.dAddress = addr;
    error err = interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        HWSTUB_READ, m_transac_id, m_intf, (unsigned char *)&read, sizeof(read), m_timeout),
        sizeof(read));
    if(err != error::SUCCESS)
        return err;
    return interpret_libusb_size(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        atomic ? HWSTUB_READ2_ATOMIC : HWSTUB_READ2, m_transac_id++, m_intf,
        (unsigned char *)buf, sz, m_timeout), sz);
}

error rb_handle::write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic)
{
    size_t hdr_sz = sizeof(struct hwstub_write_req_t);
    uint8_t *tmp_buf = new uint8_t[sz + hdr_sz];
    struct hwstub_write_req_t *req = reinterpret_cast<struct hwstub_write_req_t *>(tmp_buf);
    req->dAddress = addr;
    memcpy(tmp_buf + hdr_sz, buf, sz);
    error ret = interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        atomic ? HWSTUB_WRITE_ATOMIC : HWSTUB_WRITE, m_transac_id++, m_intf,
        (unsigned char *)req, sz + hdr_sz, m_timeout), sz + hdr_sz);
    delete[] tmp_buf;
    return ret;
}

error rb_handle::cop_dev(uint8_t op, uint8_t args[HWSTUB_COP_ARGS],
    const void *out_data, size_t out_size, void *in_data, size_t *in_size)
{
    (void) op;
    (void) args;
    (void) out_data;
    (void) out_size;
    (void) in_data;
    (void) in_size;
    std::shared_ptr<hwstub::context> hctx = get_device()->get_context();
    if(!hctx)
        return error::NO_CONTEXT;

    /* construct out request: header followed by (optional) data */
    size_t hdr_sz = sizeof(struct hwstub_cop_req_t);
    uint8_t *tmp_buf = new uint8_t[out_size + hdr_sz];
    struct hwstub_cop_req_t *req = reinterpret_cast<struct hwstub_cop_req_t *>(tmp_buf);
    req->bOp = op;
    for(int i = 0; i < HWSTUB_COP_ARGS; i++)
        req->bArgs[i] = args[i];
    if(out_size > 0)
        memcpy(tmp_buf + hdr_sz, out_data, out_size);
    error ret = interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        HWSTUB_COPROCESSOR_OP, m_transac_id++, m_intf,
        (unsigned char *)req, out_size + hdr_sz, m_timeout), out_size + hdr_sz);
    delete[] tmp_buf;
    /* return errors if any */
    if(ret != error::SUCCESS)
        return ret;
    /* return now if there is no read stage */
    if(in_data == nullptr)
        return error::SUCCESS;
    /* perform read stage (use the same transaction ID) */
    return interpret_libusb_size(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        HWSTUB_READ2, m_transac_id - 1, m_intf,
        (unsigned char *)in_data, *in_size, m_timeout), *in_size);
}

bool rb_handle::find_intf(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config, int& intf_idx)
{
    (void) dev;
    /* search hwstub interface */
    for(unsigned i = 0; i < config->bNumInterfaces; i++)
    {
        /* hwstub interface has only one setting */
        if(config->interface[i].num_altsetting != 1)
            continue;
        const struct libusb_interface_descriptor *intf = &config->interface[i].altsetting[0];
        /* check class/subclass/protocol */
        if(intf->bInterfaceClass == HWSTUB_CLASS &&
                intf->bInterfaceSubClass == HWSTUB_SUBCLASS &&
                intf->bInterfaceProtocol == HWSTUB_PROTOCOL)
        {
            /* found it ! */
            intf_idx = i;
            return true;
        }
    }
    return false;
}

/**
 * JZ Handle
 */

namespace
{
    uint16_t jz_bcd(char *bcd)
    {
        uint16_t v = 0;
        for(int i = 0; i < 4; i++)
            v = (bcd[i] - '0') | v << 4;
        return v;
    }
}

jz_handle::jz_handle(std::shared_ptr<hwstub::device> dev,
        libusb_device_handle *handle)
    :hwstub::usb::handle(dev, handle)
{
    m_probe_status = probe();
}

jz_handle::~jz_handle()
{
}

error jz_handle::probe()
{
    char cpuinfo[8];
    /* Get CPU info and devise descriptor */
    error err = jz_cpuinfo(cpuinfo);
    if(err != error::SUCCESS)
        return err;
    struct libusb_device_descriptor dev_desc;
    err = interpret_libusb_error(libusb_get_device_descriptor(
        libusb_get_device(m_handle), &dev_desc), 0);
    if(err != error::SUCCESS)
        return err;
    /** parse CPU info */
    /* if cpuinfo if of the form JZxxxxVy then extract xxxx */
    if(cpuinfo[0] == 'J' && cpuinfo[1] == 'Z' && cpuinfo[6] == 'V')
        m_desc_jz.wChipID = jz_bcd(cpuinfo + 2);
    /* if cpuinfo if of the form Bootxxxx then extract xxxx */
    else if(strncmp(cpuinfo, "Boot", 4) == 4)
        m_desc_jz.wChipID = jz_bcd(cpuinfo + 4);
    /* else use usb id */
    else
        m_desc_jz.wChipID = dev_desc.idProduct;
    m_desc_jz.bRevision = 0;

    /** Retrieve product string */
    memset(m_desc_target.bName, 0, sizeof(m_desc_target.bName));
    err = interpret_libusb_error(libusb_get_string_descriptor_ascii(m_handle,
        dev_desc.iProduct, (unsigned char *)m_desc_target.bName, sizeof(m_desc_target.bName)));
    if(err != error::SUCCESS)
        return err;
    /** The JZ4760 and JZ4760B cannot be distinguished by the above information,
     * for this the best way I have found is to check the SRAM size: 48KiB vs 16KiB.
     * This requires to enable AHB1 and SRAM clock and read/write to SRAM, but
     * this code will leaves registers and ram is the same state as before.
     * In case of failure, simply assume JZ4760. */
    if(m_desc_jz.wChipID == 0x4760)
        probe_jz4760b();

    /** Fill descriptors */
    m_desc_version.bLength = sizeof(m_desc_version);
    m_desc_version.bDescriptorType = HWSTUB_DT_VERSION;
    m_desc_version.bMajor = HWSTUB_VERSION_MAJOR;
    m_desc_version.bMinor = HWSTUB_VERSION_MINOR;
    m_desc_version.bRevision = 0;

    m_desc_layout.bLength = sizeof(m_desc_layout);
    m_desc_layout.bDescriptorType = HWSTUB_DT_LAYOUT;
    m_desc_layout.dCodeStart = 0xbfc00000; /* ROM */
    m_desc_layout.dCodeSize = 0x2000; /* 8kB per datasheet */
    m_desc_layout.dStackStart = 0; /* As far as I can tell, the ROM uses no stack */
    m_desc_layout.dStackSize = 0;
    m_desc_layout.dBufferStart = 0x080000000;
    m_desc_layout.dBufferSize = 0x4000;

    m_desc_target.bLength = sizeof(m_desc_target);
    m_desc_target.bDescriptorType = HWSTUB_DT_TARGET;
    m_desc_target.dID = HWSTUB_TARGET_JZ;

    m_desc_jz.bLength = sizeof(m_desc_jz);
    m_desc_jz.bDescriptorType = HWSTUB_DT_JZ;

    /* claim interface */
    if(libusb_claim_interface(m_handle, 0) != 0)
        m_probe_status = error::PROBE_FAILURE;

    return m_probe_status;
}

error jz_handle::read_reg32(uint32_t addr, uint32_t& value)
{
    size_t sz = sizeof(value);
    error err = read_dev(addr, &value, sz, true);
    if(err == error::SUCCESS && sz != sizeof(value))
        err = error::ERROR;
    return err;
}

error jz_handle::write_reg32(uint32_t addr, uint32_t value)
{
    size_t sz = sizeof(value);
    error err = write_dev(addr, &value, sz, true);
    if(err == error::SUCCESS && sz != sizeof(value))
        err = error::ERROR;
    return err;
}

error jz_handle::probe_jz4760b()
{
    /* first read CPM_CLKGR1 */
    const uint32_t cpm_clkgr1_addr = 0xb0000028;
    uint32_t cpm_clkgr1;
    error err = read_reg32(cpm_clkgr1_addr, cpm_clkgr1);
    if(err != error::SUCCESS)
        return err;
    /* Bit 7 controls AHB1 clock and bit 5 the SRAM. Note that SRAM is on AHB1.
     * Only ungate if gated */
    uint32_t cpm_clkgr1_mask = 1 << 7 | 1 << 5;
    if(cpm_clkgr1 & cpm_clkgr1_mask)
    {
        /* ungate both clocks */
        err = write_reg32(cpm_clkgr1_addr, cpm_clkgr1 & ~cpm_clkgr1_mask);
        if(err != error::SUCCESS)
            return err;
    }
    /* read first word of SRAM and then at end (supposedly) */
    uint32_t sram_addr = 0xb32d0000;
    uint32_t sram_end_addr = sram_addr + 16 * 1024; /* SRAM is 16KiB on JZ4760B */
    uint32_t sram_start, sram_end;
    err = read_reg32(sram_addr, sram_start);
    if(err != error::SUCCESS)
        goto Lrestore;
    err = read_reg32(sram_end_addr, sram_end);
    if(err != error::SUCCESS)
        goto Lrestore;
    /* if start and end are different, clearly the size is not 16KiB and this is
     * JZ4760 and we have nothing to do */
    if(sram_start != sram_end)
        goto Lrestore;
    /* now reverse all bits of the first word */
    sram_start ^= 0xffffffff;
    err = write_reg32(sram_addr, sram_start);
    if(err != error::SUCCESS)
        goto Lrestore;
    /* and read again at end */
    err = read_reg32(sram_end_addr, sram_end);
    if(err != error::SUCCESS)
        goto Lrestore;
    /* if they are still equal, we identified JZ4760B */
    if(sram_start == sram_end)
        m_desc_jz.bRevision = 'B';
    /* restore SRAM value */
    sram_start ^= 0xffffffff;
    err = write_reg32(sram_addr, sram_start);
    if(err != error::SUCCESS)
        goto Lrestore;

Lrestore:
    /* restore gates if needed */
    if(cpm_clkgr1 & cpm_clkgr1_mask)
        return write_reg32(cpm_clkgr1_addr, cpm_clkgr1);
    else
        return error::SUCCESS;
}

size_t jz_handle::get_buffer_size()
{
    return m_desc_layout.dBufferSize;
}

error jz_handle::status() const
{
    error err = handle::status();
    if(err == error::SUCCESS)
        err = m_probe_status;
    return err;
}

error jz_handle::get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz)
{
    void *p = nullptr;
    switch(desc)
    {
        case HWSTUB_DT_VERSION: p = &m_desc_version; break;
        case HWSTUB_DT_LAYOUT: p = &m_desc_layout; break;
        case HWSTUB_DT_TARGET: p = &m_desc_target; break;
        case HWSTUB_DT_JZ: p = &m_desc_jz; break;
        default: break;
    }
    if(p == nullptr)
        return error::ERROR;
    /* size is in the bLength field of the descriptor */
    size_t desc_sz = *(uint8_t *)p;
    buf_sz = std::min(buf_sz, desc_sz);
    memcpy(buf, p, buf_sz);
    return error::SUCCESS;
}

error jz_handle::get_dev_log(void *buf, size_t& buf_sz)
{
    (void) buf;
    buf_sz = 0;
    return error::SUCCESS;
}

error jz_handle::exec_dev(uint32_t addr, uint16_t flags)
{
    (void) flags;
    /* FIXME the ROM always do call so the stub can always return, this behaviour
     * cannot be changed */
    /* NOTE assume that exec at 0x80000000 is a first stage load with START1,
     * otherwise flush cache and use START2 */
    if(addr == 0x80000000)
        return jz_start1(addr);
    error ret = jz_flush_caches();
    if(ret == error::SUCCESS)
        return jz_start2(addr);
    else
        return ret;
}

error jz_handle::read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic)
{
    (void) atomic;
    /* NOTE disassembly shows that the ROM will do atomic read on aligned words */
    error ret = jz_set_addr(addr);
    if(ret == error::SUCCESS)
        ret = jz_set_length(sz);
    if(ret == error::SUCCESS)
        ret = jz_upload(buf, sz);
    return ret;
}

error jz_handle::write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic)
{
    (void) atomic;
    /* NOTE disassembly shows that the ROM will do atomic read on aligned words */
    /* IMPORTANT BUG Despite what the manual suggest, one must absolutely NOT send
     * a VR_SET_DATA_LENGTH request for a write, otherwise it will have completely
     * random effects */
    error ret = jz_set_addr(addr);
    if(ret == error::SUCCESS)
        ret = jz_download(buf, sz);
    return ret;
}

error jz_handle::cop_dev(uint8_t op, uint8_t args[HWSTUB_COP_ARGS],
    const void *out_data, size_t out_size, void *in_data, size_t *in_size)
{
    (void) op;
    (void) args;
    (void) out_data;
    (void) out_size;
    (void) in_data;
    (void) in_size;
    return error::UNSUPPORTED;
}

bool jz_handle::is_boot_dev(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config)
{
    (void) config;
    /* don't bother checking the config descriptor and use the device ID only */
    return dev->idVendor ==  0x601a && dev->idProduct >= 0x4740 && dev->idProduct <= 0x4780;
}

error jz_handle::jz_cpuinfo(char cpuinfo[8])
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_GET_CPU_INFO, 0, 0, (unsigned char *)cpuinfo, 8, m_timeout), 8);
}

error jz_handle::jz_set_addr(uint32_t addr)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_ADDRESS, addr >> 16, addr & 0xffff, NULL, 0, m_timeout), 0);
}

error jz_handle::jz_set_length(uint32_t size)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_LENGTH, size >> 16, size & 0xffff, NULL, 0, m_timeout), 0);
}

error jz_handle::jz_upload(void *data, size_t& length)
{
    int xfer = 0;
    error err = interpret_libusb_error(libusb_bulk_transfer(m_handle,
        LIBUSB_ENDPOINT_IN | 1, (unsigned char *)data, length, &xfer, m_timeout));
    length = xfer;
    return err;
}

error jz_handle::jz_download(const void *data, size_t& length)
{
    int xfer = 0;
    error err = interpret_libusb_error(libusb_bulk_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | 1, (unsigned char *)data, length, &xfer, m_timeout));
    length = xfer;
    return err;
}

error jz_handle::jz_start1(uint32_t addr)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_PROGRAM_START1, addr >> 16, addr & 0xffff, NULL, 0, m_timeout), 0);
}

error jz_handle::jz_flush_caches()
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_FLUSH_CACHES, 0, 0, NULL, 0, m_timeout), 0);
}

error jz_handle::jz_start2(uint32_t addr)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_PROGRAM_START2, addr >> 16, addr & 0xffff, NULL, 0, m_timeout), 0);
}

} // namespace usb
} // namespace hwstub
