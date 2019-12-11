COMPILE_DEPS = CORE_DEPS + [
    "@minimal_json//jar",
    "//protocols/p4runtime/model:onos-protocols-p4runtime-model",
    "//protocols/p4runtime/api:onos-protocols-p4runtime-api",
    "//drivers/default:onos-drivers-default",
]

osgi_jar(
    deps = COMPILE_DEPS,
)

BUNDLES = [
    "//apps/t4p4s/l2fwdgen:onos-apps-t4p4s-l2fwdgen",
]

onos_app(
    app_name = "org.onosproject.t4p4s.l2fwdgen",
    category = "Pipeconf",
    description = "Provides pipeconf for T4P4S",
    included_bundles = BUNDLES,
    required_apps = [
        "org.onosproject.drivers.p4runtime",
    ],
    title = "T4P4S",
    url = "http://t4p4s.org",
)
