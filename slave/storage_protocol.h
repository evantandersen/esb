//
//  storage_protocol.h
//  esb
//
//  Created by Evan Andersen on 12-12-14.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#ifndef esb_storage_protocol_h
#define esb_storage_protocol_h

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>

#include "network.h"

// Configuration constants.
#define MAX_CONFIG_LINE_LEN 1024    ///< Max characters in each config file line.
#define MAX_USERNAME_LEN 64         ///< Max characters of server username.
#define MAX_ENC_PASSWORD_LEN 64     ///< Max characters of server's encrypted password.
#define MAX_HOST_LEN 64             ///< Max characters of server hostname.
#define MAX_PORT_LEN 8              ///< Max characters of server port.
#define MAX_PATH_LEN 256            ///< Max characters of data directory path.

// Storage server constants.
#define MAX_TABLES 100              ///< Max tables supported by the server.
#define MAX_RECORDS_PER_TABLE 1000  ///< Max records per table.
#define MAX_TABLE_LEN 20            ///< Max characters of a table name.
#define MAX_KEY_LEN 20              ///< Max characters of a key name.
#define MAX_CONNECTIONS 10          ///< Max simultaneous client connections.

// Extended storage server constants.
#define MAX_COLUMNS_PER_TABLE 10    ///< Max columns per table.
#define MAX_COLNAME_LEN 20          ///< Max characters of a column name.
#define MAX_STRTYPE_SIZE 40         ///< Max SIZE of string types.
#define MAX_VALUE_LEN 800           ///< Max characters of a value.

// Error codes.
#define ERR_INVALID_PARAM 1             ///< A parameter is not valid.
#define ERR_CONNECTION_FAIL 2           ///< Error connecting to server.
#define ERR_NOT_AUTHENTICATED 3         ///< Client not authenticated.
#define ERR_AUTHENTICATION_FAILED 4     ///< Client authentication failed.
#define ERR_TABLE_NOT_FOUND 5           ///< The table does not exist.
#define ERR_KEY_NOT_FOUND 6             ///< The key does not exist.
#define ERR_UNKNOWN 7                   ///< Any other error.
#define ERR_TRANSACTION_ABORT 8         ///< Transaction abort error.
#define ERR_NOT_SUPPORTED 9             ///< Operation not suppored.


struct storage_record {
    char value[MAX_VALUE_LEN];
    uintptr_t metadata[8];
};

void* storage_connect(const char* hostname, const int port);

int storage_auth(const char* username, const char* passwd, void* conn);

int storage_auth_rsa(const char* username, const char* passwd, const char* pub_key, void* conn);

int storage_get(const char* table, const char* key, struct storage_record* record, void* conn);

int storage_set(const char* table, const char* key, struct storage_record* record, void* conn);

int storage_query(const char* table, const char* predicates, char** keys, const int max_keys, void* conn);

int storage_disconnect(void* conn);

#endif
