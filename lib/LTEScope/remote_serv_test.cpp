/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

extern "C"{
#include "read_cfg.h"
}
#include "serv_sock.h"


bool go_exit = false; 
void sig_int_handler(int signo)
{
  printf("SIGINT received. Exiting...\n");
  if (signo == SIGINT) {
    go_exit = true;
  } else if (signo == SIGSEGV) {
    exit(1);
  }
}

int main(int argc, char **argv) {
    srslte_config_t main_config;
    read_config_master(&main_config);

    sock_cfg_t sock_config;
    read_sock_config(&sock_config);
 
    printf("A");
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
    signal(SIGINT, sig_int_handler);

  
    printf("A");
    sock_parm_t sock_parm;
    sock_parm.pkt_intval = 1000;
    sock_parm.con_time_s = 5000;
    sock_parm.nof_pkt    = 1;
    sock_parm.local_rf_enable = false;
    sock_parm.if_name = NULL;
    strcpy(sock_parm.servIP, "3.14.132.89");
    printf("B");
    for(int pkt_idx=0;pkt_idx<sock_config.nof_pkt_intval;pkt_idx++){ 
	sock_parm.pkt_intval    = sock_config.pkt_intval[pkt_idx];
	for(int con_idx=0;con_idx<sock_config.nof_con_time;con_idx++){
	    if(go_exit) break;
	    sock_parm.con_time_s = sock_config.con_time[con_idx];
	    remote_server_1ms(&sock_parm);
	    sleep(5); 
	    printf("We are out of remote!\n");
	    /*   END THREADs */
	}
    }

    printf("\nBye MAIN FUNCTION!\n");
    exit(0);
}

