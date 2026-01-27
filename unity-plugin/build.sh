#!/bin/bash
set -e

cd "$(dirname "$0")"

OUTPUT="./Plugins"
rm -rf "$OUTPUT"
mkdir -p "$OUTPUT/iOS" "$OUTPUT/Android/arm64-v8a" "$OUTPUT/Android/armeabi-v7a" "$OUTPUT/macOS"

CFLAGS="-O2 -Wall -fvisibility=hidden"

echo "===== Building macOS (Editor) ====="
clang -dynamiclib -arch arm64 -arch x86_64 $CFLAGS \
    bsdiff_unity.c ../bsdiff.c ../bspatch.c \
    -o "$OUTPUT/macOS/libbsdiff.dylib"
echo "macOS done"

echo ""
echo "===== Building iOS ====="
TEMP_IOS=$(mktemp -d)

# iOS device (arm64)
xcrun -sdk iphoneos clang -arch arm64 $CFLAGS -c bsdiff_unity.c -o "$TEMP_IOS/unity_arm64.o"
xcrun -sdk iphoneos clang -arch arm64 $CFLAGS -c ../bsdiff.c -o "$TEMP_IOS/diff_arm64.o"
xcrun -sdk iphoneos clang -arch arm64 $CFLAGS -c ../bspatch.c -o "$TEMP_IOS/patch_arm64.o"
ar rcs "$TEMP_IOS/libbsdiff_device.a" "$TEMP_IOS/unity_arm64.o" "$TEMP_IOS/diff_arm64.o" "$TEMP_IOS/patch_arm64.o"

# iOS Simulator (arm64 + x86_64)
xcrun -sdk iphonesimulator clang -arch arm64 $CFLAGS -c bsdiff_unity.c -o "$TEMP_IOS/unity_sim_arm64.o"
xcrun -sdk iphonesimulator clang -arch arm64 $CFLAGS -c ../bsdiff.c -o "$TEMP_IOS/diff_sim_arm64.o"
xcrun -sdk iphonesimulator clang -arch arm64 $CFLAGS -c ../bspatch.c -o "$TEMP_IOS/patch_sim_arm64.o"
ar rcs "$TEMP_IOS/libbsdiff_sim_arm64.a" "$TEMP_IOS/unity_sim_arm64.o" "$TEMP_IOS/diff_sim_arm64.o" "$TEMP_IOS/patch_sim_arm64.o"

xcrun -sdk iphonesimulator clang -arch x86_64 $CFLAGS -c bsdiff_unity.c -o "$TEMP_IOS/unity_sim_x64.o"
xcrun -sdk iphonesimulator clang -arch x86_64 $CFLAGS -c ../bsdiff.c -o "$TEMP_IOS/diff_sim_x64.o"
xcrun -sdk iphonesimulator clang -arch x86_64 $CFLAGS -c ../bspatch.c -o "$TEMP_IOS/patch_sim_x64.o"
ar rcs "$TEMP_IOS/libbsdiff_sim_x64.a" "$TEMP_IOS/unity_sim_x64.o" "$TEMP_IOS/diff_sim_x64.o" "$TEMP_IOS/patch_sim_x64.o"

lipo -create "$TEMP_IOS/libbsdiff_sim_arm64.a" "$TEMP_IOS/libbsdiff_sim_x64.a" -output "$TEMP_IOS/libbsdiff_simulator.a"

xcodebuild -create-xcframework \
    -library "$TEMP_IOS/libbsdiff_device.a" \
    -library "$TEMP_IOS/libbsdiff_simulator.a" \
    -output "$OUTPUT/iOS/libbsdiff.xcframework"

rm -rf "$TEMP_IOS"
echo "iOS done"

echo ""
echo "===== Building Android ====="

# Find NDK
if [ -n "$ANDROID_NDK_HOME" ]; then
    NDK_PATH="$ANDROID_NDK_HOME"
elif [ -d "/opt/homebrew/Caskroom/android-ndk" ]; then
    NDK_PATH=$(find /opt/homebrew/Caskroom/android-ndk -name "ndk-build" -type f 2>/dev/null | head -1 | xargs dirname)
elif [ -d "$HOME/Library/Android/sdk/ndk" ]; then
    NDK_PATH=$(ls -d "$HOME/Library/Android/sdk/ndk"/* 2>/dev/null | tail -1)
fi

cd Android
"$NDK_PATH/ndk-build" NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./jni/Android.mk NDK_APPLICATION_MK=./jni/Application.mk
cp libs/arm64-v8a/libbsdiff.so "../$OUTPUT/Android/arm64-v8a/"
cp libs/armeabi-v7a/libbsdiff.so "../$OUTPUT/Android/armeabi-v7a/"
rm -rf obj libs
cd ..
echo "Android done"

echo ""
echo "===== Copying C# wrapper ====="
cp BsdiffNative.cs "$OUTPUT/"

echo ""
echo "===== BUILD COMPLETE ====="
echo ""
echo "Output: $(pwd)/Plugins/"
find "$OUTPUT" -type f | sort
echo ""
echo "Unity에 적용:"
echo "  Plugins 폴더를 통째로 Assets/ 밑에 복사하면 끝"