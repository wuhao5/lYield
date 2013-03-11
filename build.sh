#!/bin/sh
#

: ${IPHONE_SDKVERSION:=`xcodebuild -showsdks | grep iphoneos | egrep "[[:digit:]]+\.[[:digit:]]+" -o | tail -1`}
: ${XCODE_ROOT:=`xcode-select -print-path`}

: ${SRCDIR:=`pwd`}
: ${IOSBUILDDIR:=`pwd`/ios/build}
: ${OSXBUILDDIR:=`pwd`/osx/build}
: ${PREFIXDIR:=`pwd`/ios/prefix}
: ${IOSFRAMEWORKDIR:=`pwd`/ios/framework}
: ${OSXFRAMEWORKDIR:=`pwd`/osx/framework}
: ${COMPILER:="gcc"}

#===============================================================================

ARM_DEV_DIR=$XCODE_ROOT/Platforms/iPhoneOS.platform/Developer/usr/bin/
SIM_DEV_DIR=$XCODE_ROOT/Platforms/iPhoneSimulator.platform/Developer/usr/bin/

IOSSYSROOT=$XCODE_ROOT/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS$IPHONE_SDKVERSION.sdk
IOSSIMSYSROOT=$XCODE_ROOT/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator$IPHONE_SDKVERSION.sdk

LYIELD_HEADERS="$SRCDIR/lyield/lyield.h"

compile_framework() {
	FRAMEWORK_BUNDLE=$1/lyield.framework
	FRAMEWORK_VERSION=A
	FRAMEWORK_NAME=lyield
	FRAMEWORK_CURRENT_VERSION=$LYIELD_VERSION

	shift;

	rm -rf $FRAMEWORK_BUNDLE

	mkdir -p $FRAMEWORK_BUNDLE
	mkdir -p $FRAMEWORK_BUNDLE/Versions
	mkdir -p $FRAMEWORK_BUNDLE/Versions/$FRAMEWORK_VERSION
	mkdir -p $FRAMEWORK_BUNDLE/Versions/$FRAMEWORK_VERSION/Resources
	mkdir -p $FRAMEWORK_BUNDLE/Versions/$FRAMEWORK_VERSION/Headers
	mkdir -p $FRAMEWORK_BUNDLE/Versions/$FRAMEWORK_VERSION/Documentation

	FRAMEWORK_INSTALL_NAME=$FRAMEWORK_BUNDLE/Versions/$FRAMEWORK_VERSION/$FRAMEWORK_NAME

	echo "Lipoing library into $FRAMEWORK_INSTALL_NAME..."
	$ARM_DEV_DIR/lipo -create $@ -output "$FRAMEWORK_INSTALL_NAME" || exit

	ln -s $FRAMEWORK_VERSION               $FRAMEWORK_BUNDLE/Versions/Current
	ln -s Versions/Current/Headers         $FRAMEWORK_BUNDLE/Headers
	ln -s Versions/Current/Resources       $FRAMEWORK_BUNDLE/Resources
	ln -s Versions/Current/Documentation   $FRAMEWORK_BUNDLE/Documentation
	ln -s Versions/Current/$FRAMEWORK_NAME $FRAMEWORK_BUNDLE/$FRAMEWORK_NAME

	echo "Framework: Copying includes..."
	cp -r $LYIELD_HEADERS  $FRAMEWORK_BUNDLE/Headers/

	echo "Framework: Creating plist..."
	cat > $FRAMEWORK_BUNDLE/Resources/Info.plist <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
	<dict>
		<key>CFBundleDevelopmentRegion</key>
		<string>English</string>
		<key>CFBundleExecutable</key>
		<string>${FRAMEWORK_NAME}</string>
		<key>CFBundleIdentifier</key>
		<string>org.chaos3d.lyield</string>
		<key>CFBundleInfoDictionaryVersion</key>
		<string>6.0</string>
		<key>CFBundlePackageType</key>
		<string>FMWK</string>
		<key>CFBundleSignature</key>
		<string>????</string>
		<key>CFBundleVersion</key>
		<string>${FRAMEWORK_CURRENT_VERSION}</string>
	</dict>
</plist>
EOF
}


mkdir -p $IOSBUILDDIR
mkdir -p $OSXBUILDDIR

IOSSDK=`xcodebuild -showsdks | grep "iphoneos.*$" -o | tail -1`
xcodebuild clean -sdk "$IOSSDK" TARGET_BUILD_DIR=`pwd`/ios/ -configuration=Release
xcodebuild -sdk "$IOSSDK" TARGET_BUILD_DIR=`pwd`/ios/ -configuration=Release

IOSSIMSDK=`xcodebuild -showsdks | grep "iphonesim.*$" -o | tail -1`
xcodebuild clean -sdk "$IOSSIMSDK" TARGET_BUILD_DIR=`pwd`/ios-sim/ -configuration=Release
xcodebuild -sdk "$IOSSIMSDK" TARGET_BUILD_DIR=`pwd`/ios-sim/ -configuration=Release

MACOSSDK=`xcodebuild -showsdks | grep "macos.*$" -o | tail -1`
xcodebuild clean -sdk "$MACOSSDK" TARGET_BUILD_DIR=`pwd`/macos/ -configuration=Release
xcodebuild -sdk "$MACOSSDK" TARGET_BUILD_DIR=`pwd`/macos/ -configuration=Release

echo build ios framework ...
compile_framework $IOSFRAMEWORKDIR `pwd`/ios/liblyield.a `pwd`/ios-sim/liblyield.a

echo build osx framework ...
compile_framework $OSXFRAMEWORKDIR `pwd`/macos/liblyield.a

echo framework will be at $IOSFRAMEWORKDIR and $OSXFRAMEWORKDIR
echo success!
