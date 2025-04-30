/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/queue.h>
#include <linux/netlink.h>


static int fd = -1;

/* Returns 0 on failure, 1 on success */
int uevent_init()
{
	return 0;
}

int uevent_get_fd()
{
    return fd;
}

int uevent_next_event(char* buffer, int buffer_length)
{
    return 0;
}

int uevent_add_native_handler(void (*handler)(void *data, const char *msg, int msg_len),
                             void *handler_data)
{


    return 0;
}

int uevent_remove_native_handler(void (*handler)(void *data, const char *msg, int msg_len))
{

    return 0;
}
