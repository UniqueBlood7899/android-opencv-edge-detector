# Android SDK Setup Guide

## Quick Setup Options

### Option 1: Install Android Studio (Recommended)

1. **Download Android Studio**
   - Visit: https://developer.android.com/studio
   - Download for macOS
   - Install the application

2. **During First Launch**
   - Android Studio will automatically download and install the Android SDK
   - The SDK will be installed at: `~/Library/Android/sdk`

3. **Update local.properties**
   ```bash
   # Edit local.properties and set:
   sdk.dir=/Users/suryaps/Library/Android/sdk
   ```

### Option 2: Install Command-Line Tools Only

1. **Download Command-Line Tools**
   ```bash
   # Create SDK directory
   mkdir -p ~/Library/Android/sdk
   cd ~/Library/Android/sdk
   
   # Download command-line tools
   curl -O https://dl.google.com/android/repository/commandlinetools-mac-11076708_latest.zip
   unzip commandlinetools-mac-11076708_latest.zip
   mkdir -p cmdline-tools/latest
   mv cmdline-tools/* cmdline-tools/latest/ 2>/dev/null || true
   ```

2. **Install SDK Components**
   ```bash
   export ANDROID_HOME=~/Library/Android/sdk
   export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools
   
   # Accept licenses
   yes | sdkmanager --licenses
   
   # Install required components
   sdkmanager "platform-tools" "platforms;android-34" "build-tools;34.0.0" "cmake;3.22.1" "ndk;26.1.10909125"
   ```

3. **Update local.properties**
   ```bash
   # Edit local.properties and set:
   sdk.dir=/Users/suryaps/Library/Android/sdk
   ```

## Verify Installation

After installation, verify the SDK is accessible:

```bash
# Check if SDK exists
ls -la ~/Library/Android/sdk

# Check platform-tools
ls -la ~/Library/Android/sdk/platform-tools

# Update local.properties
echo "sdk.dir=$HOME/Library/Android/sdk" > local.properties
```

## Required SDK Components

For this project, you need:
- **Android SDK Platform 34** (for compileSdk 34)
- **Android SDK Build-Tools 34.0.0**
- **CMake 3.22.1** (for NDK builds)
- **NDK (r26.1.10909125 or compatible)** (for native code compilation)
- **Platform Tools** (for adb)

## After Setup

Once the SDK is installed and `local.properties` is updated:

```bash
cd /Users/suryaps/Desktop/Dump/opencv-edge-detector
./gradlew assembleDebug
```

## Troubleshooting

### SDK Not Found
- Verify the path in `local.properties` is correct
- Check that `~/Library/Android/sdk` exists
- Ensure you have write permissions

### NDK Not Found
- Install NDK via Android Studio SDK Manager
- Or via command line: `sdkmanager "ndk;26.1.10909125"`

### CMake Not Found
- Install CMake via Android Studio SDK Manager
- Or via command line: `sdkmanager "cmake;3.22.1"`

