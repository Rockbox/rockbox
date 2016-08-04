/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#ifndef __HWSTUB_VIRTUAL_HPP__
#define __HWSTUB_VIRTUAL_HPP__

#include "hwstub.hpp"
#include <libusb.h>

namespace hwstub {
namespace virt {

class hardware;

/** Virtual context
 *
 * A virtual context hosts a number of virtual devices.
 * This kind of contexts is mostly useful for testing/debugging purposes */
class context : public hwstub::context
{
protected:
    context();
public:
    virtual ~context();
    /** Create a virtual context.  */
    static std::shared_ptr<context> create();
    /** To ease creation, the context can be given a specification of the initial
     * device list using the following format:
     * dev1;dev2;...
     * At the moment the only format support for devi is:
     * dummy(Device name) */
    static std::shared_ptr<context> create_spec(const std::string& spec, std::string *error = nullptr);

    /** Connect a device to the context. Return false if device is already connected,
     * and true otherwise. This method is thread-safe. */
    bool connect(std::shared_ptr<hardware> hw);
    /** Disconnect a device from the context. Return false if device is not connected,
     * and true otheriwse. This method is thread-safe. */
    bool disconnect(std::shared_ptr<hardware> hw);

protected:
    /* NOTE ctx_dev_t = hardware* */
    std::shared_ptr<hardware> from_ctx_dev(ctx_dev_t dev);
    ctx_dev_t to_ctx_dev(std::shared_ptr<hardware>& dev);
    virtual error fetch_device_list(std::vector<ctx_dev_t>& list, void*& ptr);
    virtual void destroy_device_list(void *ptr);
    virtual error create_device(ctx_dev_t dev, std::shared_ptr<device>& hwdev);
    virtual bool match_device(ctx_dev_t dev, std::shared_ptr<device> hwdev);

    std::vector<std::shared_ptr<hardware>> m_hwlist; /* List of connected hardware */
};

/** Virtual hardware device (server/provider side)
 *
 * This base class represents a virtual piece of hardware that is being accessed
 * by the context. Users of virtual contexts must inherit from this class and
 * implement the requests. All requests are guaranteed to be serialize at the device
 * level so there is no need to care about concurrency issues */
class hardware : public std::enable_shared_from_this<hardware>
{
public:
    hardware();
    virtual ~hardware();

    virtual error read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic) = 0;
    virtual error write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic) = 0;
    virtual error get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz) = 0;
    virtual error get_dev_log(void *buf, size_t& buf_sz) = 0;
    virtual error exec_dev(uint32_t addr, uint16_t flags) = 0;
    virtual size_t get_buffer_size() = 0;
};

/** Dummy implementation of an hardware.
 *
 * This dummy hardware will fail all operations except getting descriptors.
 * The device description can be customised */
class dummy_hardware : public hardware
{
public:
    dummy_hardware(const std::string& name);
    virtual ~dummy_hardware();

    virtual error read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic);
    virtual error write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic);
    virtual error get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz);
    virtual error get_dev_log(void *buf, size_t& buf_sz);
    virtual error exec_dev(uint32_t addr, uint16_t flags);
    virtual size_t get_buffer_size();

protected:
    struct hwstub_version_desc_t m_desc_version;
    struct hwstub_layout_desc_t m_desc_layout;
    struct hwstub_target_desc_t m_desc_target;
};

/** Virtual device (client/user side)
 *
 * Device based on virtual device. */
class device : public hwstub::device
{
    friend class context; /* for ctor */
protected:
    device(std::shared_ptr<hwstub::context> ctx, std::shared_ptr<hardware> dev);
public:
    virtual ~device();
    /** Get native device (possibly null) */
    std::shared_ptr<hardware> native_device();

protected:
    virtual error open_dev(std::shared_ptr<handle>& handle);
    virtual bool has_multiple_open() const;

    std::weak_ptr<hardware> m_hwdev; /* pointer to hardware */
};

/** Virtual handle
 *
 * Handle based on virtual device. */
class handle : public hwstub::handle
{
    friend class device; /* for ctor */
protected:
    handle(std::shared_ptr<hwstub::device> dev);
public:
    virtual ~handle();

protected:
    virtual error read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic);
    virtual error write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic);
    virtual error get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz);
    virtual error get_dev_log(void *buf, size_t& buf_sz);
    virtual error exec_dev(uint32_t addr, uint16_t flags);
    virtual error cop_dev(uint8_t op, uint8_t args[HWSTUB_COP_ARGS], const void *out_data,
        size_t out_size, void *in_data, size_t *in_size);
    virtual error status() const;
    virtual size_t get_buffer_size();

    std::weak_ptr<hardware> m_hwdev; /* pointer to hardware */
};

} // namespace virt
} // namespace hwstub

#endif /* __HWSTUB_VIRTUAL_HPP__ */
