#ifndef ENDPOINTS_H
#define ENDPOINTS_H

#include "mongoose.h"

void root_endpoint(struct mg_connection *, int, void *);
void ap_root_endpoint(struct mg_connection *, int, void *);
void config_endpoint(struct mg_connection *, int, void *);
void config_time_endpoint(struct mg_connection *, int, void *);
void config_wifi_endpoint(struct mg_connection *, int, void *);

#endif