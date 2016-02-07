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
#include "hwstub_uri.hpp"
#include "hwstub_usb.hpp"
#include "hwstub_virtual.hpp"
#include "hwstub_net.hpp"
#include <cctype>

namespace hwstub {
namespace uri {

void print_usage(FILE *f, bool client, bool server)
{
    if(client && server)
        fprintf(f, "A context/server");
    else if(client)
        fprintf(f, "A context");
    else
        fprintf(f, "A server");
    fprintf(f, " URI has the following format:\n");
    fprintf(f, "  scheme:[//domain[:port]][/]\n");
    fprintf(f, "The scheme is mandatory and specifies the type of context to create.\n");
    fprintf(f, "The following scheme are recognized:\n");
    if(client)
        fprintf(f, "  usb       USB context (using libusb)\n");
    fprintf(f, "  tcp       TCP context\n");
    fprintf(f, "  unix      Local unix domain context\n");
    if(client)
        fprintf(f, "  virt      A virtual context (testing and debugging mostly)\n");
    fprintf(f, "  default   Default choice made by the library\n");
    if(client)
    {
        fprintf(f, "When creating a USB context, the domain and port must be empty:\n");
        fprintf(f, "  usb:\n");
    }
    fprintf(f, "When creating a TCP context, the domain and port are the usual TCP parameters:\n");
    fprintf(f, "  tcp://localhost\n");
    fprintf(f, "  tcp://localhost:6666\n");
    fprintf(f, "The port is optional, in which the default port %s is used.\n", hwstub::net::context::default_tcp_port().c_str());
    fprintf(f, "When creating a unix domain context, the port must be empty and there are two type of unix domains.\n");
    fprintf(f, "Normal domains are specified by filesystem paths. Abstract domains (Linux-only) can be arbitrary strings,\n");
    fprintf(f, "in which case the domain must start with a '#':\n");
    fprintf(f, "  unix:///path/to/socket\n");
    fprintf(f, "  unix://#hwstub\n");
    if(client)
    {
        fprintf(f, "When creating a virtual context, the domain must contain a specification of the devices.\n");
        fprintf(f, "The device list is of the form type(param);type(param);... where the only supported type\n");
        fprintf(f, "at the moment is 'dummy' with a single parameter which is the device name:\n");
        fprintf(f, "  virt://dummy(Device A);dummy(Device B);dummy(Super device C)\n");
    }
    if(server && client)
        fprintf(f, "Note that usb and virt schemes are not supported for server URIs.\n");
}

uri::uri(const std::string& uri)
    :m_uri(uri), m_valid(false)
{
    parse();
}

bool uri::validate_scheme()
{
    /* scheme must be nonempty */
    if(m_scheme.empty())
    {
        m_error = "empty scheme";
        return false;
    }
    /* and contain alphanumeric characters or '+', '-' or '.' */
    for(auto c : m_scheme)
        if(!isalnum(c) && c != '+' && c != '-' && c != '.')
        {
            m_error = std::string("invalid character '") + c + "' in scheme";
            return false;
        }
    return true;
}

bool uri::validate_domain()
{
    return true;
}

bool uri::validate_port()
{
    return true;
}

void uri::parse()
{
    std::string str = m_uri;
    /* try to find the first ':' */
    size_t scheme_colon = str.find(':');
    if(scheme_colon == std::string::npos)
    {
        m_error = "URI contains no scheme";
        return;
    }
    /* found scheme */
    m_scheme = str.substr(0, scheme_colon);
    /* validate scheme */
    if(!validate_scheme())
        return;
    /* remove scheme: */
    str = str.substr(scheme_colon + 1);
    /* check if it begins with // */
    if(str.size() >= 2 && str[0] == '/' && str[1] == '/')
    {
        /* remove it */
        str = str.substr(2);
        /* find next '/' */
        std::string path;
        size_t next_slash = str.find('/');
        /* if none, we can assume the path is empty, otherwise split path */
        if(next_slash != std::string::npos)
        {
            path = str.substr(next_slash);
            str = str.substr(0, next_slash);
        }
        /* find last ':' if any, indicating a port */
        size_t port_colon = str.rfind(':');
        if(port_colon != std::string::npos)
        {
            m_domain = str.substr(0, port_colon);
            m_port = str.substr(port_colon + 1);
        }
        else
            m_domain = str;
        if(!validate_domain() || !validate_port())
            return;
        /* pursue with path */
        str = path;
    }
    /* next is path */
    m_path = str;
    m_valid = true;
}

bool uri::valid() const
{
    return m_valid;
}

std::string uri::error() const
{
    return m_error;
}

std::string uri::full_uri() const
{
    return m_uri;
}

std::string uri::scheme() const
{
    return m_scheme;
}

std::string uri::domain() const
{
    return m_domain;
}

std::string uri::port() const
{
    return m_port;
}

std::string uri::path() const
{
    return m_path;
}

/** Context creator */

std::shared_ptr<context> create_default_context(std::string *error)
{
    /* first try to create a unix context with abstract name 'hwstub' */
    std::shared_ptr<context> ctx = hwstub::net::context::create_unix_abstract(
        hwstub::net::context::default_unix_path(), error);
    if(ctx)
        return ctx;
    /* otherwise try default tcp */
    ctx = hwstub::net::context::create_tcp(hwstub::net::context::default_tcp_domain(),
        hwstub::net::context::default_tcp_port(), error);
    if(ctx)
        return ctx;
    /* otherwise default to usb */
    return hwstub::usb::context::create(nullptr, false, error);
}

std::shared_ptr<context> create_context(const uri& uri, std::string *error)
{
    /* check URI is valid */
    if(!uri.valid())
    {
        if(error)
            *error = "invalid URI: " + uri.error();
        return std::shared_ptr<context>();
    }
    /* handle different types of contexts */
    if(uri.scheme() == "usb")
    {
        /* domain and port must be empty */
        if(!uri.domain().empty() || !uri.port().empty())
        {
            if(error)
            *error = "USB URI cannot contain a domain or a port";
            return std::shared_ptr<context>();
        }
        /* in doubt, create a new libusb context and let the context destroy it */
        libusb_context *ctx;
        libusb_init(&ctx);
        return hwstub::usb::context::create(ctx, true);
    }
    else if(uri.scheme() == "virt")
    {
        /* port must be empty */
        if(!uri.port().empty())
        {
            if(error)
            *error = "virt URI cannot contain a port";
            return std::shared_ptr<context>();
        }
        return hwstub::virt::context::create_spec(uri.domain(), error);
    }
    else if(uri.scheme() == "tcp")
    {
        return hwstub::net::context::create_tcp(uri.domain(), uri.port(), error);
    }
    else if(uri.scheme() == "unix")
    {
        /* port must be empty */
        if(!uri.port().empty())
        {
            if(error)
                *error = "unix URI cannot contain a port";
            return std::shared_ptr<context>();
        }
        if(!uri.domain().empty() && uri.domain()[0] == '#')
            return hwstub::net::context::create_unix_abstract(uri.domain().substr(1), error);
        else
            return hwstub::net::context::create_unix(uri.domain(), error);
    }
    else if(uri.scheme() == "default")
    {
        /* domain and port must be empty */
        if(!uri.domain().empty() || !uri.port().empty())
        {
            if(error)
            *error = "Default URI cannot contain a domain or a port";
            return std::shared_ptr<context>();
        }
        return create_default_context(error);
    }
    else
    {
        if(error)
            *error = "unknown scheme '" + uri.scheme() + "'";
        return std::shared_ptr<context>();
    }
}

std::shared_ptr<net::server> create_server(std::shared_ptr<context> ctx,
    const uri& uri, std::string *error)
{
    /* check URI is valid */
    if(!uri.valid())
    {
        if(error)
            *error = "invalid URI: " + uri.error();
        return std::shared_ptr<net::server>();
    }
    /* handle different types of contexts */
    if(uri.scheme() == "unix")
    {
        /* port must be empty */
        if(!uri.port().empty())
        {
            if(error)
                *error = "unix URI cannot contain a port";
            return std::shared_ptr<net::server>();
        }
        if(!uri.domain().empty() && uri.domain()[0] == '#')
            return hwstub::net::server::create_unix_abstract(ctx, uri.domain().substr(1), error);
        else
            return hwstub::net::server::create_unix(ctx, uri.domain(), error);
    }
    else if(uri.scheme() == "tcp")
    {
        return hwstub::net::server::create_tcp(ctx, uri.domain(), uri.port(), error);
    }
    else
    {
        if(error)
            *error = "unknown scheme '" + uri.scheme() + "'";
        return std::shared_ptr<net::server>();
    }
}

uri default_uri()
{
    return uri("default:");
}

uri default_server_uri()
{
    return uri("unix://#" + hwstub::net::context::default_unix_path());
}

} // namespace uri
} // namespace hwstub

