#include "device_mgr.h"
#include "google/rpc/code.pb.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <arpa/inet.h>
#include <string>
#include <sstream>
#include <iomanip>

std::string convbytes(char* data, int len) {
	std::stringstream ss;
	for (int i=0;i<len;++i) {
		ss << std::setfill('0') << std::setw(2) << std::hex <<  (0xff & (unsigned int)data[i]) << " ";
	}
	return ss.str();
}



struct ConfigFile {
 public:
  ConfigFile() { }

  ~ConfigFile() {
    if (fp != nullptr) std::fclose(fp);
  }

  grpc::Status change_config(const ::p4::v1::ForwardingPipelineConfig &config_proto) {
    if (fp != nullptr) std::fclose(fp);  // delete old file
    fp = std::tmpfile();  // new temporary file
    if (!fp) {
	    return grpc::Status::OK; //TODO
    }
    if (config_proto.p4_device_config().size() > 0) {
      auto nb_written = std::fwrite(config_proto.p4_device_config().data(),
                                    config_proto.p4_device_config().size(),
                                    1,
                                    fp);
      /*if (nb_written != 1) {
        RETURN_ERROR_STATUS(
            Code::INTERNAL, "Error when saving config to temporary file");
      }*/
    }
    size = config_proto.p4_device_config().size();
    return grpc::Status::OK;
  }

  grpc::Status read_config(::p4::v1::ForwardingPipelineConfig *config_proto) {
    if (!fp || size == 0) return grpc::Status::OK;  // no config was saved
    if (std::fseek(fp, 0, SEEK_SET) != 0) {  // seek to start
	    return grpc::Status::OK; // TODO
      /*RETURN_ERROR_STATUS(
          Code::INTERNAL,
          "Error when reading saved config from temporary file");*/
    }
    // Unfortunately, in C++11, one cannot write directly to the std::string
    // storage (unlike in C++17), so we need an extra copy. To avoid having 2
    // copies of the config simultaneously in memory, we read the file by chunks
    // of 512 bytes.
    char buffer[512];
    auto *device_config = config_proto->mutable_p4_device_config();
    device_config->reserve(size);
    size_t iters = size / sizeof(buffer);
    size_t remainder = size - iters * sizeof(buffer);
    size_t i;
    for (i = 0; i < iters && std::fread(buffer, sizeof(buffer), 1, fp); i++) {
      device_config->append(buffer, sizeof(buffer));
    }
    if (i != iters ||
        (remainder != 0 && !std::fread(buffer, remainder, 1, fp))) {
	    return grpc::Status::OK;
      /*RETURN_ERROR_STATUS(
          Code::INTERNAL,
          "Error when reading saved config from temporary file");*/
    }
    device_config->append(buffer, remainder);
    return grpc::Status::OK;
  }

 private:
  std::FILE *fp{nullptr};
  size_t size{0};
};

ConfigFile saved_device_config;
bool has_config_cookie{false};
::p4::v1::ForwardingPipelineConfig::Cookie config_cookie;

struct p4_field_match_header* _gen_match_rule_exact(argument_t *arg, const ::p4::v1::FieldMatch::Exact &exact) {
	struct p4_field_match_exact *result = malloc(sizeof(struct p4_field_match_exact));

	strcpy(result->header.name, arg->name);
	result->header.type = P4_FMT_EXACT;
	result->length = arg->bitwidth;
	memcpy(result->bitmap, exact.value().c_str(), exact.value().size());

	return (struct p4_field_match_header*)result; /* TODO: NTOH !!! */
}

struct p4_field_match_header* _gen_match_rule_lpm(argument_t *arg, const ::p4::v1::FieldMatch::LPM &lpm) {
        struct p4_field_match_lpm *result = malloc(sizeof(struct p4_field_match_lpm));

        strcpy(result->header.name, arg->name);
        result->header.type = P4_FMT_LPM;
        result->prefix_length = lpm.prefix_len();
        memcpy(result->bitmap, lpm.value().c_str(), lpm.value().size());

        return (struct p4_field_match_header*)result; /* TODO: NTOH !!! */
}

struct p4_field_match_header* _gen_match_rule_ternary(argument_t *arg, const ::p4::v1::FieldMatch::Ternary &ternary) {
        struct p4_field_match_ternary *result = malloc(sizeof(struct p4_field_match_ternary));

        strcpy(result->header.name, arg->name);
        result->header.type = P4_FMT_TERNARY;
        result->length = arg->bitwidth;
        memcpy(result->bitmap, ternary.value().c_str(), ternary.value().size());
	memcpy(result->mask, ternary.mask().c_str(), ternary.mask().size());

        return (struct p4_field_match_header*)result; /* TODO: NTOH !!! */
}

struct p4_action_parameter* _gen_action_param(argument_t *arg, const ::p4::v1::Action::Param &param) {
	struct p4_action_parameter *result = malloc(sizeof(struct p4_action_parameter));
	uint16_t *tmp16;
	uint32_t *tmp32;

	strcpy(result->name, arg->name);
	result->length = arg->bitwidth;
	memcpy(result->bitmap, param.value().c_str(), param.value().size());
	if (param.value().size()==2) {
		tmp16 = (uint16_t*)result->bitmap;
		*tmp16 = htons(*tmp16);
	} else if (param.value().size()==4) {
		tmp32 = (uint32_t*)result->bitmap;
		*tmp32 = htonl(*tmp32);
        }

	return result; /* TODO: NTOH  */
}

grpc::Status table_insert(device_mgr_t *dm, const ::p4::v1::TableEntry &table_entry) {
	uint32_t table_id;
	uint32_t field_id;
	uint32_t action_id;
	uint16_t value16; /* TODO: remove after testing */
	uint8_t ip[4]; /* TODO: remove after testing */
	size_t i;
	int32_t prefix_len = 0; /* in bits */


	grpc::Status status = grpc::Status::OK;
//	::p4::v1::FieldMatch match;
//	::p4::v1::TableAction action;
//	::p4::v1::FieldMatch::Exact exact;
//	::p4::v1::FieldMatch::LPM lpm;
//	::p4::v1::FieldMatch::Ternary ternary;
	
//	::p4::v1::Action tmp_act;
//	::p4::v1::Action::Param param;
	
	table_id = table_entry.table_id();

	element_t *elem = get_element(&(dm->id_map), table_id);
	argument_t *arg = NULL;

	struct p4_ctrl_msg ctrl_m;
	ctrl_m.num_field_matches = 0;
	ctrl_m.num_action_params = 0;
	ctrl_m.type = P4T_ADD_TABLE_ENTRY;
	ctrl_m.table_name = strdup(elem->value);

	for (const auto &match: table_entry.match()) {
		field_id = match.field_id();
		arg = get_argument(elem, field_id);
		if (arg==NULL) {
			printf("NULL ARGUMENT for FIELD_ID=%d\n", field_id);
		}
		switch(match.field_match_type_case()) {
			case ::p4::v1::FieldMatch::FieldMatchTypeCase::kExact: {
				const auto exact = match.exact();
				ctrl_m.field_matches[ctrl_m.num_field_matches] = _gen_match_rule_exact(arg, exact);
				printf("EXACT MATCH TableID:%d (%s) FieldID:%d (%s) KEY_LENGTH:%d VALUE: %s -- \n", table_id, elem->value, field_id, arg->name, exact.value().size(), convbytes(exact.value().c_str(), (int)exact.value().size()).c_str() );
				ctrl_m.num_field_matches++;
				status = grpc::Status::OK;	
				//status.set_code(Code::OK);
				break; }
			case ::p4::v1::FieldMatch::FieldMatchTypeCase::kLpm: {
				printf(" -- LPM\n");
				const auto lpm = match.lpm();
				prefix_len = lpm.prefix_len();
                                if (lpm.value().size()>=4) {
					ip[0] = lpm.value().c_str()[0];
					ip[1] = lpm.value().c_str()[1];
					ip[2] = lpm.value().c_str()[2];
					ip[3] = lpm.value().c_str()[3];
				}
				printf("LPM MATCH TableID:%:%d (%s) FieldID:%d (%s) KEY_LENGTH:%d VALUE_IP: %d.%d.%d.%d PREFIX_LEN: %d  -- \n", table_id, elem->value, field_id, arg->name, lpm.value().size(), (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3],  prefix_len);
				ctrl_m.field_matches[ctrl_m.num_field_matches] = _gen_match_rule_lpm(arg, lpm);
				ctrl_m.num_field_matches++;
				status = grpc::Status::OK;
				break; }
			case ::p4::v1::FieldMatch::FieldMatchTypeCase::kTernary: {
				const auto ternary = match.ternary();
				printf("TERNARY MATCH TableID:%d (%s) FieldID:%d (%s) KEY_LENGTH:%d VALUE16: %d M_LEN:%d MASK:%d  --\n", table_id, elem->value, field_id, arg->name, ternary.value().size(), ternary.value().c_str()[0], ternary.mask().size(), ternary.mask().c_str()[0]); /* len - length , data - uint8_t* */
				ctrl_m.field_matches[ctrl_m.num_field_matches] = _gen_match_rule_ternary(arg, ternary);
				ctrl_m.num_field_matches++;
                                //status.gcs_code = GOOGLE__RPC__CODE__OK;
                                break;}

			//case ::p4::v1::FieldMatch::FieldMatchTypeCase::kRange:	
			default: {
				printf("unimplemented\n");
				status = grpc::Status( grpc::StatusCode::UNIMPLEMENTED, "MatchType is not implemented" );
				break;}
		}
	}
//return status; } //
	if (table_entry.is_default_action()) { /* n_match is 0 in this case */
		ctrl_m.type = P4T_SET_DEFAULT_ACTION;
	}

	const auto action = table_entry.action();

	switch(action.type_case()) {
		case ::p4::v1::TableAction::TypeCase::kAction:
			ctrl_m.action_type = P4_AT_ACTION; /* ACTION PROFILE IS NOT SUPPORTED */
			const auto tmp_act = action.action();
			action_id = tmp_act.action_id();
			elem = get_element(&(dm->id_map), action_id);
			if (elem == NULL) {
				status = grpc::Status( grpc::StatusCode::INVALID_ARGUMENT, "Invalid ActionId" );
				break;
			}
			ctrl_m.action_name = strdup(elem->value);
			for (const auto &param: tmp_act.params()) {
				//param = tmp_act->params[i];
				arg = get_argument(elem, param.param_id());
				ctrl_m.action_params[ctrl_m.num_action_params] = _gen_action_param(arg, param);
				ctrl_m.num_action_params++;
			}
			//status.gcs_code = GOOGLE__RPC__CODE__OK;
			break;
		default:
			status = grpc::Status( grpc::StatusCode::UNIMPLEMENTED, "ActionType is not implemented" );
			break;
	}

	if (status.ok()) {
		dm->cb(&ctrl_m);
	}

        return status;
}

grpc::Status table_modify(device_mgr_t *dm, const ::p4::v1::TableEntry &table_entry) {
	grpc::Status status;
	status = grpc::Status( grpc::StatusCode::UNIMPLEMENTED, "table_modify is not implemented" );
	return status;
}

grpc::Status table_delete(device_mgr_t *dm, const ::p4::v1::TableEntry &table_entry) {
	grpc::Status status;
	status = grpc::Status( grpc::StatusCode::UNIMPLEMENTED, "table_delete is not implemented" );
        return status;
}

bool check_p4_id(uint32_t id, int type) {
	return true;
}

grpc::Status table_write(device_mgr_t *dm, ::p4::v1::Update::Type update, const ::p4::v1::TableEntry &table_entry) {
	grpc::Status status;
	if (!check_p4_id(table_entry.table_id(), P4IDS_TABLE)) {
		status = grpc::Status( grpc::StatusCode::UNKNOWN, "P4ID is not unknown" );
		return status; /*TODO: more informative error msg is needed!!!*/
	}

	switch (update) {
		case ::p4::v1::Update::UNSPECIFIED: {
			status = grpc::Status( grpc::StatusCode::INVALID_ARGUMENT, "Invalid argument" );
			/*TODO: more informative error msg is needed!!!*/
	        	break; }
		case ::p4::v1::Update::INSERT: {
			return table_insert(dm, table_entry);}
		case ::p4::v1::Update::MODIFY: {
			return table_modify(dm, table_entry);}
		case ::p4::v1::Update::DELETE: {
			return table_delete(dm, table_entry);}
		default: {
			status = grpc::Status( grpc::StatusCode::UNKNOWN, "Unknown update message" );
			/*TODO: more informative error msg is needed!!!*/
			break;}
	}
	return status;
}

grpc::Status counter_write(device_mgr_t *dm, ::p4::v1::Update::Type update, const ::p4::v1::CounterEntry &counter_entry) {
        grpc::Status status;
        if (!check_p4_id(counter_entry.counter_id(), P4IDS_COUNTER)) {
                status = grpc::Status( grpc::StatusCode::UNKNOWN, "P4ID is not unknown" );
                return status; /*TODO: more informative error msg is needed!!!*/
        }
	if (!counter_entry.has_index()) {
		status = grpc::Status( grpc::StatusCode::UNIMPLEMENTED, "Wildcard write is not supported" );
		return status;
	}
	if (counter_entry.index().index() < 0) {
		status = grpc::Status( grpc::StatusCode::UNIMPLEMENTED, "A negative number is not a valid index value." );
		return status;
	}

	auto index = static_cast<size_t>(counter_entry.index().index());

	switch(update) {
		case ::p4::v1::Update::MODIFY:
		case ::p4::v1::Update::DELETE:
		default:
			status = grpc::Status( grpc::StatusCode::UNKNOWN, "Unknown update message" );
                        /*TODO: more informative error msg is needed!!!*/
                        break;

	}

	return status; 
}

void counter_read_one(uint32_t* counter_values_bytes, uint32_t size_bytes, uint32_t* counter_values_packets, uint32_t size_packets, int index, ::p4::v1::ReadResponse *response) {
	auto entry = response->add_entities()->mutable_counter_entry();
	if (counter_values_bytes != 0x0 && size_bytes > 0){
		entry->mutable_data()->set_byte_count(counter_values_bytes[index]);
		printf("COUNTER VALUE BYTES: %d\n", counter_values_bytes[index]);
	}
	if (counter_values_packets != 0x0 && size_packets > 0){
		entry->mutable_data()->set_packet_count(counter_values_packets[index]);
		printf("COUNTER VALUE PACKETS: %d\n", counter_values_packets[index]);
	}
}

grpc::Status counter_read(device_mgr_t *dm, const ::p4::v1::CounterEntry &counter_entry, ::p4::v1::ReadResponse *response) {
        grpc::Status status;
        if (!check_p4_id(counter_entry.counter_id(), P4IDS_COUNTER) && counter_entry.counter_id()!=0) {
                status = grpc::Status( grpc::StatusCode::UNKNOWN, "P4ID is not unknown" );
                return status; /*TODO: more informative error msg is needed!!!*/
        }
	
	auto counter_id = counter_entry.counter_id();
	element_t *elem = get_element(&(dm->id_map), counter_id);
	int size_bytes   = -1;
	int size_packets = -1;
	uint32_t* counter_values_bytes   = dm->get_counter_by_name(elem->value, &size_bytes, true);
	uint32_t* counter_values_packets = dm->get_counter_by_name(elem->value, &size_packets, false);
	if ((counter_values_bytes == 0x0 || size_bytes == -1) && (counter_values_packets == 0x0 || size_packets == -1)){
		return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Counter cannot be found %s",elem->value );
	}
	if (!counter_entry.has_index()) {
		/*TODO: read one element as in https://github.com/p4lang/PI/blob/master/proto/frontend/src/device_mgr.cpp*/
		if (counter_entry.index().index() < 0) {
			status = grpc::Status( grpc::StatusCode::UNKNOWN, "counter index negative" );
                	return status;
		}
		auto index = static_cast<size_t>(counter_entry.index().index());
		counter_read_one(counter_values_bytes, size_bytes, counter_values_packets, size_packets, index, response);
		status = grpc::Status::OK;
	        return status;
	} 

	/* TODO: read all elements as in https://github.com/p4lang/PI/blob/master/proto/frontend/src/device_mgr.cpp */

	for (int index = 0; index < size_packets && index < size_bytes; index++){
		counter_read_one(counter_values_bytes, size_bytes, counter_values_packets, size_packets, index, response);
	}
	status = grpc::Status::OK;
        return status;
}



grpc::Status direct_counter_write(device_mgr_t *dm, ::p4::v1::Update::Type update, const ::p4::v1::TableEntry &table_entry) {
        grpc::Status status;
        if (!check_p4_id(table_entry.table_id(), P4IDS_TABLE)) {
                status = grpc::Status( grpc::StatusCode::UNKNOWN, "P4ID is not unknown" );
                return status; /*TODO: more informative error msg is needed!!!*/
        }

	status = grpc::Status( grpc::StatusCode::UNIMPLEMENTED, "Direct counters are not supported." );

	return status;
}

// Copied from p4lang/PI
class P4ErrorReporter {
 public:
  void push_back(const ::p4::v1::Error &error) {
    //if (!error.canonical_code().ok())
    //  errors.emplace_back(index, error);
    //index++;
  }

  // TODO(antonin): remove this overload when we generalize the use of
  // p4v1::Error in the code?
  void push_back(const grpc::Status &status) {
    //if (!status.ok()) {
    //  ::p4::v1::Error error;
      //error.set_canonical_code(status.error_code());
      //error.set_message(status.message());
      //error.set_space("ALL-sswitch-p4org");
      //errors.emplace_back(index, error);
    //}
    //index++;
  }

  grpc::Status get_status() const {
    grpc::Status status;
    if (errors.empty()) {
      status = grpc::Status::OK;
    } else {
      status = grpc::Status(grpc::StatusCode::UNKNOWN, "Unknown");
/*      ::p4::v1::Error success;
      success.set_code(Code::OK);
      status.set_code(Code::UNKNOWN);
      size_t i = 0;
      for (const auto &p : errors) {
        for (; i++ < p.first;) {
          auto success_any = status.add_details();
          success_any->PackFrom(success);
        }
        auto error_any = status.add_details();
        error_any->PackFrom(p.second);
      }
      // add trailing OKs
      for (; i++ < index;) {
        auto success_any = status.add_details();
        success_any->PackFrom(success);
      }*/
    }
    return status;
  }

 private:
  std::vector<std::pair<size_t, ::p4::v1::Error> > errors{};
  size_t index{0};
};


grpc::Status dev_mgr_write(device_mgr_t *dm, const ::p4::v1::WriteRequest &request, ::p4::v1::WriteResponse *response) {
	grpc::Status status = grpc::Status::OK;
	size_t i;

	if (request.atomicity() != ::p4::v1::WriteRequest::CONTINUE_ON_ERROR) {
		status = grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not implemented");
		return status;
	}

	P4ErrorReporter error_reporter;

	for (const auto &update : request.updates()) {
		const auto entity = update.entity();
		switch(entity.entity_case()) {
			case ::p4::v1::Entity::kTableEntry: {
				status = table_write(dm, update.type(), entity.table_entry());
				break; }
			case ::p4::v1::Entity::kCounterEntry: {
				status = counter_write(dm, update.type(), entity.counter_entry());
				break; }
			case ::p4::v1::Entity::kDirectCounterEntry: {
				/*status = direct_counter_write(dm, update.type(), entity.direct_counter_entry());*/
				break; }
			case ::p4::v1::Entity::kRegisterEntry:
			case ::p4::v1::Entity::kDigestEntry: /*TODO: Check why?*/
			default: {
				status = grpc::Status(grpc::StatusCode::UNKNOWN, "Entity case is unknown");
				break; }
		}
		/* TODO:collect multiple status messages - now we assume a simple update */
		error_reporter.push_back(status);
	}
	
	return error_reporter.get_status();
}

grpc::Status dev_mgr_read_one(device_mgr_t *dm, const ::p4::v1::Entity &entity, ::p4::v1::ReadResponse *response) {
        grpc::Status status(grpc::StatusCode::OK, "ok");

	switch(entity.entity_case()) {
		case ::p4::v1::Entity::kCounterEntry: {
			status = counter_read(dm, entity.counter_entry(), response);
			break; }
		default: {
			status = grpc::Status(grpc::StatusCode::UNKNOWN, "Entity case is unknown");
                        break; }
	}
	return status;
}


grpc::Status dev_mgr_read(device_mgr_t *dm, const ::p4::v1::ReadRequest &request, ::p4::v1::ReadResponse *response) {
	grpc::Status status(grpc::StatusCode::OK, "ok");
	for (const auto &entity : request.entities()) {
		status = dev_mgr_read_one(dm, entity, response);
		if (!status.ok()) break;
	}

	return grpc::Status::OK;
}



grpc::Status dev_mgr_set_pipeline_config(device_mgr_t *dm, ::p4::v1::SetForwardingPipelineConfigRequest_Action action, const ::p4::v1::ForwardingPipelineConfig config) {
	using SetConfigRequest = ::p4::v1::SetForwardingPipelineConfigRequest;
	grpc::Status status(grpc::StatusCode::OK, "ok");

	if (action == SetConfigRequest::VERIFY ||
        action == SetConfigRequest::VERIFY_AND_SAVE ||
        action == SetConfigRequest::VERIFY_AND_COMMIT ||
        action == SetConfigRequest::RECONCILE_AND_COMMIT) {
		printf("P4Info configuration received...\n");
		dm->p4info.CopyFrom( config.p4info() );
		dm->has_p4info = 1;
		config_cookie.CopyFrom(config.cookie());
		saved_device_config.change_config(config); // TODO: return value
		has_config_cookie = true;
		element_t *elem;
		size_t i,j;
		for (const auto &table : dm->p4info.tables()) {
			const auto &pre = table.preamble();
			printf("  [+] TABLE id: %d; name: %s\n", pre.id(), pre.name().c_str());
			elem = add_element(&(dm->id_map), pre.id(), pre.name().c_str());
	                if (elem == NULL) {
        	                printf("   +-----> ERROR\n");
                	        break;
                	}
			for (const auto &mf : table.match_fields()) {
				printf("   +-----> MATCH FIELD; name: %s; id: %d; bitwidth: %d\n", mf.name().c_str(), mf.id(), mf.bitwidth());
				strcpy(elem->args[elem->n_args].name, mf.name().c_str());
	                        elem->args[elem->n_args].id = mf.id();
        	                elem->args[elem->n_args].bitwidth = mf.bitwidth();
                	        elem->n_args++;
			}

                }
		for (const auto &taction : dm->p4info.actions()) {
			const auto &pre = taction.preamble();
			printf("  [#] ACTION id: %d; name: %s\n", pre.id(), pre.name().c_str());
                	elem = add_element(&(dm->id_map), pre.id(), pre.name().c_str());
                	for (const auto &param : taction.params()) {
                        	printf("   #-----> ACTION PARAM; name: %s; id: %d; bitwidth: %d\n", param.name().c_str(), param.id(), param.bitwidth());
                        	strcpy(elem->args[elem->n_args].name, param.name().c_str());
                        	elem->args[elem->n_args].id = param.id();
	                        elem->args[elem->n_args].bitwidth = param.bitwidth();
        	                elem->n_args++;
                	}
		}
                for (const auto &counter : dm->p4info.counters()) {
                        const auto &pre = counter.preamble();
                        printf("  [#] COUNTER id: %d; name: %s\n", pre.id(), pre.name().c_str());
                        elem = add_element(&(dm->id_map), pre.id(), pre.name().c_str());
                }


		return status;
	}

	return status; // TODO
}

grpc::Status dev_mgr_get_pipeline_config(device_mgr_t *dm, ::p4::v1::GetForwardingPipelineConfigRequest::ResponseType response_type, ::p4::v1::ForwardingPipelineConfig *config) {
	using GetConfigRequest = ::p4::v1::GetForwardingPipelineConfigRequest;
	switch (response_type) {
      		case GetConfigRequest::ALL: {
        		config->mutable_p4info()->CopyFrom(dm->p4info);
        		saved_device_config.read_config(config);
        		break;}
      		case GetConfigRequest::COOKIE_ONLY: {
        		break;}
      		case GetConfigRequest::P4INFO_AND_COOKIE: {
        		config->mutable_p4info()->CopyFrom(dm->p4info);
        		break;}
      		case GetConfigRequest::DEVICE_CONFIG_AND_COOKIE: {
        		saved_device_config.read_config(config);
        		break;}
      		default: {
			return grpc::Status::OK;}
    	}
	if (has_config_cookie)
      		config->mutable_cookie()->CopyFrom(config_cookie);
	return grpc::Status::OK;
}

void dev_mgr_init(device_mgr_t *dm) {
	init_map(&(dm->id_map));
	dm->has_p4info = 0;
}

void dev_mgr_init_with_t4p4s(device_mgr_t *dm, p4_msg_callback cb, p4_cnt_read get_counter_by_name, uint64_t device_id) {
	dev_mgr_init(dm);
	dm->cb = cb;
	dm->get_counter_by_name = get_counter_by_name;
	dm->device_id = device_id;
}

void dev_mgr_send_digest(device_mgr_t *dm, struct p4_digest* digest, uint64_t digest_id) {
	::p4::v1::StreamMessageResponse response;
	::p4::v1::DigestList _digest{};
	::p4::v1::P4Data* data;
	struct p4_digest_field* df = (struct p4_digest_field*)((void*)digest + sizeof(struct p4_digest));
	_digest.set_digest_id(digest_id);
	_digest.set_list_id(1);
	size_t bytes = 0;
	data = _digest.add_data();
	auto *struct_like = data->mutable_tuple();
	for (int i=0;i<digest->list_size;++i) {
		bytes = (df->length+7)/8;
		struct_like->add_members()->set_bitstring(df->value, bytes);
		df++;
	}

	response.set_allocated_digest(&_digest);
	auto succ = dm->stream->Write(response);

	response.release_digest();
	//_digest.set_list_id(_digest.list_id() + 1);
	_digest.clear_data();
}
