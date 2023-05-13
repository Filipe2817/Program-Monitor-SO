#ifndef HANDLER_H
#define HANDLER_H

#include "../../common/include/hashtable.h"
#include "../../common/include/request.h"

void handle_request(Request *request, Hashtable *ongoing_ht, Hashtable *finished_ht, char *directory);

#endif
