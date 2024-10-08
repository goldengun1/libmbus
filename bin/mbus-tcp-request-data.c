//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <string.h>

#include <stdio.h>
#include <mbus/mbus.h>

static int debug = 0;

//------------------------------------------------------------------------------
// Execution starts here:
//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    mbus_frame reply;
    mbus_frame_data reply_data;
    mbus_handle *handle = NULL;

    char *host, *addr_str, matching_addr[16], *xml_result;
    int address;
    long port;
    int timeout_ud2 = 10; // default timeout in seconds for data request function, later it will be taken from command arrgs.

    memset((void *)&reply, 0, sizeof(mbus_frame));
    memset((void *)&reply_data, 0, sizeof(mbus_frame_data));

    if (argc == 4)
    {
        host = argv[1];
        port = atol(argv[2]);
        addr_str = argv[3];
        debug = 0;
    }
    else if (argc == 5 && strcmp(argv[1], "-d") == 0)
    {
        host = argv[2];
        port = atol(argv[3]);
        addr_str = argv[4];
        debug = 1;
    }
    else if (argc == 6 && strcmp(argv[1], "-t") == 0)
    {
        timeout_ud2 = atol(argv[2]);
        host = argv[3];
        port = atol(argv[4]);
        addr_str = argv[5];
    }
    else if (argc == 7 && strcmp(argv[1], "-d") == 0 && strcmp(argv[2], "-t") == 0)
    {
        debug = 1;
        timeout_ud2 = atol(argv[3]);
        host = argv[4];
        port = atol(argv[5]);
        addr_str = argv[6];
    }
    else
    {
        fprintf(stderr, "usage: %s [-d] [-t TIMEOUT] host port mbus-address\n", argv[0]);
        fprintf(stderr, "\toptional flag -d for debug printout\n");
        fprintf(stderr, "\toptional flag -t for setting custom timeout value\n");
        return 0;
    }

    if ((port < 0) || (port > 0xFFFF))
    {
        fprintf(stderr, "Invalid port: %ld\n", port);
        return 1;
    }

    if ((handle = mbus_context_tcp(host, port)) == NULL)
    {
        fprintf(stderr, "Could not initialize M-Bus context: %s\n", mbus_error_str());
        return 1;
    }

    if (mbus_context_set_option(handle, MBUS_OPTION_CUSTOM_TIMEOUT, timeout_ud2) == -1)
    {
        fprintf(stderr, "Failed to set custom timeout\n");
        return 1;
    }

    if (debug)
    {
        mbus_register_send_event(handle, &mbus_dump_send_event);
        mbus_register_recv_event(handle, &mbus_dump_recv_event);
    }

    if (mbus_connect(handle) == -1)
    {
        fprintf(stderr, "Failed to setup connection to M-bus gateway\n");
        return 1;
    }

    if (mbus_is_secondary_address(addr_str))
    {
        // secondary addressing

        int ret;

        ret = mbus_select_secondary_address(handle, addr_str);

        if (ret == MBUS_PROBE_COLLISION)
        {
            fprintf(stderr, "%s: Error: The address mask [%s] matches more than one device.\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "%s: Error: The selected secondary address does not match any device [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "%s: Error: Failed to select secondary address [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        // else MBUS_PROBE_SINGLE

        if (mbus_send_request_frame(handle, MBUS_ADDRESS_NETWORK_LAYER) == -1)
        {
            fprintf(stderr, "Failed to send M-Bus request frame.\n");
            return 1;
        }
    }
    else
    {
        // primary addressing

        address = atoi(addr_str);
        if (mbus_send_request_frame(handle, address) == -1)
        {
            fprintf(stderr, "Failed to send M-Bus request frame.\n");
            return 1;
        }
    }

    if (mbus_recv_frame(handle, &reply) != MBUS_RECV_RESULT_OK)
    {
        fprintf(stderr, "Failed to receive M-Bus response frame: %s\n", mbus_error_str());
        return 1;
    }

    //
    // parse data and print in XML format
    //
    if (debug)
    {
        mbus_frame_print(&reply);
    }

    if (mbus_frame_data_parse(&reply, &reply_data) == -1)
    {
        fprintf(stderr, "M-bus data parse error: %s\n", mbus_error_str());
        return 1;
    }

    if ((xml_result = mbus_frame_data_xml(&reply_data)) == NULL)
    {
        fprintf(stderr, "Failed to generate XML representation of MBUS frame: %s\n", mbus_error_str());
        return 1;
    }

    printf("%s", xml_result);
    free(xml_result);

    // manual free
    if (reply_data.data_var.record)
    {
        mbus_data_record_free(reply_data.data_var.record); // free's up the whole list
    }

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}
