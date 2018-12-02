/*
 * Authors:
 * Dmitriy A. Gearasimov <gerasimov.dmitriy@demlabs.net>
 * DeM Labs Inc.   https://demlabs.net
 * Kelvin Project https://github.com/kelvinblockchain
 * Copyright  (c) 2017-2018
 * All rights reserved.

 This file is part of DAP (Deus Applications Prototypes) the open source project

    DAP (Deus Applicaions Prototypes) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DAP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with any DAP based project.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include "dap_common.h"
#include "dap_config.h"
//#include "dap_
#include "dap_chain_net.h"
#include "dap_chain_node_ctl.h"
#include "dap_module.h"

#define LOG_TAG "chain_net"

/**
  * @struct dap_chain_net_pvt
  * @details Private part of chain_net dap object
  */
typedef struct dap_chain_net_pvt{
    pthread_t proc_tid;
    pthread_cond_t proc_cond;
    dap_chain_node_role_t node_role;
    //dap_client_t client;
    dap_chain_node_ctl_t * node;
} dap_chain_net_pvt_t;

#define PVT(a) ( (dap_chain_net_pvt_t *) a->pvt )
#define PVT_S(a) ( (dap_chain_net_pvt_t *) a.pvt )

size_t            s_net_configs_count = 0;
pthread_cond_t    s_net_proc_loop_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t    s_net_proc_loop_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief s_net_proc_thread
 * @details Brings up and check the Dap Chain Network
 * @param a_cfg Network1 configuration
 * @return
 */
static void * s_net_proc_thread ( void * a_net)
{
    dap_chain_net_t * l_net = (dap_chain_net_t *) a_net;

    return NULL;
}

/**
 * @brief net_proc_start
 * @param a_cfg
 */
static void s_net_proc_start( dap_chain_net_t * a_net )
{
    if ( pthread_create(& PVT(a_net)->proc_tid ,NULL, s_net_proc_thread, a_net) == 0 ){
        log_it (L_NOTICE,"Network processing thread started");
        dap_chain_node_role_t l_role;
        switch (l_role.enums = PVT (a_net)->node_role.enums){
            case ROOT:
                log_it(L_DEBUG , "Root node functions initialized");
            case ROOT_DELEGATE:
                log_it(L_DEBUG , "Root delegate node functions initialized");
            case ARCHIVE:
                log_it(L_DEBUG , "Archive node functions initialized");
            case SHARD_DELEGATE:
                if ( l_role.enums != ARCHIVE ){
                    log_it(L_DEBUG , "Shard delegate node functions initialized");
                }
            case MASTER:
                log_it(L_DEBUG , "Master node functions initialized");
            case FULL:
                log_it(L_DEBUG , "Full node functions initialized");
            case LIGHT:
                log_it(L_DEBUG , "Light node functions initialized");
            default:
                log_it(L_NOTICE, "Node role initialized");
        }
    }
}

/**
 * @brief s_net_proc_kill
 * @param a_net
 */
static void s_net_proc_kill( dap_chain_net_t * a_net )
{
    if ( PVT(a_net)->proc_tid ) {
        pthread_cond_signal(& PVT(a_net)->proc_cond);
        log_it(L_NOTICE,"Sent KILL signal to the net process thread %d, waiting for shutdown...",PVT(a_net)->proc_tid);
        pthread_join( PVT(a_net)->proc_tid , NULL);
        log_it(L_NOTICE,"Net process thread %d shutted down",PVT(a_net)->proc_tid);
        PVT(a_net)->proc_tid = 0;
    }
}

/**
 * @brief dap_chain_net_new
 * @param a_id
 * @param a_name
 * @param a_node_role
 * @param a_node_name
 * @return
 */
dap_chain_net_t * dap_chain_net_new(const char * a_id, const char * a_name ,
                                    const char * a_node_role, const char * a_node_name)
{
    dap_chain_net_t * ret = DAP_NEW_Z_SIZE (dap_chain_net_t, sizeof (ret->pub)+ sizeof (dap_chain_net_pvt_t) );
    ret->pub.name = strdup( a_name );
    if ( sscanf(a_id,"0x%llu", &ret->pub.id.uint64 ) == 1 ){
        if (strcmp (a_node_role, "root")==0){
            PVT(ret)->node_role.enums = ROOT;
            log_it (L_NOTICE, "Node role \"root\" selected");
        }

        PVT(ret)->node = dap_chain_node_ctl_open(a_node_name);
        if ( PVT(ret)->node ){

        } else {
            log_it( L_ERROR, "Can't open \"%s\" node's config",a_node_name);
        }
    } else {
        log_it (L_ERROR, "Wrong id format (\"%s\"). Must be like \"0x0123456789ABCDE\"" , a_id );
    }
    return ret;

}

/**
 * @brief dap_chain_net_delete
 * @param a_net
 */
void dap_chain_net_delete( dap_chain_net_t * a_net )
{
    DAP_DELETE( PVT(a_net)->node);
    DAP_DELETE( PVT(a_net) );
}


/**
 * @brief dap_chain_net_init
 * @return
 */
int dap_chain_net_init()
{
    static dap_config_t *l_cfg=NULL;
    if((l_cfg = dap_config_open( "network/default" ) ) == NULL) {
        log_it(L_ERROR,"Can't open default network config");
        return -1;
    }else{
        dap_chain_net_t * l_net = dap_chain_net_new(
                                            dap_config_get_item_str(l_cfg , "general" , "id" ),
                                            dap_config_get_item_str(l_cfg , "general" , "name" ),
                                            dap_config_get_item_str(l_cfg , "general" , "node-role" ),
                                            dap_config_get_item_str(l_cfg , "general" , "node-default" )
                                           );
        switch ( PVT( l_net )->node_role.enums ) {
            case ROOT:
            case ROOT_DELEGATE:
            case SHARD_DELEGATE:
               // dap_chain_net_ca_load ( dap_config_get_item_str (""));
            default:
                log_it(L_DEBUG,"Net config loaded");

        }

        s_net_proc_start(l_net);
        log_it(L_NOTICE, "Сhain network initialized");
        return 0;
    }
}

void dap_chain_net_deinit()
{
}
