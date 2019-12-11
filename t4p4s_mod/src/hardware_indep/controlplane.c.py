# Copyright 2016 Eotvos Lorand University, Budapest, Hungary
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

def match_type_order(t):
    match_types = {
        "exact":    0,
        "lpm":      1,
        "ternary":  2,
    }
    return match_types[t]

#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include "tables.h"

#[ #include "PI/proto/pi_server.h"
#[ #include "p4rt/device_mgr.h"

#[ #define member_size(type, member) sizeof(((type *)0)->member)

#[ extern void table_setdefault_promote  (int tableid, uint8_t* value);
#[ extern void exact_add_promote  (int tableid, uint8_t* key, uint8_t* value);
#[ extern void lpm_add_promote    (int tableid, uint8_t* key, uint8_t depth, uint8_t* value);
#[ extern void ternary_add_promote(int tableid, uint8_t* key, uint8_t* mask, uint8_t* value);

#[ extern device_mgr_t *dev_mgr_ptr;

#[ //extern struct all_metadatas_t all_metadatas;

for table in hlir16.tables:
    #[ extern void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key); // defined in dataplane.c


if len(hlir16.tables)>0:
    max_bytes = max([t.key_length_bytes for t in hlir16.tables if hasattr(t, 'key')])
    #[ uint8_t reverse_buffer[$max_bytes];

# Variable width fields are not supported
def get_key_byte_width(k):
    # for special functions like isValid
    if k.get_attr('header') is None:
        return 0
        
    return (k.width+7)/8 if not k.header.type.type_ref.is_vw else 0


hlir16_tables_with_keys = [t for t in hlir16.tables if hasattr(t, 'key')]
keyed_table_names = ", ".join(["\"T4LIT(" + table.name + ",table)\"" for table in hlir16_tables_with_keys])


for table in hlir16_tables_with_keys:
    #[ // note: ${table.name}, ${table.match_type}, ${table.key_length_bytes}
    #{ void ${table.name}_add(
    for k in table.key.keyElements:
        # TODO should properly handle specials (isValid etc.)
        if k.get_attr('header') is None:
            if k.header_name!='meta':
                continue

        if k.header_name=='meta':
            byte_width = 'member_size(struct all_metadatas_t, metafield_%s)' % k.field_name
            #[ uint8_t field_instance_${k.header_name}_${k.field_name}[$byte_width], 
        else:
            byte_width = get_key_byte_width(k)
            #[ uint8_t field_instance_${k.header.name}_${k.field_name}[$byte_width],
        
        # TODO have keys' and tables' match_type the same case (currently: LPM vs lpm)
        if k.match_type == "ternary":
            #[ uint8_t ${k.field_name}_mask[$byte_width],
        if k.match_type == "lpm":
            #[ uint8_t field_instance_${k.header.name}_${k.field_name}_prefix_length,

    #}     struct ${table.name}_action action)
    #{ {
    
    if table.key_length_bytes==0 and table.key.keyElements[0].header_name=='meta':  # TODO: Check this part!!!
         #[     uint8_t key[member_size(struct all_metadatas_t, metafield_${table.key.keyElements[0].field_name})];
    else:
         #[     uint8_t key[${table.key_length_bytes}];

    byte_idx = 0
    for k in sorted((k for k in table.key.keyElements if k.get_attr('match_type') is not None), key = lambda k: match_type_order(k.match_type)):
        # TODO should properly handle specials (isValid etc.)
        if k.get_attr('header') is None:
            if k.header_name!='meta':
                continue
        if k.header_name=='meta':
            byte_width = 'member_size(struct all_metadatas_t, metafield_%s)' % k.field_name
            #[ memcpy(key+$byte_idx, field_instance_${k.header_name}_${k.field_name}, $byte_width);
            byte_idx += 1 # Metafield size???
        else:
            byte_width = get_key_byte_width(k)
            #[ memcpy(key+$byte_idx, field_instance_${k.header.name}_${k.field_name}, $byte_width);
            byte_idx += byte_width 

    if table.match_type == "LPM":
        #[ uint8_t prefix_length = 0;
        for k in table.key.keyElements:
            # TODO should properly handle specials (isValid etc.)
            if k.get_attr('header') is None:
                continue

            if k.match_type == "exact":
                #[ prefix_length += ${get_key_byte_width(k)};
            if k.match_type == "lpm":
                #[ prefix_length += field_instance_${k.header.name}_${k.field_name}_prefix_length;
        #[ int c, d;
        #[ for(c = ${byte_idx-1}, d = 0; c >= 0; c--, d++) *(reverse_buffer+d) = *(key+c);
        #[ for(c = 0; c < ${byte_idx}; c++) *(key+c) = *(reverse_buffer+c);
        #[ lpm_add_promote(TABLE_${table.name}, (uint8_t*)key, prefix_length, (uint8_t*)&action);

    if table.match_type == "EXACT":
        #[ exact_add_promote(TABLE_${table.name}, (uint8_t*)key, (uint8_t*)&action);

    #} }

for table in hlir16.tables:
    #[ void ${table.name}_setdefault(struct ${table.name}_action action)
    #[ {
    #[     table_setdefault_promote(TABLE_${table.name}, (uint8_t*)&action);
    #[ }


# TODO is there a more appropriate source for this than the annotation?
def get_action_name_str(action):
    name_parts = action.action_object.annotations.annotations.get('name').expr[0].value
    return name_parts.rsplit(".")[-1]


for table in hlir16_tables_with_keys:
    #{ void ${table.name}_add_table_entry(struct p4_ctrl_msg* ctrl_m) {
    for i, k in enumerate(table.key.keyElements):
        # TODO should properly handle specials (isValid etc.)
        if k.get_attr('header') is None:
            if k.header_name!='meta':
                continue

        if k.match_type == "exact":
            if k.header_name=='meta':
                #[ uint8_t* field_instance_${k.header_name}_${k.field_name} = (uint8_t*)(((struct p4_field_match_exact*)ctrl_m->field_matches[${i}])->bitmap);
            else:
                #[ uint8_t* field_instance_${k.header.name}_${k.field_name} = (uint8_t*)(((struct p4_field_match_exact*)ctrl_m->field_matches[${i}])->bitmap);
        if k.match_type == "lpm":
            #[ uint8_t* field_instance_${k.header.name}_${k.field_name} = (uint8_t*)(((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->bitmap);
            #[ uint16_t field_instance_${k.header.name}_${k.field_name}_prefix_length = ((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->prefix_length;
        if k.match_type == "ternary":
            # TODO are these right?
            #[ uint8_t* field_instance_${k.header.name}_${k.field_name} = (uint8_t*)(((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->bitmap);
            #[ uint16_t field_instance_${k.header.name}_${k.field_name}_prefix_length = ((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->prefix_length;

    for action in table.actions:
        # TODO is there a more appropriate source for this than the annotation?
        action_name_str = get_action_name_str(action)
        #{ if(strcmp("$action_name_str", ctrl_m->action_name)==0) {
        #[     struct ${table.name}_action action;
        #[     action.action_id = action_${action.action_object.name};
        #[     debug("From controller: add new entry to $$[table]{table.name} with action $$[action]{action.action_object.name}\n");
        for j, p in enumerate(action.action_object.parameters.parameters):
            #[ uint8_t* ${p.name} = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[$j])->bitmap;

            if p.type('type_ref').size <= 32:
                size = 8 if p.type('type_ref').size <= 8 else 16 if p.type('type_ref').size <= 16 else 32
                #[ debug("   :: $$[field]{p.name} ($${}{%d} bits): $$[bytes]{}{%d}\n", ${p.type('type_ref').size}, *(uint${size}_t*)${p.name});
            else:
                #[ dbg_bytes(${p.name}, (${p.type('type_ref').size}+7)/8, "   :: $$[field]{p.name} ($${}{%d} bits): ", ${p.type('type_ref').size});

            #[ memcpy(action.${action.action_object.name}_params.${p.name}, ${p.name}, ${(p.type('type_ref').size+7)/8});

        #{     ${table.name}_add(
        for i, k in enumerate(table.key.keyElements):
            # TODO handle specials properly (isValid etc.)
            if k.get_attr('header') is None:
                if k.header_name!='meta':
                    continue
            if k.header_name == 'meta':
                #[ field_instance_${k.header_name}_${k.field_name},
            else:
                #[ field_instance_${k.header.name}_${k.field_name},
            if k.match_type == "lpm":
                #[ field_instance_${k.header.name}_${k.field_name}_prefix_length,
            if k.match_type == "ternary":
                #[ 0 /* TODO dstPort_mask */,
        #[     action);
        #}
        #} } else

    valid_actions = ", ".join(["\" T4LIT(" + get_action_name_str(a) + ",action) \"" for a in table.actions])
    #[ debug(" $$[warning]{}{!!!! Table add entry} on table $$[table]{table.name}: action name $$[warning]{}{mismatch}: $$[action]{}{%s}, expected one of ($valid_actions).\n", ctrl_m->action_name);
    #} }

for table in hlir16_tables_with_keys:
    #{ void ${table.name}_set_default_table_action(struct p4_ctrl_msg* ctrl_m) {
    for action in table.actions:
        action_name_str = get_action_name_str(action)
        #{ if(strcmp("$action_name_str", ctrl_m->action_name)==0) {
        #[     struct ${table.name}_action action;
        #[     action.action_id = action_${action.action_object.name};
        for j, p in enumerate(action.action_object.parameters.parameters):
            #[ uint8_t* ${p.name} = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[$j])->bitmap;
            #[ memcpy(action.${action.action_object.name}_params.${p.name}, ${p.name}, ${(p.type('type_ref').size+7)/8});
        #[     debug("From controller: set default action for $$[table]{table.name} with action $$[action]{action_name_str}\n");
        #[     ${table.name}_setdefault( action );
        #} } else

    valid_actions = ", ".join(["\" T4LIT(" + get_action_name_str(a) + ",action) \"" for a in table.actions])
    #[ debug(" $$[warning]{}{!!!! Table setdefault} on table $$[table]{table.name}: action name $$[warning]{}{mismatch} ($$[action]{}{%s}), expected one of ($valid_actions).\n", ctrl_m->action_name);
    #} }


#{ void ctrl_add_table_entry(struct p4_ctrl_msg* ctrl_m) {
for table in hlir16_tables_with_keys:
    #{ if (strcmp("${table.name}", ctrl_m->table_name) == 0) {
    #[     ${table.name}_add_table_entry(ctrl_m);
    #[     return;
    #} }
#[     debug(" $$[warning]{}{!!!! Table add entry}: table name $$[warning]{}{mismatch} ($$[table]{}{%s}), expected one of ($keyed_table_names).\n", ctrl_m->table_name);
#} }


#[ extern char* action_names[];

#{ void ctrl_setdefault(struct p4_ctrl_msg* ctrl_m) {
#[ debug("Set default message from control plane for table $$[table]{}{%s}: $$[action]{}{%s}\n", ctrl_m->table_name, ctrl_m->action_name);
for table in hlir16_tables_with_keys:
    #{ if (strcmp("${table.name}", ctrl_m->table_name) == 0) {
    #[     ${table.name}_set_default_table_action(ctrl_m);
    #[     return;
    #} }

#[     debug(" $$[warning]{}{!!!! Table setdefault}: table name $$[warning]{}{mismatch} ($$[table]{}{%s}), expected one of ($keyed_table_names).\n", ctrl_m->table_name);
#} }


#[ extern volatile int ctrl_is_initialized;
#{ void ctrl_initialized() {
#[     debug("Control plane fully initialized.\n");
#[     ctrl_is_initialized = 1;
#} }


#{ void recv_from_controller(struct p4_ctrl_msg* ctrl_m) {
#{     if (ctrl_m->type == P4T_ADD_TABLE_ENTRY) {
#[          ctrl_add_table_entry(ctrl_m);
#[     } else if (ctrl_m->type == P4T_SET_DEFAULT_ACTION) {
#[         ctrl_setdefault(ctrl_m);
#[     } else if (ctrl_m->type == P4T_CTRL_INITIALIZED) {
#[         ctrl_initialized();
#}     }
#} }



#[ ctrl_plane_backend bg;
#[ void init_control_plane()
#[ {
#[ #ifndef T4P4S_NO_CONTROL_PLANE
#[     bg = create_backend(3, 1000, "localhost", 11111, recv_from_controller);
#[     launch_backend(bg);
#[     dev_mgr_init_with_t4p4s(dev_mgr_ptr, recv_from_controller, 1);
#[     PIGrpcServerRunAddrGnmi("127.0.0.1:50051", 0);
#[     PIGrpcServerRun();
#[ #endif
#[ }
