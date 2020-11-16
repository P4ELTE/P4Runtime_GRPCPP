#include <stdio.h>
#include "PI/proto/pi_server.h"
#include "device_mgr.h"

extern device_mgr_t *dev_mgr_ptr;

void dummy_callback(struct p4_ctrl_msg* ctrl_m) {
        printf("Dummy callback - table_name::%s\n", ctrl_m->table_name);
}

uint32_t* dummy_get_counter_by_name(char* name, int* size, bool is_bytes) {
        printf("Dummy callback - get_counter_by_name::%s\n", name);
	uint32_t* res= (uint32_t*)malloc(sizeof(uint32_t));
	return res;
}


int
main (int argc, char **argv)
{
    dev_mgr_init_with_t4p4s(dev_mgr_ptr, dummy_callback, dummy_get_counter_by_name, 1);

    PIGrpcServerRun();

    PIGrpcServerWait();

}

