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
#ifndef __HWSTUB_URI_HPP__
#define __HWSTUB_URI_HPP__

#include "hwstub.hpp"
#include "hwstub_net.hpp"

namespace hwstub {
namespace uri {

/** HWSTUB URIs
 *
 * They are of the form:
 * 
 * scheme:[//domain[:port]][/][path[?query]]
 * 
 * The scheme is mandatory and controls the type of context that is created.
 * The following scheme are recognized:
 *   usb      USB context
 *   tcp      TCP context
 *   unix     Unix domain context
 *   virt     Virtual context (Testing and debugging)
 *   default  Default context (This is the default)
 *
 * When creating a USB context, the domain and port must be empty:
 *   usb:
 *
 * When creating a TCP context, the domain and port are given as argument to
 * the context:
 *   tcp://localhost:6666
 *
 * When creating a Unix context, the domain is given as argument to
 * the context, it is invalid to specify a port. There are two types of
 * unix contexts: the one specified by a filesystem path, or (Linux-only) by
 * an abstract domain. Abstract names are specified as a domain starting with a '#',
 * whereas standard path can be any path:
 *   unix:///path/to/socket
 *   unix://#hwstub
 *
 * When creating a virtual context, the domain will contain a specification of
 * the device to create. The device list is of the type(param);type(param);...
 * where the only supported type at the moment is 'dummy' with a single parameter
 * which is the device name:
 *   virt://dummy(Device A);dummy(Device B);dummy(Super device C)
 *
 *
 * HWSTUB SERVER URIs
 *
 * The same scheme can be used to spawn servers. Server URIs are a subset of
 * context URIs and only support tcp and unix schemes.
 */

/** URI
 * 
 * Represents an URI and allows queries on it */
class uri
{
public:
    uri(const std::string& uri);
    /** Return whether the URI is syntactically correct */
    bool valid() const;
    /** Return error description if URI is invalid */
    std::string error() const;
    /** Return the original URI */
    std::string full_uri() const;
    /** Return the scheme */
    std::string scheme() const;
    /** Return the domain, or empty is none */
    std::string domain() const;
    /** Return the port, or empty is none */
    std::string port() const;
    /** Return the path, or empty is none */
    std::string path() const;

protected:
    void parse();
    bool validate_scheme();
    bool validate_domain();
    bool validate_port();

    std::string m_uri; /* original uri */
    bool m_valid; /* did it parse correctly ? */
    std::string m_scheme; /* scheme (extracted from URI) */
    std::string m_domain; /* domain (extracted from URI) */
    std::string m_port; /* port (extracted from URI) */
    std::string m_path; /* path (extracted from URI) */
    std::string m_error; /* error string (for invalid URIs) */
};

/** Create a context based on a URI. This function only uses the scheme/domain/port
 * parts of the URI. This function may fail and return a empty pointer. An optional
 * string can receive a description of the error */
std::shared_ptr<context> create_context(const uri& uri, std::string *error = nullptr);
/** Return a safe default for a URI */
uri default_uri();
/** Special case function for the default function */
std::shared_ptr<context> create_default_context(std::string *error = nullptr);
/** Create a server based on a URI. This function only uses the scheme/domain/port
 * parts of the URI. This function may fail and return a empty pointer. An optional
 * string can receive a description of the error */
std::shared_ptr<net::server> create_server(std::shared_ptr<context> ctx,
    const uri& uri, std::string *error);
/** Return a safe default for a server URI */
uri default_server_uri();
/** Print help for the format of a URI, typically for a command-line help.
 * The help can be client-only, server-only, or both. */
void print_usage(FILE *f, bool client, bool server);

} // namespace uri
} // namespace hwstub

#endif /* __HWSTUB_URI_HPP__ */
