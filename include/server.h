//
// Created by depau on 1/26/21.
//

#ifndef WI_SE_SW_SERVER_H
#define WI_SE_SW_SERVER_H

void server_start();

void handleIndex(AsyncWebServerRequest *request);

void handleToken(AsyncWebServerRequest *request);

void handleStty(AsyncWebServerRequest *request);

#endif //WI_SE_SW_SERVER_H
