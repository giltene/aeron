/*
 * Copyright 2014-2019 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(__linux__)
#define _BSD_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <string.h>
#include "aeron_alloc.h"
#include "util/aeron_error.h"
#include "util/aeron_dlopen.h"

#include "aeron_udp_channel_transport_bindings.h"
#include "aeron_udp_channel_transport.h"
#include "aeron_udp_channel_transport_loss.h"
#include "aeron_udp_transport_poller.h"

aeron_udp_channel_transport_bindings_t aeron_udp_channel_transport_bindings_default =
    {
        aeron_udp_channel_transport_init,
        aeron_udp_channel_transport_close,
        aeron_udp_channel_transport_recvmmsg,
        aeron_udp_channel_transport_sendmmsg,
        aeron_udp_channel_transport_sendmsg,
        aeron_udp_channel_transport_get_so_rcvbuf,
        aeron_udp_channel_transport_bind_addr_and_port,
        aeron_udp_transport_poller_init,
        aeron_udp_transport_poller_close,
        aeron_udp_transport_poller_add,
        aeron_udp_transport_poller_remove,
        aeron_udp_transport_poller_poll
    };

aeron_udp_channel_transport_bindings_t aeron_udp_channel_transport_bindings_loss =
    {
        aeron_udp_channel_transport_init,
        aeron_udp_channel_transport_close,
        aeron_udp_channel_transport_loss_recvmmsg,
        aeron_udp_channel_transport_loss_sendmmsg,
        aeron_udp_channel_transport_loss_sendmsg,
        aeron_udp_channel_transport_get_so_rcvbuf,
        aeron_udp_channel_transport_bind_addr_and_port,
        aeron_udp_transport_poller_init,
        aeron_udp_transport_poller_close,
        aeron_udp_transport_poller_add,
        aeron_udp_transport_poller_remove,
        aeron_udp_transport_poller_poll
    };

aeron_udp_channel_transport_bindings_t *aeron_udp_channel_transport_bindings_load_media(const char *bindings_name)
{
    aeron_udp_channel_transport_bindings_t *bindings = NULL;

    if (NULL == bindings_name)
    {
        aeron_set_err(EINVAL, "%s", "invalid UDP channel transport bindings name");
        return NULL;
    }

    if (strncmp(bindings_name, "default", sizeof("default")) == 0)
    {
        return aeron_udp_channel_transport_bindings_load("aeron_udp_channel_transport_bindings_default", "");
    }
    else
    {
        if ((bindings = (aeron_udp_channel_transport_bindings_t *)aeron_dlsym(RTLD_DEFAULT, bindings_name)) == NULL)
        {
            aeron_set_err(EINVAL, "could not find UDP channel transport bindings %s: dlsym - %s",
                bindings_name, aeron_dlerror());
        }
    }

    return bindings;
}

//aeron_udp_channel_transport_bindings_t *aeron_udp_channel_transport_bindings_load_interceptors(
//    aeron_udp_channel_transport_bindings_t * media_bindings, const char *interceptors)
//{
//    aeron_udp_channel_transport_bindings_t *bindings = NULL;
//
//    if (NULL == bindings_name)
//    {
//        aeron_set_err(EINVAL, "%s", "invalid UDP channel transport bindings name");
//        return NULL;
//    }
//
//    if (strncmp(bindings_name, "default", sizeof("default")) == 0)
//    {
//        return aeron_udp_channel_transport_bindings_load("aeron_udp_channel_transport_bindings_default", "");
//    }
//    else
//    {
//        if ((bindings = (aeron_udp_channel_transport_bindings_t *)aeron_dlsym(RTLD_DEFAULT, bindings_name)) == NULL)
//        {
//            aeron_set_err(EINVAL, "could not find UDP channel transport bindings %s: dlsym - %s",
//                bindings_name, aeron_dlerror());
//        }
//    }
//
//    return bindings;
//}

aeron_udp_channel_transport_bindings_t *aeron_udp_channel_transport_bindings_load(
    const char *bindings_name,
    const char *bindings_args)
{
    aeron_udp_channel_transport_bindings_t *bindings = NULL;

    if (NULL == bindings_name)
    {
        aeron_set_err(EINVAL, "%s", "invalid UDP channel transport bindings name");
        return NULL;
    }

    if (strncmp(bindings_name, "default", sizeof("default")) == 0)
    {
        return aeron_udp_channel_transport_bindings_load("aeron_udp_channel_transport_bindings_default", "");
    }
    else if (strncmp(bindings_name, "loss", sizeof("loss")) == 0)
    {
        aeron_udp_channel_transport_loss_params_t* loss_params;
        aeron_alloc((void **)&loss_params, sizeof(aeron_udp_channel_transport_loss_params_t));

        char *params_str = strdup(bindings_args);
        if (aeron_udp_channel_transport_loss_parse_params(params_str, loss_params) < 0)
        {
            aeron_free(loss_params);
        }
        else
        {
            const char *delegate_bindings_name = NULL != loss_params->delegate_bindings_name ?
                loss_params->delegate_bindings_name : "default";

            const aeron_udp_channel_transport_bindings_t *delegate_bindings =
                aeron_udp_channel_transport_bindings_load(delegate_bindings_name, bindings_args);

            if (NULL != delegate_bindings)
            {
                aeron_udp_channel_transport_loss_init(delegate_bindings, loss_params);

                bindings = aeron_udp_channel_transport_bindings_load("aeron_udp_channel_transport_bindings_loss", "");
            }
        }
        aeron_free(params_str);
    }
    else
    {
        if ((bindings = (aeron_udp_channel_transport_bindings_t *)aeron_dlsym(RTLD_DEFAULT, bindings_name)) == NULL)
        {
            aeron_set_err(EINVAL, "could not find UDP channel transport bindings %s: dlsym - %s",
                bindings_name, aeron_dlerror());
        }
    }

    return bindings;
}
