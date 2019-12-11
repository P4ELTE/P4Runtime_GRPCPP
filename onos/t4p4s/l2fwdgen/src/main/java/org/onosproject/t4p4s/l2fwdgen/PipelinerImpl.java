/*
 * Copyright 2018-present Open Networking Foundation
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

package org.onosproject.t4p4s.l2fwdgen;

import org.onosproject.net.DeviceId;
import org.onosproject.net.behaviour.NextGroup;
import org.onosproject.net.behaviour.Pipeliner;
import org.onosproject.net.behaviour.PipelinerContext;
import org.onosproject.net.driver.AbstractHandlerBehaviour;
import org.onosproject.net.flow.DefaultFlowRule;
import org.onosproject.net.flow.FlowRule;
import org.onosproject.net.flow.FlowRuleService;
import org.onosproject.net.flowobjective.FilteringObjective;
import org.onosproject.net.flowobjective.ForwardingObjective;
import org.onosproject.net.flowobjective.NextObjective;
import org.onosproject.net.flowobjective.ObjectiveError;
import org.slf4j.Logger;
import org.onosproject.net.pi.model.PiActionId;
import org.onosproject.net.pi.model.PiActionParamId;
import org.onosproject.net.pi.model.PiMatchFieldId;
import org.onosproject.net.pi.runtime.PiAction;
import org.onosproject.net.pi.runtime.PiActionParam;
import org.onosproject.net.flow.criteria.PiCriterion;
import org.onosproject.net.PortNumber;
import org.onlab.packet.MacAddress;
import org.onosproject.net.flow.DefaultTrafficSelector;
import org.onosproject.net.flow.DefaultTrafficTreatment;
import org.onosproject.net.pi.model.PiTableId;
import org.onosproject.core.ApplicationId;
import org.onosproject.core.CoreService;

import java.util.Collections;
import java.util.List;

import static org.slf4j.LoggerFactory.getLogger;

/*
 * Pipeliner implementation for basic.p4 that maps all forwarding objectives to
 * table0. All other types of objectives are not supported.
 */
public class PipelinerImpl extends AbstractHandlerBehaviour implements Pipeliner {

    private final Logger log = getLogger(getClass());

    private FlowRuleService flowRuleService;
    private DeviceId deviceId;
    private ApplicationId appId;


    public static final PiTableId INGRESS_TABLE_DMAC = PiTableId.of("dmac");
    public static final PiMatchFieldId HDR_ETHERNET_DST_ADDR = PiMatchFieldId.of("ethernet.dstAddr");
    public static final PiActionId FORWARD_ACTION = PiActionId.of("forward");
    public static final int DEFAULT_FLOW_RULE_PRIORITY = 10;
    public static final String PIPELINE_APP_NAME = "org.onosproject.t4p4s.l2fwdgen";

    @Override
    public void init(DeviceId deviceId, PipelinerContext context) {
        this.deviceId = deviceId;
        this.flowRuleService = context.directory().get(FlowRuleService.class);
	appId = handler().get(CoreService.class).getAppId(PIPELINE_APP_NAME);
	setupRule( PortNumber.portNumber(1, "p1"), MacAddress.valueOf("aa:bb:cc:dd:11:22"));
	setupRule( PortNumber.portNumber(1, "p1"), MacAddress.valueOf("aa:bb:cc:dd:11:33"));
    }

    public void setupRule(PortNumber port, MacAddress mac) {
	log.info("Adding L2 unicast rule on {} for mac {} (port {})...",
	          this.deviceId, mac, port);
	final PiCriterion hostMacCriterion = PiCriterion.builder()
                .matchExact(HDR_ETHERNET_DST_ADDR,
                            mac.toBytes())
                .build();

	final PiAction forwardAction = PiAction.builder()
                .withId(FORWARD_ACTION)
                .withParameter(new PiActionParam(
                        PiActionParamId.of("port"),
                        port.toLong()))
                .build();

	final FlowRule rule = DefaultFlowRule.builder()
		.forDevice(deviceId)
		.forTable(INGRESS_TABLE_DMAC)
		.fromApp(appId)
		.withPriority(DEFAULT_FLOW_RULE_PRIORITY)
		.makePermanent()
		.withSelector(DefaultTrafficSelector.builder()
                                      .matchPi(hostMacCriterion).build())
                .withTreatment(DefaultTrafficTreatment.builder()
                                       .piTableAction(forwardAction).build())
                .build();

	flowRuleService.applyFlowRules(rule);
    }

    @Override
    public void filter(FilteringObjective obj) {
        obj.context().ifPresent(c -> c.onError(obj, ObjectiveError.UNSUPPORTED));
    }

    @Override
    public void forward(ForwardingObjective obj) {
        if (obj.treatment() == null) {
            obj.context().ifPresent(c -> c.onError(obj, ObjectiveError.UNSUPPORTED));
        }

        // Simply create an equivalent FlowRule for table 0.
        final FlowRule.Builder ruleBuilder = DefaultFlowRule.builder()
                .forTable(INGRESS_TABLE_DMAC)
                .forDevice(deviceId)
                .withSelector(obj.selector())
                .fromApp(obj.appId())
                .withPriority(obj.priority())
                .withTreatment(obj.treatment());

        if (obj.permanent()) {
            ruleBuilder.makePermanent();
        } else {
            ruleBuilder.makeTemporary(obj.timeout());
        }

        switch (obj.op()) {
            case ADD:
                flowRuleService.applyFlowRules(ruleBuilder.build());
                break;
            case REMOVE:
                flowRuleService.removeFlowRules(ruleBuilder.build());
                break;
            default:
                log.warn("Unknown operation {}", obj.op());
        }

        obj.context().ifPresent(c -> c.onSuccess(obj));
    }

    @Override
    public void next(NextObjective obj) {
        obj.context().ifPresent(c -> c.onError(obj, ObjectiveError.UNSUPPORTED));
    }

    @Override
    public List<String> getNextMappings(NextGroup nextGroup) {
        // We do not use nextObjectives or groups.
        return Collections.emptyList();
    }
}
