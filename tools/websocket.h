/* websocket.c
 *
 * Copyright 2015 Brian Swetland <swetland@frotz.net>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WEBSOCKET_SERVER_H_
#define _WEBSOCKET_SERVER_H_

typedef struct ws_server ws_server_t;

typedef void (*ws_callback_t)(unsigned op, void *msg, size_t len, void *cookie);

ws_server_t *ws_handshake(int fd, ws_callback_t func, void *cookie);
int ws_process_messages(ws_server_t *ws);

int ws_send_text(ws_server_t *ws, const void *msg, size_t len);
int ws_send_binary(ws_server_t *ws, const void *msg, size_t len);

void ws_close(ws_server_t *ws);

#endif
