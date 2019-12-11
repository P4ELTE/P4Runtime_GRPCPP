#!/bin/bash
OUTDIR2=./cpp-out
OUTDIR=./grpc-cpp-out
PLUGIN=grpc_cpp_plugin
#../grpc-c/build/compiler/protoc-gen-grpc-c
PSRC=../PI/proto

rm -rf $OUTDIR2
rm -rf $OUTDIR

mkdir $OUTDIR2
mkdir $OUTDIR

read -r -d '' PROTOS <<- EOM
${PSRC}/p4/v1/p4data.proto
${PSRC}/p4/v1/p4runtime.proto
${PSRC}/p4/config/v1/p4info.proto
${PSRC}/p4/config/v1/p4types.proto
${PSRC}/google/rpc/status.proto
${PSRC}/google/rpc/code.proto
${PSRC}/p4/tmp/p4config.proto
${PSRC}/gnmi/gnmi.proto
EOM

mkdir -p $OUTDIR

for protofile in $PROTOS
do
	echo Processing: ${protofile}
	protoc -I $PSRC --cpp_out ${OUTDIR2} ${protofile}
	protoc -I $PSRC --grpc_out=${OUTDIR} --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ${protofile}
done

#protoc -I ../grpc-c/third_party/protobuf/src --grpc-c_out=${OUTDIR} --plugin=protoc-gen-grpc-c=${PLUGIN} ../grpc-c/third_party/protobuf/src/google/protobuf/any.proto

#protoc -I ../grpc-c/third_party/protobuf/src --grpc-c_out=${OUTDIR} --plugin=protoc-gen-grpc-c=${PLUGIN} ../grpc-c/third_party/protobuf/src/google/protobuf/descriptor.proto
