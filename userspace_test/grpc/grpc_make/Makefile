target := mm_monitor


#protoc --proto_path=proto        --cpp_out=.        --grpc_out=.        --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin        proto/greeter.protoprot

SRCS = greeter.grpc.pb.cc  greeter.pb.cc  server.cc

LIBS :=  /usr/local/lib/libgrpc++.a /usr/local/lib/libgrpc.a /usr/local/lib64/libprotobuf.a /usr/local/lib/libupb_collections_lib.a /usr/local/lib/libupb_json_lib.a /usr/local/lib/libupb_textformat_lib.a /usr/local/lib/libutf8_range_lib.a /usr/local/lib/libupb.a /usr/local/lib/libre2.a /usr/local/lib/libz.a /usr/local/lib/libcares.a /usr/local/lib/libgpr.a /usr/local/lib64/libabsl_random_distributions.a /usr/local/lib64/libabsl_random_seed_sequences.a /usr/local/lib64/libabsl_random_internal_pool_urbg.a /usr/local/lib64/libabsl_random_internal_randen.a /usr/local/lib64/libabsl_random_internal_randen_hwaes.a /usr/local/lib64/libabsl_random_internal_randen_hwaes_impl.a /usr/local/lib64/libabsl_random_internal_randen_slow.a /usr/local/lib64/libabsl_random_internal_platform.a /usr/local/lib64/libabsl_random_internal_seed_material.a /usr/local/lib64/libabsl_random_seed_gen_exception.a /usr/local/lib/libssl.a /usr/local/lib/libcrypto.a /usr/local/lib/libaddress_sorting.a /usr/local/lib64/libabsl_statusor.a /usr/local/lib64/libabsl_log_internal_check_op.a /usr/local/lib64/libabsl_leak_check.a /usr/local/lib64/libabsl_die_if_null.a /usr/local/lib64/libabsl_log_internal_conditions.a /usr/local/lib64/libabsl_log_internal_message.a /usr/local/lib64/libabsl_log_internal_nullguard.a /usr/local/lib64/libabsl_examine_stack.a /usr/local/lib64/libabsl_log_internal_format.a /usr/local/lib64/libabsl_log_internal_proto.a /usr/local/lib64/libabsl_log_internal_log_sink_set.a /usr/local/lib64/libabsl_log_sink.a /usr/local/lib64/libabsl_log_entry.a /usr/local/lib64/libabsl_flags.a /usr/local/lib64/libabsl_flags_internal.a /usr/local/lib64/libabsl_flags_marshalling.a /usr/local/lib64/libabsl_flags_reflection.a /usr/local/lib64/libabsl_flags_config.a /usr/local/lib64/libabsl_flags_program_name.a /usr/local/lib64/libabsl_flags_private_handle_accessor.a /usr/local/lib64/libabsl_flags_commandlineflag.a /usr/local/lib64/libabsl_flags_commandlineflag_internal.a /usr/local/lib64/libabsl_log_initialize.a /usr/local/lib64/libabsl_log_globals.a /usr/local/lib64/libabsl_log_internal_globals.a /usr/local/lib64/libabsl_raw_hash_set.a /usr/local/lib64/libabsl_hash.a /usr/local/lib64/libabsl_city.a /usr/local/lib64/libabsl_low_level_hash.a /usr/local/lib64/libabsl_hashtablez_sampler.a /usr/local/lib64/libabsl_status.a /usr/local/lib64/libabsl_cord.a /usr/local/lib64/libabsl_cordz_info.a /usr/local/lib64/libabsl_cord_internal.a /usr/local/lib64/libabsl_cordz_functions.a /usr/local/lib64/libabsl_exponential_biased.a /usr/local/lib64/libabsl_cordz_handle.a /usr/local/lib64/libabsl_crc_cord_state.a /usr/local/lib64/libabsl_crc32c.a /usr/local/lib64/libabsl_crc_internal.a /usr/local/lib64/libabsl_crc_cpu_detect.a /usr/local/lib64/libabsl_bad_optional_access.a /usr/local/lib64/libabsl_str_format_internal.a /usr/local/lib64/libabsl_strerror.a /usr/local/lib64/libabsl_synchronization.a /usr/local/lib64/libabsl_stacktrace.a /usr/local/lib64/libabsl_symbolize.a /usr/local/lib64/libabsl_debugging_internal.a /usr/local/lib64/libabsl_demangle_internal.a /usr/local/lib64/libabsl_graphcycles_internal.a /usr/local/lib64/libabsl_kernel_timeout_internal.a /usr/local/lib64/libabsl_malloc_internal.a /usr/local/lib64/libabsl_time.a /usr/local/lib64/libabsl_civil_time.a /usr/local/lib64/libabsl_time_zone.a /usr/local/lib64/libabsl_bad_variant_access.a /usr/local/lib64/libutf8_validity.a /usr/local/lib64/libabsl_strings.a /usr/local/lib64/libabsl_int128.a /usr/local/lib64/libabsl_string_view.a /usr/local/lib64/libabsl_throw_delegate.a /usr/local/lib64/libabsl_strings_internal.a /usr/local/lib64/libabsl_base.a /usr/local/lib64/libabsl_spinlock_wait.a -lrt /usr/local/lib64/libabsl_raw_logging_internal.a /usr/local/lib64/libabsl_log_severity.a 

CFLAGS := -I./ -DCARES_STATICLIB  -std=gnu++17 -MD -MT 
OBJS = $(SRCS:.cc=.o)

all: ${target}

#vmlinux:
#	bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

%.o: %.cc
	g++ -Wall -c -g $< -o $@

#${target}: ${target}.c bpf/${target}.skel.h
#	gcc -Wall -o $@ $< -lbpf
${target}: $(OBJS) 
	 #$(CC) $(CFLAGS) $(OBJS) -o $@ -lbpf
	 #$(CC) $(CFLAGS) $(OBJS) -o $@ /usr/lib64/libbpf.a /lib64/libc.a  -lelf -lz
	 g++  $(OBJS) $(LIBS) -o $@  

.PHONY:
clean:
	rm -rf bpf/${target}.bpf.o bpf/${target}.skel.h ${target} *.o

