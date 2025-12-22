!host_build|!cross_compile {
    QMAKE_CFLAGS=-g -O2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -fstack-protector-strong -fstack-clash-protection -Wformat -Werror=format-security -fcf-protection -fdebug-prefix-map=/build/qtbase-opensource-src-y4ZVUy/qtbase-opensource-src-5.15.13+dfsg=/usr/src/qtbase-opensource-src-5.15.13+dfsg-1ubuntu1 -Wdate-time -D_FORTIFY_SOURCE=3 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 
    QMAKE_CXXFLAGS=-g -O2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -fstack-protector-strong -fstack-clash-protection -Wformat -Werror=format-security -fcf-protection -fdebug-prefix-map=/build/qtbase-opensource-src-y4ZVUy/qtbase-opensource-src-5.15.13+dfsg=/usr/src/qtbase-opensource-src-5.15.13+dfsg-1ubuntu1 -Wdate-time -D_FORTIFY_SOURCE=3 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 
    QMAKE_LFLAGS=-Wl,-Bsymbolic-functions -Wl,-z,relro
}
QT_CPU_FEATURES.x86_64 = mmx sse sse2
QT.global_private.enabled_features = sse2 alloca_h alloca avx2 dbus dbus-linked dlopen gui intelcet libudev network posix_fallocate reduce_exports reduce_relocations release_tools sql system-zlib testlib widgets xml zstd
QT.global_private.disabled_features = alloca_malloc_h android-style-assets private_tests gc_binaries relocatable stack-protector-strong
PKG_CONFIG_EXECUTABLE = /usr/bin/x86_64-linux-gnu-pkg-config
QMAKE_LIBS_DBUS = -ldbus-1
QMAKE_INCDIR_DBUS = /usr/include/dbus-1.0 /usr/lib/x86_64-linux-gnu/dbus-1.0/include
QMAKE_LIBS_LIBDL = 
QMAKE_LIBS_LIBUDEV = -ludev
QT_COORD_TYPE = double
QMAKE_LIBS_ZLIB = -lz
QMAKE_LIBS_ZSTD = -lzstd
CONFIG += sse2 aesni sse3 ssse3 sse4_1 sse4_2 avx avx2 avx512f avx512bw avx512cd avx512dq avx512er avx512ifma avx512pf avx512vbmi avx512vl compile_examples enable_new_dtags f16c largefile precompile_header rdrnd rdseed shani nostrip x86SimdAlways
QT_BUILD_PARTS += libs examples tools
QT_HOST_CFLAGS_DBUS += -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include
