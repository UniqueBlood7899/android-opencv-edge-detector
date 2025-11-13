# Setup Guide

## Quick Start

### 1. Install OpenCV Android SDK

1. Download OpenCV Android SDK from [https://opencv.org/releases/](https://opencv.org/releases/)
2. Extract the SDK to a location (e.g., `~/opencv-sdk`)
3. Update `app/src/main/cpp/CMakeLists.txt`:
   ```cmake
   set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/../../../../opencv-sdk/sdk/native/jni")
   ```
   Or set an environment variable:
   ```bash
   export OPENCV_ANDROID_SDK=/path/to/opencv-sdk
   ```

### 2. Build the Android App

```bash
cd opencv-edge-detector
./gradlew assembleDebug
```

### 3. Install on Device

```bash
./gradlew installDebug
```

### 4. Build Web Viewer

```bash
cd web
npm install
npm run build
npm run serve
```

## Detailed Setup

### Prerequisites

- **Android Studio** (Arctic Fox or later)
- **Android NDK** (r21 or later) - Install via Android Studio SDK Manager
- **OpenCV Android SDK** (4.5.0 or later)
- **Node.js** (v16 or later) - For web viewer
- **Android device** with camera support

### Android Studio Setup

1. Open the project in Android Studio
2. Go to `File > Project Structure > SDK Location`
3. Ensure NDK path is set correctly
4. Sync Gradle files

### OpenCV Configuration

The project uses CMake to build native code. OpenCV must be properly configured:

1. **Download OpenCV Android SDK**
   - Visit https://opencv.org/releases/
   - Download the Android pack
   - Extract to a known location

2. **Update CMakeLists.txt**
   - Open `app/src/main/cpp/CMakeLists.txt`
   - Set `OpenCV_DIR` to point to: `opencv-sdk/sdk/native/jni`
   - Example:
     ```cmake
     set(OpenCV_DIR "/Users/username/opencv-sdk/sdk/native/jni")
     ```

3. **Alternative: Use OpenCV as Android Module**
   - Import OpenCV as a module in your project
   - Update `settings.gradle` to include the module
   - Update `app/build.gradle` to add dependency

### Building

#### Debug Build
```bash
./gradlew assembleDebug
```

#### Release Build
```bash
./gradlew assembleRelease
```

#### Install on Connected Device
```bash
./gradlew installDebug
```

### Troubleshooting

#### OpenCV Not Found
- Verify OpenCV_DIR path in CMakeLists.txt
- Check that OpenCV Android SDK is properly extracted
- Ensure `sdk/native/jni/OpenCVConfig.cmake` exists

#### NDK Issues
- Install NDK via Android Studio SDK Manager
- Set `ndkVersion` in `app/build.gradle` if needed
- Verify NDK path in `local.properties`

#### Build Errors
- Clean and rebuild: `./gradlew clean assembleDebug`
- Invalidate caches in Android Studio: `File > Invalidate Caches`

## Web Viewer Setup

### Install Dependencies
```bash
cd web
npm install
```

### Build TypeScript
```bash
npm run build
```

### Serve Locally
```bash
npm run serve
# Or use Python:
# python -m http.server 8080
```

### Open in Browser
Navigate to `http://localhost:8080`

## Testing

### Android App
1. Connect Android device via USB
2. Enable USB debugging
3. Run: `./gradlew installDebug`
4. Launch app on device
5. Grant camera permission
6. Verify edge detection works

### Web Viewer
1. Build TypeScript: `npm run build`
2. Open `index.html` in browser
3. Test file upload functionality
4. Verify statistics display

## Notes

- Minimum Android SDK: 24 (Android 7.0)
- Target Android SDK: 34
- OpenGL ES 2.0 required
- Camera permission required at runtime

