all: l2fwd

l2fwd: l2fwd-gen.p4
	p4c-bm2-ss --arch v1model -o l2fwd-gen.json \
		--p4runtime-files l2fwd-gen_p4info.txt \
		l2fwd-gen.p4
