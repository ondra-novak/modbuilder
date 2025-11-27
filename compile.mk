.PHONY: all t_0 t_1 t_2 t_3 t_4 t_5 t_6 t_7 t_8 t_9 t_10 t_11 t_12 t_13 t_14 t_15 t_16 t_17 t_18 t_19 t_20 t_21 t_22 t_23 t_24 t_25 t_26 t_27 t_28 t_29 t_30 t_31 t_32 t_33 t_34 t_35 t_36 t_37 t_38 t_39 t_40 t_41 t_42 t_43 t_44 t_45 t_46 t_47 t_48 t_49 t_50 t_51 t_52 t_53 t_54 t_55 t_56 t_57 t_58 t_59 t_60 t_61 t_62 t_63 t_64 t_65 t_66 t_67 t_68 t_69 t_70 t_71 t_72 t_73 t_74 t_75 t_76 t_77 t_78 t_79 t_80 t_81
all: t_36

t_0: t_37 t_38 t_39 t_40| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/cstring_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/cerrno_0.~hdr.pcm -fmodule-file=.install/pcm/streambuf_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/fd_streambuf.cpp -o .install/obj/fd_streambuf_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/cstring_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/cerrno_0.~hdr.pcm -fmodule-file=.install/pcm/streambuf_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/fd_streambuf.cpp -o .install/pcm/cairn.utils.fd_streambuf.pcm

t_1: t_41| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/fkyaml_9a8dee51f130cf79.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/vendors.cpp -o .install/obj/vendors_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/fkyaml_9a8dee51f130cf79.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/vendors.cpp -o .install/pcm/cairn.utils.vendors.pcm

t_2: t_42 t_43 t_44 t_4 t_0 t_39 t_45 t_46 t_19 t_18 t_37 t_47 t_48 t_49 t_50 t_51 t_38| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/optional_0.~hdr.pcm -fmodule-file=.install/pcm/span_0.~hdr.pcm -fmodule-file=.install/pcm/memory_0.~hdr.pcm -fmodule-file=.install/pcm/cerrno_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -fmodule-file=.install/pcm/cstring_0.~hdr.pcm -fmodule-file=.install/pcm/cstddef_0.~hdr.pcm -fmodule-file=.install/pcm/system_error_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/numeric_0.~hdr.pcm -fmodule-file=.install/pcm/stdexcept_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/process_posix.cpp -o .install/obj/process_posix_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/optional_0.~hdr.pcm -fmodule-file=.install/pcm/span_0.~hdr.pcm -fmodule-file=.install/pcm/memory_0.~hdr.pcm -fmodule-file=.install/pcm/cerrno_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -fmodule-file=.install/pcm/cstring_0.~hdr.pcm -fmodule-file=.install/pcm/cstddef_0.~hdr.pcm -fmodule-file=.install/pcm/system_error_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/numeric_0.~hdr.pcm -fmodule-file=.install/pcm/stdexcept_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/process_posix.cpp -o .install/pcm/cairn.utils.process-posix.pcm

t_3: t_52 t_53 t_54 t_45 t_55 t_56 t_42 t_38 t_16| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/charconv_0.~hdr.pcm -fmodule-file=.install/pcm/exception_0.~hdr.pcm -fmodule-file=.install/pcm/format_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/unordered_map_0.~hdr.pcm -fmodule-file=.install/pcm/variant_0.~hdr.pcm -fmodule-file=.install/pcm/optional_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/simple_json.cpp -o .install/obj/simple_json_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/charconv_0.~hdr.pcm -fmodule-file=.install/pcm/exception_0.~hdr.pcm -fmodule-file=.install/pcm/format_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/unordered_map_0.~hdr.pcm -fmodule-file=.install/pcm/variant_0.~hdr.pcm -fmodule-file=.install/pcm/optional_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/simple_json.cpp -o .install/pcm/cairn.utils.simple_json.pcm

t_4: t_57 t_43 t_46 t_49 t_56 t_58 t_45| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/algorithm_0.~hdr.pcm -fmodule-file=.install/pcm/span_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/variant_0.~hdr.pcm -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/arguments.cpp -o .install/obj/arguments_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/algorithm_0.~hdr.pcm -fmodule-file=.install/pcm/span_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/variant_0.~hdr.pcm -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/arguments.cpp -o .install/pcm/cairn.utils.arguments.pcm

t_5: t_45 t_38 t_59| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/sstream_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/version.cpp -o .install/obj/version_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/sstream_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/version.cpp -o .install/pcm/cairn.utils.version.pcm

t_6: t_58| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/module_type.cpp -o .install/obj/module_type_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/module_type.cpp -o .install/pcm/cairn.module_type.pcm

t_7: t_16 t_18 t_48 t_58 t_1 t_45 t_51 t_30 t_49 t_60 t_61 t_31| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/system_error_0.~hdr.pcm -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/stdexcept_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/cctype_0.~hdr.pcm -fmodule-file=.install/pcm/fstream_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/module_resolver.cpp -o .install/obj/module_resolver_f4ce731b09f65819.o

t_8: t_42 t_4 t_30 t_59 t_49 t_16 t_38 t_10 t_3 t_61 t_55 t_62 t_45 t_56 t_58| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/optional_0.~hdr.pcm -fmodule-file=.install/pcm/sstream_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/fstream_0.~hdr.pcm -fmodule-file=.install/pcm/unordered_map_0.~hdr.pcm -fmodule-file=.install/pcm/ostream_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/variant_0.~hdr.pcm -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/compile_commands_supp.cpp -o .install/obj/compile_commands_supp_f4ce731b09f65819.o

t_9: t_49| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/compile_target.cpp -o .install/obj/compile_target_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/compile_target.cpp -o .install/pcm/cairn.compile_target.pcm

t_10: t_3 t_4 t_55 t_38 t_30 t_56 t_49| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/unordered_map_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/variant_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/compile_commands_supp.ifc.cpp -o .install/obj/compile_commands_supp.ifc_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/unordered_map_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/variant_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/compile_commands_supp.ifc.cpp -o .install/pcm/cairn.compile_commands.pcm

t_11: t_56 t_63 t_38 t_64 t_49 t_6 t_17 t_4 t_28 t_9 t_65 t_23 t_66 t_30 t_12 t_10 t_67 t_55 t_68| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/variant_0.~hdr.pcm -fmodule-file=.install/pcm/map_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/atomic_0.~hdr.pcm -fmodule-file=.install/pcm/iterator_0.~hdr.pcm -fmodule-file=.install/pcm/chrono_0.~hdr.pcm -fmodule-file=.install/pcm/unordered_map_0.~hdr.pcm -fmodule-file=.install/pcm/functional_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/module_database.ifc.cpp -o .install/obj/module_database.ifc_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/variant_0.~hdr.pcm -fmodule-file=.install/pcm/map_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/atomic_0.~hdr.pcm -fmodule-file=.install/pcm/iterator_0.~hdr.pcm -fmodule-file=.install/pcm/chrono_0.~hdr.pcm -fmodule-file=.install/pcm/unordered_map_0.~hdr.pcm -fmodule-file=.install/pcm/functional_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/module_database.ifc.cpp -o .install/pcm/cairn.module_database.pcm

t_12: t_43 t_38 t_24 t_10 t_4 t_23 t_6 t_25 t_58 t_54 t_17 t_18 t_19 t_49| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/span_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -fmodule-file=.install/pcm/format_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/abstract_compiler.cpp -o .install/obj/abstract_compiler_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/span_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -fmodule-file=.install/pcm/format_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/abstract_compiler.cpp -o .install/pcm/cairn.abstract_compiler.pcm

t_13: t_38 t_49 t_9 t_29 t_18 t_4| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/cli.cpp -o .install/obj/cli_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/cli.cpp -o .install/pcm/cairn.cli.pcm

t_14: t_38 t_68 t_69 t_46 t_18| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/functional_0.~hdr.pcm -fmodule-file=.install/pcm/mutex_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/log-impl.cpp -o .install/obj/log-impl_9a8dee51f130cf79.o

t_15: t_44 t_12| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/memory_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/compilers/clang/factory.cpp -o .install/obj/factory_2f01e9763865527e.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/memory_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/compilers/clang/factory.cpp -o .install/pcm/cairn.compiler.clang.pcm

t_16: t_57 t_45 t_46| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/algorithm_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/utf_8.cpp -o .install/obj/utf_8_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/algorithm_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/utf_8.cpp -o .install/pcm/cairn.utils.utf8.pcm

t_17: t_49 t_45 t_38 t_57 t_6| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/algorithm_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/scanner.cpp -o .install/obj/scanner_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/algorithm_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/scanner.cpp -o .install/pcm/cairn.source_scanner.pcm

t_18: t_64 t_68 t_70 t_38 t_54| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/functional_0.~hdr.pcm -fmodule-file=.install/pcm/array_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/format_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/log.cpp -o .install/obj/log_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/functional_0.~hdr.pcm -fmodule-file=.install/pcm/array_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/format_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/log.cpp -o .install/pcm/cairn.utils.log.pcm

t_19: t_38 t_63 t_45 t_71 t_48| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/map_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/cwctype_0.~hdr.pcm -fmodule-file=.install/pcm/system_error_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/env.cpp -o .install/obj/env_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/map_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/cwctype_0.~hdr.pcm -fmodule-file=.install/pcm/system_error_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/env.cpp -o .install/pcm/cairn.utils.env.pcm

t_20: t_70 t_72 t_51 t_61 t_49 t_57 t_43 t_44 t_12 t_10 t_42 t_24 t_16 t_4 t_66 t_15 t_23 t_6 t_25 t_5 t_18 t_17 t_19| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/array_0.~hdr.pcm -fmodule-file=.install/pcm/regex_0.~hdr.pcm -fmodule-file=.install/pcm/stdexcept_0.~hdr.pcm -fmodule-file=.install/pcm/fstream_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/algorithm_0.~hdr.pcm -fmodule-file=.install/pcm/span_0.~hdr.pcm -fmodule-file=.install/pcm/memory_0.~hdr.pcm -fmodule-file=.install/pcm/optional_0.~hdr.pcm -fmodule-file=.install/pcm/iterator_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/compilers/clang/compiler_clang.cpp -o .install/obj/compiler_clang_2f01e9763865527e.o

t_21: t_44 t_12| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/memory_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/compilers/gcc/factory.cpp -o .install/obj/factory_4b9f900a6746ec2b.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/memory_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/compilers/gcc/factory.cpp -o .install/pcm/cairn.compiler.gcc.pcm

t_22: t_65 t_70 t_51 t_61 t_49 t_46 t_19 t_72 t_21 t_16 t_73 t_10 t_24 t_44 t_12 t_53 t_6 t_25 t_4 t_23 t_5 t_27 t_18 t_17| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/atomic_0.~hdr.pcm -fmodule-file=.install/pcm/array_0.~hdr.pcm -fmodule-file=.install/pcm/stdexcept_0.~hdr.pcm -fmodule-file=.install/pcm/fstream_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -fmodule-file=.install/pcm/regex_0.~hdr.pcm -fmodule-file=.install/pcm/unordered_set_0.~hdr.pcm -fmodule-file=.install/pcm/memory_0.~hdr.pcm -fmodule-file=.install/pcm/exception_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/compilers/gcc/compiler_gcc.cpp -o .install/obj/compiler_gcc_4b9f900a6746ec2b.o

t_23: t_38 t_49| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/origin_env.cpp -o .install/obj/origin_env_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/origin_env.cpp -o .install/pcm/cairn.origin_env.pcm

t_24: t_49 t_6| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/source_def.cpp -o .install/obj/source_def_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/source_def.cpp -o .install/pcm/cairn.source_def.pcm

t_25: t_2| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fprebuilt-module-path=.install/pcm -c src/cairn/utils/process.cpp -o .install/obj/process_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/process.cpp -o .install/pcm/cairn.utils.process.pcm

t_26: t_44 t_69 t_65 t_27 t_28 t_18 t_12| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/memory_0.~hdr.pcm -fmodule-file=.install/pcm/mutex_0.~hdr.pcm -fmodule-file=.install/pcm/atomic_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/builder.cpp -o .install/obj/builder_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/memory_0.~hdr.pcm -fmodule-file=.install/pcm/mutex_0.~hdr.pcm -fmodule-file=.install/pcm/atomic_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/builder.cpp -o .install/pcm/cairn.builder.pcm

t_27: t_64 t_74 t_75 t_69 t_76 t_44| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/condition_variable_0.~hdr.pcm -fmodule-file=.install/pcm/queue_0.~hdr.pcm -fmodule-file=.install/pcm/mutex_0.~hdr.pcm -fmodule-file=.install/pcm/thread_0.~hdr.pcm -fmodule-file=.install/pcm/memory_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/thread_pool.cpp -o .install/obj/thread_pool_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/condition_variable_0.~hdr.pcm -fmodule-file=.install/pcm/queue_0.~hdr.pcm -fmodule-file=.install/pcm/mutex_0.~hdr.pcm -fmodule-file=.install/pcm/thread_0.~hdr.pcm -fmodule-file=.install/pcm/memory_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/thread_pool.cpp -o .install/pcm/cairn.utils.threadpool.pcm

t_28: t_38 t_45 t_43| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/span_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/build_plan.cpp -o .install/obj/build_plan_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/span_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/build_plan.cpp -o .install/pcm/cairn.build_plan.pcm

t_29: t_58| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/version.cpp -o .install/obj/version_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/string_view_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/version.cpp -o .install/pcm/cairn.version.pcm

t_30: t_47| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/cstddef_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/hash.cpp -o .install/obj/hash_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/cstddef_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/hash.cpp -o .install/pcm/cairn.utils.hash.pcm

t_31: t_49 t_38 t_23| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/module_resolver.ifc.cpp -o .install/obj/module_resolver.ifc_f4ce731b09f65819.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/module_resolver.ifc.cpp -o .install/pcm/cairn.module_resolver.pcm

t_32: t_77 t_64 t_51 t_78 t_44 t_46 t_63 t_79 t_80| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/utility_0.~hdr.pcm -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/stdexcept_0.~hdr.pcm -fmodule-file=.install/pcm/set_0.~hdr.pcm -fmodule-file=.install/pcm/memory_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -fmodule-file=.install/pcm/map_0.~hdr.pcm -fmodule-file=.install/pcm/cstdint_0.~hdr.pcm -fmodule-file=.install/pcm/concepts_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/serializer.cpp -o .install/obj/serializer_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/utility_0.~hdr.pcm -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/stdexcept_0.~hdr.pcm -fmodule-file=.install/pcm/set_0.~hdr.pcm -fmodule-file=.install/pcm/memory_0.~hdr.pcm -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -fmodule-file=.install/pcm/map_0.~hdr.pcm -fmodule-file=.install/pcm/cstdint_0.~hdr.pcm -fmodule-file=.install/pcm/concepts_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/serializer.cpp -o .install/pcm/cairn.utils.serializer.pcm

t_33: t_81 t_75 t_11 t_24 t_73 t_10 t_6 t_31 t_4 t_62 t_17 t_18 t_30 t_12 t_34 t_32 t_65| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/ranges_0.~hdr.pcm -fmodule-file=.install/pcm/queue_0.~hdr.pcm -fmodule-file=.install/pcm/unordered_set_0.~hdr.pcm -fmodule-file=.install/pcm/ostream_0.~hdr.pcm -fmodule-file=.install/pcm/atomic_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/module_database.cpp -o .install/obj/module_database_f4ce731b09f65819.o

t_34: t_64 t_45 t_49| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/utils/serialization_rules.cpp -o .install/obj/serialization_rules_9a8dee51f130cf79.o
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/type_traits_0.~hdr.pcm -fmodule-file=.install/pcm/string_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -xc++-module --precompile src/cairn/utils/serialization_rules.cpp -o .install/pcm/cairn.utils.serializer.rules.pcm

t_35: t_46 t_57 t_63 t_48 t_61 t_70 t_38 t_44 t_12 t_49 t_13 t_26 t_4 t_15 t_23 t_21 t_53 t_6 t_11 t_73 t_10 t_18 t_17 t_27| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-file=.install/pcm/iostream_0.~hdr.pcm -fmodule-file=.install/pcm/algorithm_0.~hdr.pcm -fmodule-file=.install/pcm/map_0.~hdr.pcm -fmodule-file=.install/pcm/system_error_0.~hdr.pcm -fmodule-file=.install/pcm/fstream_0.~hdr.pcm -fmodule-file=.install/pcm/array_0.~hdr.pcm -fmodule-file=.install/pcm/vector_0.~hdr.pcm -fmodule-file=.install/pcm/memory_0.~hdr.pcm -fmodule-file=.install/pcm/filesystem_0.~hdr.pcm -fmodule-file=.install/pcm/exception_0.~hdr.pcm -fmodule-file=.install/pcm/unordered_set_0.~hdr.pcm -Wno-experimental-header-units -fprebuilt-module-path=.install/pcm -c src/cairn/main.cpp -o .install/obj/main_f4ce731b09f65819.o

t_36: t_0 t_1 t_2 t_3 t_4 t_5 t_6 t_7 t_8 t_9 t_10 t_11 t_12 t_13 t_14 t_15 t_16 t_17 t_18 t_19 t_20 t_21 t_22 t_23 t_24 t_25 t_26 t_27 t_28 t_29 t_30 t_31 t_32 t_33 t_34 t_35| workdir 
	/usr/bin/clang++ -std=c++20 -O3 .install/obj/module_database_f4ce731b09f65819.o .install/obj/serializer_9a8dee51f130cf79.o .install/obj/main_f4ce731b09f65819.o .install/obj/module_resolver.ifc_f4ce731b09f65819.o .install/obj/version_f4ce731b09f65819.o .install/obj/build_plan_f4ce731b09f65819.o .install/obj/process_9a8dee51f130cf79.o .install/obj/hash_9a8dee51f130cf79.o .install/obj/source_def_f4ce731b09f65819.o .install/obj/origin_env_f4ce731b09f65819.o .install/obj/compiler_gcc_4b9f900a6746ec2b.o .install/obj/factory_4b9f900a6746ec2b.o .install/obj/serialization_rules_9a8dee51f130cf79.o .install/obj/env_9a8dee51f130cf79.o .install/obj/log_9a8dee51f130cf79.o .install/obj/builder_f4ce731b09f65819.o .install/obj/factory_2f01e9763865527e.o .install/obj/scanner_f4ce731b09f65819.o .install/obj/log-impl_9a8dee51f130cf79.o .install/obj/module_database.ifc_f4ce731b09f65819.o .install/obj/compile_commands_supp.ifc_f4ce731b09f65819.o .install/obj/compile_target_f4ce731b09f65819.o .install/obj/compile_commands_supp_f4ce731b09f65819.o .install/obj/module_resolver_f4ce731b09f65819.o .install/obj/module_type_f4ce731b09f65819.o .install/obj/utf_8_9a8dee51f130cf79.o .install/obj/abstract_compiler_f4ce731b09f65819.o .install/obj/version_9a8dee51f130cf79.o .install/obj/arguments_9a8dee51f130cf79.o .install/obj/compiler_clang_2f01e9763865527e.o .install/obj/cli_f4ce731b09f65819.o .install/obj/simple_json_9a8dee51f130cf79.o .install/obj/process_posix_9a8dee51f130cf79.o .install/obj/vendors_9a8dee51f130cf79.o .install/obj/thread_pool_9a8dee51f130cf79.o .install/obj/fd_streambuf_9a8dee51f130cf79.o -o .install/cairn

t_37:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile cstring -o .install/pcm/cstring_0.~hdr.pcm

t_38:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile vector -o .install/pcm/vector_0.~hdr.pcm

t_39:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile cerrno -o .install/pcm/cerrno_0.~hdr.pcm

t_40:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile streambuf -o .install/pcm/streambuf_0.~hdr.pcm

t_41:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -fmodule-header=user -xc++-header --precompile src/cairn/utils/fkyaml.hpp -o .install/pcm/fkyaml_9a8dee51f130cf79.~hdr.pcm

t_42:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile optional -o .install/pcm/optional_0.~hdr.pcm

t_43:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile span -o .install/pcm/span_0.~hdr.pcm

t_44:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile memory -o .install/pcm/memory_0.~hdr.pcm

t_45:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile string -o .install/pcm/string_0.~hdr.pcm

t_46:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile iostream -o .install/pcm/iostream_0.~hdr.pcm

t_47:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile cstddef -o .install/pcm/cstddef_0.~hdr.pcm

t_48:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile system_error -o .install/pcm/system_error_0.~hdr.pcm

t_49:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile filesystem -o .install/pcm/filesystem_0.~hdr.pcm

t_50:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile numeric -o .install/pcm/numeric_0.~hdr.pcm

t_51:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile stdexcept -o .install/pcm/stdexcept_0.~hdr.pcm

t_52:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile charconv -o .install/pcm/charconv_0.~hdr.pcm

t_53:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile exception -o .install/pcm/exception_0.~hdr.pcm

t_54:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile format -o .install/pcm/format_0.~hdr.pcm

t_55:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile unordered_map -o .install/pcm/unordered_map_0.~hdr.pcm

t_56:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile variant -o .install/pcm/variant_0.~hdr.pcm

t_57:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile algorithm -o .install/pcm/algorithm_0.~hdr.pcm

t_58:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile string_view -o .install/pcm/string_view_0.~hdr.pcm

t_59:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile sstream -o .install/pcm/sstream_0.~hdr.pcm

t_60:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile cctype -o .install/pcm/cctype_0.~hdr.pcm

t_61:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile fstream -o .install/pcm/fstream_0.~hdr.pcm

t_62:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile ostream -o .install/pcm/ostream_0.~hdr.pcm

t_63:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile map -o .install/pcm/map_0.~hdr.pcm

t_64:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile type_traits -o .install/pcm/type_traits_0.~hdr.pcm

t_65:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile atomic -o .install/pcm/atomic_0.~hdr.pcm

t_66:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile iterator -o .install/pcm/iterator_0.~hdr.pcm

t_67:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile chrono -o .install/pcm/chrono_0.~hdr.pcm

t_68:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile functional -o .install/pcm/functional_0.~hdr.pcm

t_69:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile mutex -o .install/pcm/mutex_0.~hdr.pcm

t_70:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile array -o .install/pcm/array_0.~hdr.pcm

t_71:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile cwctype -o .install/pcm/cwctype_0.~hdr.pcm

t_72:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile regex -o .install/pcm/regex_0.~hdr.pcm

t_73:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile unordered_set -o .install/pcm/unordered_set_0.~hdr.pcm

t_74:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile condition_variable -o .install/pcm/condition_variable_0.~hdr.pcm

t_75:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile queue -o .install/pcm/queue_0.~hdr.pcm

t_76:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile thread -o .install/pcm/thread_0.~hdr.pcm

t_77:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile utility -o .install/pcm/utility_0.~hdr.pcm

t_78:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile set -o .install/pcm/set_0.~hdr.pcm

t_79:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile cstdint -o .install/pcm/cstdint_0.~hdr.pcm

t_80:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile concepts -o .install/pcm/concepts_0.~hdr.pcm

t_81:| workdir 
	/usr/bin/clang++ -std=c++20 -O3 -Xclang -fretain-comments-from-system-headers -Wno-pragma-system-header-outside-header -fmodule-header=system -xc++-system-header --precompile ranges -o .install/pcm/ranges_0.~hdr.pcm


workdir:
	mkdir -p .install
	mkdir -p .install/pcm
	mkdir -p .install/obj
