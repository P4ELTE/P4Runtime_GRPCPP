#!/bin/bash

DEBUG=""

if [ "$1" = "-D" ];then
	DEBUG="-g"
fi

GRPCDIR="../grpc-c/build"

T4P4SDIR="../t4p4s"

#LFLAGS="${GRPCDIR}/lib/libgrpc-c.la -lgrpc -lgpr -lprotobuf-c -lpthread"
LFLAGS="-lgrpc -lgpr -lprotobuf-c -lpthread"



#IFLAGS="-I. -I${GRPCDIR}/lib/h/ -I${GRPCDIR}/third_party/protobuf-c -I${GRPCDIR}/third_party/grpc/include -I./grpc-c-out/ -I./server/ -I../PI/proto/server -I${T4P4SDIR}/src/hardware_dep/shared/ctrl_plane"
IFLAGS="-I. -I./grpc-cpp-out/ -I./cpp-out/ -I./server/ -I./${T4P4SDIR}/src/hardware_dep/shared/ctrl_plane -I./../PI/proto/server"

#PROTO_SOURCES="grpc-c-out/p4/tmp/p4config.grpc-c.service.c grpc-c-out/p4/tmp/p4config.grpc-c.c grpc-c-out/p4/v1/p4data.grpc-c.service.c grpc-c-out/p4/v1/p4runtime.grpc-c.service.c grpc-c-out/p4/v1/p4data.grpc-c.c grpc-c-out/p4/v1/p4runtime.grpc-c.c grpc-c-out/p4/config/v1/p4types.grpc-c.c grpc-c-out/p4/config/v1/p4types.grpc-c.service.c grpc-c-out/p4/config/v1/p4info.grpc-c.service.c grpc-c-out/p4/config/v1/p4info.grpc-c.c grpc-c-out/google/rpc/status.grpc-c.service.c grpc-c-out/google/rpc/status.grpc-c.c grpc-c-out/google/rpc/code.grpc-c.c grpc-c-out/google/rpc/code.grpc-c.service.c grpc-c-out/gnmi/gnmi.grpc-c.c grpc-c-out/gnmi/gnmi.grpc-c.service.c grpc-c-out/google/protobuf/any.grpc-c.c grpc-c-out/google/protobuf/descriptor.grpc-c.c"

PROTO_PB_S="cpp-out/p4/v1/p4data.pb.cc cpp-out/p4/v1/p4runtime.pb.cc cpp-out/p4/config/v1/p4info.pb.cc cpp-out/p4/config/v1/p4types.pb.cc cpp-out/p4/tmp/p4config.pb.cc cpp-out/google/rpc/status.pb.cc cpp-out/google/rpc/code.pb.cc cpp-out/gnmi/gnmi.pb.cc"

PROTO_SOURCES="grpc-cpp-out/p4/v1/p4data.grpc.pb.cc grpc-cpp-out/p4/v1/p4runtime.grpc.pb.cc grpc-cpp-out/p4/config/v1/p4info.grpc.pb.cc grpc-cpp-out/p4/config/v1/p4types.grpc.pb.cc grpc-cpp-out/p4/tmp/p4config.grpc.pb.cc grpc-cpp-out/google/rpc/status.grpc.pb.cc grpc-cpp-out/google/rpc/code.grpc.pb.cc grpc-cpp-out/gnmi/gnmi.grpc.pb.cc"

rm -rf obj
mkdir -p obj

OBJECTS=""

for src in $PROTO_PB_S
do
        echo Compiling $src
        obj=`echo $src | sed 's/c$/o/' | sed 's/^.*\///g'`
        g++ ${DEBUG} ${IFLAGS} -std=c++11 `pkg-config --cflags protobuf grpc` -c -o ./obj/${obj} ${src}
	OBJECTS="$OBJECTS ./obj/${obj}"
#       gcc ${DEBUG} ${IFLAGS} -c -o ./obj/${obj} ${src}
done


for src in $PROTO_SOURCES
do
	echo Compiling $src
	obj=`echo $src | sed 's/c$/o/' | sed 's/^.*\///g'`
	g++ ${DEBUG} ${IFLAGS} -std=c++11 `pkg-config --cflags protobuf grpc` -c -o ./obj/${obj} ${src}
	OBJECTS="$OBJECTS ./obj/${obj}"
#	gcc ${DEBUG} ${IFLAGS} -c -o ./obj/${obj} ${src}
done


echo "Compiling map.c"
g++ ${DEBUG} ${IFLAGS} -std=c++11 -fpermissive `pkg-config --cflags protobuf grpc` -c -o obj/map.o utils/map.c

echo "Compiling uint128.cpp"
g++ ${DEBUG} ${IFLAGS} -std=c++11 `pkg-config --cflags protobuf grpc` -c -o obj/uint128.o server/uint128.cpp

echo "Compiling device_mgr.c"
#gcc ${DEBUG} ${IFLAGS} -c -o obj/device_mgr.o server/device_mgr.c
g++ ${DEBUG} ${IFLAGS} -std=c++11 -fpermissive `pkg-config --cflags protobuf grpc` -c -o obj/device_mgr.o server/device_mgr.c

#gcc ${DEBUG} ${IFLAGS} -c -o obj/pi_server.o server/pi_server.c
echo "Compiling pi_server.cc"
g++ ${DEBUG} ${IFLAGS} -std=c++11 `pkg-config --cflags protobuf grpc` -c -o obj/pi_server.o server/pi_server.cc

echo "Compiling test_server.c"
g++ ${DEBUG} ${IFLAGS} -std=c++11 `pkg-config --cflags protobuf grpc` -c -o test_server.o server/test_server.c

echo "Linking test_server"
#g++ helloworld.pb.o helloworld.grpc.pb.o greeter_client.o -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o greeter_client
echo "g++ ${DEBUG} ${IFLAGS} -o pi_server ${OBJECTS} obj/map.o obj/uint128.o obj/device_mgr.o obj/pi_server.o test_server.o -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc` -pthread -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl"
g++ ${DEBUG} ${IFLAGS} -o pi_server ${OBJECTS} obj/map.o obj/uint128.o obj/device_mgr.o obj/pi_server.o test_server.o -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc` -pthread -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl

echo "Creating static lib"
mkdir -p static_lib
ar rcs static_lib/libp4rt.a obj/*.o

rm -rf include
mkdir -p include
mkdir -p include/p4rt
mkdir -p include/utils
cp server/device_mgr.h include/p4rt/
cp server/config.h include/
cp utils/map.h include/utils/


echo "Static library is available in folder ./static_lib"
echo "Usage example:"
echo "gcc ${IFLAGS} server/test_server.c -L./static_lib -lp4rt -L${GRPCDIR}/lib/.libs -lgrpc-c ${LFLAGS} -o test_server"

