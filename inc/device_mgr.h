#ifndef __DEVICE_MGR_H__
#define __DEVICE_MGR_H__

#include <stdint.h>
//#include "google/rpc/code.grpc.pb.h"
//#include "p4/v1/p4runtime.grpc.pb.h"
//#include "p4/config/v1/p4info.grpc.pb.h"
//#include "utils/map.h"
#include "handlers.h" /* T4P4S*/
#include "messages.h" /* T4P4S*/
//#include <grpc++/grpc++.h>
//#include <grpcpp/grpcpp.h>



typedef struct device_mgr_t /*{
	uint64_t device_id;
	map_t id_map;
	p4_msg_callback cb;
	int has_p4info;
	::p4::config::v1::P4Info p4info;
}*/ device_mgr_t;
/*
typedef enum {
	P4IDS_TABLE = 0,
} p4_ids;

grpc::Status table_insert(device_mgr_t *dm, const ::p4::v1::TableEntry &table_entry);

grpc::Status table_modify(device_mgr_t *dm, const ::p4::v1::TableEntry &table_entry);

grpc::Status table_delete(device_mgr_t *dm, const ::p4::v1::TableEntry &table_entry);

grpc::Status table_write(device_mgr_t *dm, ::p4::v1::Update::Type update, const ::p4::v1::TableEntry &table_entry);

grpc::Status dev_mgr_write(device_mgr_t *dm, const ::p4::v1::WriteRequest &request, ::p4::v1::WriteResponse *response);

grpc::Status dev_mgr_read(device_mgr_t *dm, const ::p4::v1::ReadRequest &request, ::p4::v1::ReadResponse *response);

grpc::Status dev_mgr_set_pipeline_config(device_mgr_t *dm, ::p4::v1::SetForwardingPipelineConfigRequest_Action action, const ::p4::v1::ForwardingPipelineConfig config);

grpc::Status dev_mgr_get_pipeline_config(device_mgr_t *dm, ::p4::v1::GetForwardingPipelineConfigRequest::ResponseType response_type, ::p4::v1::ForwardingPipelineConfig *config);


extern "C" {*/
  void dev_mgr_init(device_mgr_t *dm);
  void dev_mgr_init_with_t4p4s(device_mgr_t *dm, p4_msg_callback cb, uint64_t device_id);
//}

#endif /* __DEVICE_MGR_H__ */
