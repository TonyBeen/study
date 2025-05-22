ARCH=arm
function build() {
        echo -e "\nbuild udp_send for $ARCH ..."

        if [ "$ARCH" = "arm" ]; then
			export CC=$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi19-clang
			export CXX=$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi19-clang++
        else
			export CC=$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang
			export CXX=$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang++
        fi
        echo "CC: $CC"
        echo "CXX: $CXX"

		$CXX -o udp_send udp_send.cc -std=c++11

        echo "build udp_send for $ARCH done."
}
build arm
