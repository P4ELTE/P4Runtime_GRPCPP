//#include <grpc++/grpc++.h>
#include <grpcpp/grpcpp.h>
#include "gnmi.h"
#include "google/rpc/code.pb.h"
//#include "p4/server/v1/config.grpc.pb.h"
#include "p4/v1/p4runtime.grpc.pb.h"
#include <string>
#include <iostream>
#include "uint128.h"
#include <cstdio>
#include "PI/proto/pi_server.h"
#include "device_mgr.h"

#define SIMPLELOG std::cout

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::ServerReaderWriter;
using grpc::Status;
using grpc::StatusCode;

namespace p4v1 = ::p4::v1;
//namespace p4serverv1 = ::p4::server::v1;


static device_mgr_t dm;

extern "C" {
	device_mgr_t *dev_mgr_ptr = &dm;
}
/*
void gnmi__g_nmi__capabilities_cb (grpc_c_context_t *context)
{
	 printf("gnmi__g_nmi__capabilities_cb\n");
}

void gnmi__g_nmi__get_cb (grpc_c_context_t *context)
{
	printf("gnmi__g_nmi__get_cb\n");
}

void gnmi__g_nmi__set_cb (grpc_c_context_t *context)
{
	printf("gnmi__g_nmi__set_cb");
}

void gnmi__g_nmi__subscribe_cb (grpc_c_context_t *context)
{
	printf("gnmi__g_nmi__set_cb");
}
*/

grpc::Status no_pipeline_config_status() {
  return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                      "No forwarding pipeline config set for this device");
}

grpc::Status not_master_status() {
  return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "Not master");
}

using StreamChannelReaderWriter = grpc::ServerReaderWriter<p4v1::StreamMessageResponse, p4v1::StreamMessageRequest>;

void notify_one(StreamChannelReaderWriter *stream, uint64_t device_id, Uint128 election_id) {
    auto is_master = true;
    p4v1::StreamMessageResponse response;
    auto arbitration = response.mutable_arbitration();
    arbitration->set_device_id(device_id);
    auto convert_u128 = [](const Uint128 &from, p4v1::Uint128 *to) {
      to->set_high(from.high());
      to->set_low(from.low());
    };
    convert_u128(election_id,
                 arbitration->mutable_election_id());
    auto status = arbitration->mutable_status();
    if (is_master) {
      SIMPLELOG << "MASTER\n"; 
      status->set_code(::google::rpc::Code::OK);
      status->set_message("Is master");
    } else {
      SIMPLELOG << "SLAVE\n";
      status->set_code(::google::rpc::Code::ALREADY_EXISTS);
      status->set_message("Is slave");
    }
    stream->Write(response);
}


class P4RuntimeServiceImpl : public p4v1::P4Runtime::Service {
	private:
	Status Write(ServerContext *context,
               const p4v1::WriteRequest *request,
               p4v1::WriteResponse *rep) override {
    		SIMPLELOG << "P4Runtime Write\n";
    		SIMPLELOG << request->DebugString();
    		(void) rep;
    		SIMPLELOG << "Device ID:: " << request->device_id() << std::endl;
    		if (dm.has_p4info == 0) return no_pipeline_config_status();

		auto status = dev_mgr_write(&dm, *request, rep);
    		return status;
  	}

	Status Read(ServerContext *context,
               const p4v1::ReadRequest *request,
               ServerWriter<p4v1::ReadResponse> *writer) override {
    		SIMPLELOG << "P4Runtime Read\n";
    		SIMPLELOG << request->DebugString();
    		p4v1::ReadResponse response;
		SIMPLELOG << "Device ID:: " << request->device_id() << std::endl;
                if (dm.has_p4info == 0) {
			return no_pipeline_config_status();
		}
    		auto status = dev_mgr_read(&dm, *request, &response);
    		writer->Write(response);
    		return status;
  	}
	
	Status SetForwardingPipelineConfig(ServerContext *context,
	       const p4v1::SetForwardingPipelineConfigRequest *request,
	       p4v1::SetForwardingPipelineConfigResponse *rep) override {
    		SIMPLELOG << "P4Runtime SetForwardingPipelineConfig\n";
    		(void) rep;
		SIMPLELOG << "Device ID:: " << request->device_id() << std::endl;
		dm.device_id = request->device_id();
    		auto status = dev_mgr_set_pipeline_config(&dm, request->action(), request->config());
    		//device_mgr->stream_message_response_register_cb(stream_message_response_cb, NULL);
    		return status;
  	}

	Status GetForwardingPipelineConfig(ServerContext *context,
               const p4v1::GetForwardingPipelineConfigRequest *request,
               p4v1::GetForwardingPipelineConfigResponse *rep) override {
		SIMPLELOG << "P4Runtime GetForwardingPipelineConfig\n";
		SIMPLELOG << request->DebugString();
		SIMPLELOG << "Device ID:: " << request->device_id() << std::endl;
    		if (dm.has_p4info == 0) {
			SIMPLELOG << " -- No pipeline config status\n";
			return no_pipeline_config_status();
		}
		auto status = dev_mgr_get_pipeline_config(&dm, request->response_type(), rep->mutable_config());
    		return status;
  	}

	Status StreamChannel(ServerContext *context,
               StreamChannelReaderWriter *stream) override {
		p4v1::StreamMessageRequest request;
		SIMPLELOG << "P4Runtime StreamChannel\n";
		while (stream->Read(&request)) {
			SIMPLELOG << request.DebugString();
      			switch (request.update_case()) {
        		 case p4v1::StreamMessageRequest::kArbitration:
          		 {
            		  auto device_id = request.arbitration().device_id();
            		  if (dm.device_id != device_id) {
				SIMPLELOG << "Device ID:: " << device_id << " expected: " << dm.device_id << std::endl;
              			return Status(StatusCode::FAILED_PRECONDITION,
                                "Invalid device id");
            		  }
			  auto election_id = convert_u128(request.arbitration().election_id());
			  dm.stream = stream;
			  notify_one(stream, device_id, election_id);
              		  return Status::OK; //, "Invalid");
                         }
          		 break;
        		 case p4v1::StreamMessageRequest::kPacket:
        		 case p4v1::StreamMessageRequest::kDigestAck:
          		 {
            		 //auto device_id = connection_status.device_id;
            		 //Devices::get(device_id)->process_stream_message_request(
                	 //connection_status.connection.get(), request);
          		 }
          		 break;
        		 default:
           		 break;
      			}
    		}
    		return Status::OK;
       }

       static Uint128 convert_u128(const p4v1::Uint128 &from) {
	return Uint128(from.high(), from.low());
       }
	

};
/*
class ServerConfigServiceImpl : public p4serverv1::ServerConfig::Service {
 private:
  Status Set(ServerContext *context,
             const p4serverv1::SetRequest *request,
             p4serverv1::SetResponse *response) override {
	SIMPLELOG << "ServerConfigServiceImpl::Set\n";
    	//(void) response;
    	//auto device = Devices::get(request->device_id());
    	return Status::OK; //device->set_server_config(request->config());
  }

  Status Get(ServerContext *context,
             const p4serverv1::GetRequest *request,
             p4serverv1::GetResponse *response) override {
	SIMPLELOG << "ServerConfigServiceImpl::Set\n";
    	//auto device = Devices::get(request->device_id());
    	//response->mutable_config()->CopyFrom(device->get_server_config());
    	return Status::OK;
  }
};
*/
struct ServerData {
  std::string server_address;
  int server_port;
  P4RuntimeServiceImpl pi_service;
//  std::unique_ptr<gnmi::gNMI::Service> gnmi_service;
  //ServerConfigServiceImpl server_config_service;
  ServerBuilder builder;
  std::unique_ptr<Server> server;
};

ServerData *server_data;

extern "C" {

void PIGrpcServerRunAddrGnmi(const char *server_address, void *gnmi_service) 
{
  server_data = new ServerData();
  server_data->server_address = std::string(server_address);
  auto &builder = server_data->builder;
  builder.AddListeningPort(
    server_data->server_address, grpc::InsecureServerCredentials(),
    &server_data->server_port);
  builder.RegisterService(&server_data->pi_service);
// builder.RegisterService(server_data->gnmi_service.get());
//  builder.RegisterService(&server_data->server_config_service);
  builder.SetMaxReceiveMessageSize(256*1024*1024);  // 256MB

  server_data->server = builder.BuildAndStart();
  std::cout << "Server listening on " << server_data->server_address << "\n";

}

void PIGrpcServerRunAddr(const char *server_address) {
  PIGrpcServerRunAddrGnmi(server_address, 0);
}

void PIGrpcServerRun() {
  PIGrpcServerRunAddrGnmi("127.0.0.1:50051", 0);
}

int PIGrpcServerGetPort() {
  return server_data->server_port;
}

uint64_t PIGrpcServerGetPacketInCount(uint64_t device_id) {
  return 0;
}

uint64_t PIGrpcServerGetPacketOutCount(uint64_t device_id) {
  return 0;
}

void PIGrpcServerWait() {
  server_data->server->Wait();
}

void PIGrpcServerShutdown() {
  server_data->server->Shutdown();
}

void PIGrpcServerForceShutdown(int deadline_seconds) {
  using clock = std::chrono::system_clock;
  auto deadline = clock::now() + std::chrono::seconds(deadline_seconds);
  server_data->server->Shutdown(deadline);  
}

void PIGrpcServerCleanup() {
  delete server_data;
}

} // extern "C"
