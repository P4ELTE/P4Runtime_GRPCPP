# P4Runtime API for T4P4S - based on GRPCPP

P4Runtime support od T4P4S is under development. This repo will contain the source files needed for the interaction with P4Runtime-capable control planes. Currently, the source codes in this repo are experimental!!!

## Dependencies

* grpcpp
* P4 PI: https://github.com/p4lang/PI
* T4P4S (only 2 header files are needed): https://github.com/P4ELTE/t4p4s

P4 PI was configured with the following arguments:
```
./configure --with-proto --without-internal-rpc --without-cli
```

## P4Runtime GRPC-CPP stubs
After you managed to compile P4 PI, all the proto files needed are generated.
Update the PLUGIN, OUTDIR and PSRC variables in  ./install.sh, and execute it. The P4Runtime stub code is then generated into the folder OUTDIR.

## Compilation of source files and an example server
Update the GRPCDIR and T4P4SDIR in ./compile.sh and execute it:
```
./compile.sh
```

For debugging (with gdb) you can use "-D" option:
```
./compile.sh -D
```
## Run the example server
Currently pi_server is bound to localhost:50051
```
sudo ./pi_server
```

## Connecting to the switch and filling a table from ONOS Control Plane
First, you should copy onos/t4p4s folder into the apps folder of your ONOS installation (for testing we assume it is available on the same host as T4P4S).

Then you can build the T4P4S app in the main folder of your ONOS installation:
```
bazel build onos
```

ONOS can then be launched by the following command
```
bazel run onos-local -- clean
```

In another terminal the T4P4S application can be activated in the ONOS shell:
```
onos localhost

xyz@xyz > app activate org.onosproject.t4p4s.l2fwdgen
xyz@xyz > apps -s -a
xyz@xyz > logout
```

The netconf file should then be loaded to make the switch visible for ONOS.
```
onos-netcfg localhost <gitrepo>\onos\netcfg.json
```

After that ONOS connects to the pi_server if it is running and send entries to dmac table.
The interaction with the pi_server can be seen in the ONOS log:
```
onos localhost

xyz@xyz > log:tail
Ctrl+c
xyz@xyz > logout
```

## Compiling with T4P4S
Copy the content of t4p4s_mod into the t4p4s folder. It overwrites the makefiles and the control plane API generator part.
It may happen that the paths in the makefiles should be updated.

After that T4P4S can be compiled and executed by, for example, 
```
./t4p4s.sh :l2fwd-gen
```



