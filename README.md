# OpenCV Edge Detection - Android + OpenGL + Web Viewer

A real-time edge detection Android application that captures camera frames, processes them using OpenCV (C++), and displays the results using OpenGL ES. Includes a TypeScript-based web viewer for displaying processed frames.

## 📋 Features Implemented

### Android App
- ✅ Camera feed integration using Camera2 API
- ✅ Real-time frame capture via TextureView/SurfaceTexture
- ✅ OpenCV C++ processing (Canny edge detection) via JNI
- ✅ OpenGL ES 2.0 rendering for processed frames
- ✅ Toggle between raw camera feed and edge-detected output
- ✅ FPS counter display
- ✅ Smooth real-time performance (target: 10-15 FPS)

### Web Viewer
- ✅ TypeScript-based web viewer
- ✅ Display processed frames (base64/image data)
- ✅ Frame statistics display (FPS, resolution, processing time)
- ✅ Modular, buildable TypeScript project
- ✅ Modern, responsive UI

## 🏗️ Architecture

### Project Structure
```
opencv-edge-detector/
├── app/                          # Android application
│   ├── src/main/
│   │   ├── java/com/opencv/edgedetector/
│   │   │   ├── MainActivity.kt          # Main activity
│   │   │   ├── NativeBridge.kt          # JNI bridge interface
│   │   │   ├── camera/
│   │   │   │   └── CameraManager.kt    # Camera2 API manager
│   │   │   └── gl/
│   │   │       └── OpenGLSurfaceView.kt # OpenGL renderer
│   │   ├── cpp/
│   │   │   ├── native_renderer.cpp      # C++ OpenCV processing
│   │   │   └── CMakeLists.txt           # NDK build config
│   │   └── res/                         # Android resources
│   └── build.gradle                     # App build config
├── web/                          # TypeScript web viewer
│   ├── src/
│   │   ├── viewer.ts                    # Viewer class
│   │   └── index.ts                     # Main entry point
│   ├── index.html                       # Web page
│   ├── package.json                     # Node dependencies
│   └── tsconfig.json                    # TypeScript config
└── README.md
```

### Frame Flow
1. **Camera Capture**: Camera2 API captures frames via TextureView
2. **Frame Processing**: Frames are sent to native C++ code via JNI
3. **OpenCV Processing**: Canny edge detection applied in C++
4. **OpenGL Rendering**: Processed frames rendered as OpenGL textures
5. **Display**: Results displayed on screen with FPS counter

### JNI Communication
- Java/Kotlin ↔ C++ bridge using JNI
- Native methods for OpenGL initialization and frame processing
- Efficient memory management for frame data transfer

## 🚀 Setup Instructions

### Prerequisites
- Android Studio (Arctic Fox or later)
- Android NDK (r21 or later)
- OpenCV Android SDK (4.5.0 or later)
- Node.js and npm (for web viewer)
- Android device/emulator with camera support

### Android Setup

1. **Install OpenCV Android SDK**
   ```bash
   # Download OpenCV Android SDK from https://opencv.org/releases/
   # Extract to a directory (e.g., ~/opencv-sdk)
   ```

2. **Configure OpenCV in CMakeLists.txt**
   - Update `CMakeLists.txt` to point to your OpenCV installation:
   ```cmake
   set(OpenCV_DIR "path/to/opencv-sdk/sdk/native/jni")
   ```

3. **Build the Project**
   ```bash
   cd opencv-edge-detector
   ./gradlew assembleDebug
   ```

4. **Install on Device**
   ```bash
   ./gradlew installDebug
   ```

### Web Viewer Setup

1. **Install Dependencies**
   ```bash
   cd web
   npm install
   ```

2. **Build TypeScript**
   ```bash
   npm run build
   ```

3. **Serve the Web Page**
   ```bash
   npm run serve
   # Or use any HTTP server:
   # python -m http.server 8080
   # npx http-server . -p 8080
   ```

4. **Open in Browser**
   - Navigate to `http://localhost:8080`

## 📱 Usage

### Android App
1. Launch the app on your Android device
2. Grant camera permission when prompted
3. The app will start displaying the camera feed
4. Tap "Toggle Mode" to switch between:
   - Edge Detection mode (Canny edges)
   - Raw Camera mode (original feed)
5. FPS counter shows real-time performance

### Web Viewer
1. Open `index.html` in a web browser
2. View the sample edge-detected pattern
3. Upload a processed frame image (PNG/JPG) to test
4. Monitor frame statistics (FPS, resolution, processing time)

## 🔧 Technical Details

### OpenCV Processing (C++ Implementation)
- **Algorithm**: Canny Edge Detection
- **Implementation**: Full CPU-based processing with GPU-CPU data transfer
- **Processing Pipeline**:
  1. Render camera texture (GL_TEXTURE_EXTERNAL_OES) to Framebuffer Object (FBO)
  2. Read pixels from FBO to CPU memory using `glReadPixels`
  3. Convert RGBA to Grayscale using `cv::cvtColor`
  4. Apply Canny edge detection with thresholds (50, 150)
  5. Convert result back to RGBA
  6. Upload processed frame to GL_TEXTURE_2D
  7. Render processed texture to screen
- **Parameters**: 
  - Low threshold: 50
  - High threshold: 150
- **Performance**: ~10-15 FPS with edge detection enabled

### OpenGL ES Architecture
- **Version**: OpenGL ES 2.0
- **Two-Pass Rendering**:
  - Pass 1: Camera → FBO (for CPU access)
  - Pass 2: Processed texture → Screen
- **Dual Shader Programs**:
  - Program 1: External OES texture shader (camera input)
  - Program 2: 2D texture shader (processed output)
- **Texture Format**: RGBA8888
- **Rotation Handling**: Shader-based texture coordinate rotation

### JNI Bridge
- **Native Methods**:
  - `nativeInit`: Initialize renderer state
  - `nativeOnSurfaceCreated`: Setup OpenGL resources
  - `nativeOnSurfaceChanged`: Handle viewport changes
  - `nativeOnDrawFrame`: Main rendering loop with edge detection
  - `nativeSetCameraRotation`: Configure camera orientation
  - `nativeSetFpsCallback`: FPS reporting to Java
  - `nativeRelease`: Cleanup resources
- **Data Flow**: Java Camera2 → SurfaceTexture → Native C++ → OpenCV → OpenGL

### Performance Optimization
- Framebuffer Objects (FBO) for efficient texture access
- Direct pixel buffer operations
- Efficient Mat memory management with `clone()`
- FPS tracking and monitoring

## 📸 Screenshots

_Add screenshots or GIFs of the working app here_

## 🧪 Testing

### Android
- Test on physical device with camera
- Verify edge detection accuracy
- Monitor FPS performance
- Test mode toggle functionality

### Web Viewer
- Test with sample images
- Verify statistics display
- Test file upload functionality

## 📝 Notes

- OpenCV Android SDK must be properly configured in CMakeLists.txt
- Camera permissions are required at runtime
- Minimum Android SDK: 24 (Android 7.0)
- Target Android SDK: 34

## 🔮 Future Enhancements

- [ ] WebSocket integration for real-time frame streaming
- [ ] Additional OpenCV filters (blur, threshold, etc.)
- [ ] OpenGL shader effects
- [ ] Save processed frames to gallery
- [ ] Adjustable edge detection parameters

## 📄 License

MIT License

## 👤 Author

RnD Intern Assessment Project

---

**⚠️ Important**: This project requires proper Git version control. Ensure all changes are committed with meaningful commit messages.

