#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <chrono>
#include <mutex>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

// OpenGL shader sources
const char* vertexShaderSource = R"(
attribute vec4 aPosition;
attribute vec2 aTexCoord;
uniform float uRotation;
uniform bool uIsFrontCamera;
varying vec2 vTexCoord;
void main() {
    gl_Position = aPosition;
    
    // Rotate texture coordinates based on rotation
    vec2 texCoord = aTexCoord;
    
    // Apply rotation
    float angle = uRotation * 3.14159265359 / 180.0;
    float cosA = cos(angle);
    float sinA = sin(angle);
    
    // Translate to center, rotate, translate back
    texCoord -= 0.5;
    float newX = texCoord.x * cosA - texCoord.y * sinA;
    float newY = texCoord.x * sinA + texCoord.y * cosA;
    texCoord = vec2(newX, newY) + 0.5;
    
    // Mirror horizontally for front camera
    if (uIsFrontCamera) {
        texCoord.x = 1.0 - texCoord.x;
    }
    
    vTexCoord = texCoord;
}
)";

const char* fragmentShaderSource = R"(
#extension GL_OES_EGL_image_external : require
precision mediump float;
uniform samplerExternalOES uTexture;
varying vec2 vTexCoord;
void main() {
    gl_FragColor = texture2D(uTexture, vTexCoord);
}
)";

// Fragment shader for regular 2D texture
const char* fragmentShader2DSource = R"(
precision mediump float;
uniform sampler2D uTexture;
varying vec2 vTexCoord;
void main() {
    gl_FragColor = texture2D(uTexture, vTexCoord);
}
)";

struct RendererState {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;
    
    GLuint program;
    GLuint program2D;  // Program for 2D textures
    GLuint cameraTextureId;  // Texture from SurfaceTexture
    GLuint outputTextureId;  // Texture for processed output
    GLuint fbo;  // Framebuffer for intermediate rendering
    GLuint vertexBuffer;
    
    int width;
    int height;
    int cameraWidth;
    int cameraHeight;
    
    ANativeWindow* window;
    
    bool processingMode;
    int frameCount;
    std::chrono::steady_clock::time_point lastFpsTime;
    int currentFps;
    
    int cameraRotation;  // Rotation in degrees (0, 90, 180, 270)
    bool isFrontCamera;
    
    std::mutex frameMutex;
    cv::Mat currentFrame;
    bool frameReady;
    
    jobject fpsCallback;
    JavaVM* jvm;
};

static RendererState* g_renderer = nullptr;

// Helper function to compile shader
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            delete[] infoLog;
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// Helper function to create shader program
GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) return 0;
    
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }
    
    GLuint program = glCreateProgram();
    if (program == 0) return 0;
    
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetProgramInfoLog(program, infoLen, nullptr, infoLog);
            delete[] infoLog;
        }
        glDeleteProgram(program);
        return 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_opencv_edgedetector_gl_OpenGLSurfaceView_00024OpenGLRenderer_nativeInit(JNIEnv *env, jobject thiz) {
    RendererState* renderer = new RendererState();
    renderer->display = EGL_NO_DISPLAY;
    renderer->surface = EGL_NO_SURFACE;
    renderer->context = EGL_NO_CONTEXT;
    renderer->window = nullptr;
    renderer->width = 0;
    renderer->height = 0;
    renderer->cameraWidth = 1280;
    renderer->cameraHeight = 720;
    renderer->processingMode = true;
    renderer->frameCount = 0;
    renderer->currentFps = 0;
    renderer->frameReady = false;
    renderer->fpsCallback = nullptr;
    renderer->lastFpsTime = std::chrono::steady_clock::now();
    renderer->cameraRotation = 0;
    renderer->isFrontCamera = false;
    renderer->fbo = 0;
    renderer->program2D = 0;
    
    env->GetJavaVM(&renderer->jvm);
    
    g_renderer = renderer;
    return reinterpret_cast<jlong>(renderer);
}

extern "C" JNIEXPORT void JNICALL
Java_com_opencv_edgedetector_gl_OpenGLSurfaceView_00024OpenGLRenderer_nativeOnSurfaceCreated(JNIEnv *env, jobject thiz, jlong rendererPtr, jint textureId) {
    RendererState* renderer = reinterpret_cast<RendererState*>(rendererPtr);
    
    // Store the camera texture ID from SurfaceTexture
    renderer->cameraTextureId = textureId;
    
    // Create shader program for external textures
    renderer->program = createProgram(vertexShaderSource, fragmentShaderSource);
    
    // Create shader program for 2D textures
    renderer->program2D = createProgram(vertexShaderSource, fragmentShader2DSource);
    
    // Create output texture for processed frames
    glGenTextures(1, &renderer->outputTextureId);
    glBindTexture(GL_TEXTURE_2D, renderer->outputTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create framebuffer for intermediate rendering
    glGenFramebuffers(1, &renderer->fbo);
    
    // Set up camera texture parameters
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, renderer->cameraTextureId);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_opencv_edgedetector_gl_OpenGLSurfaceView_00024OpenGLRenderer_nativeOnSurfaceChanged(JNIEnv *env, jobject thiz, jlong rendererPtr, jint width, jint height) {
    RendererState* renderer = reinterpret_cast<RendererState*>(rendererPtr);
    renderer->width = width;
    renderer->height = height;
    
    glViewport(0, 0, width, height);
}

// Helper function to process frame with Canny edge detection
cv::Mat processFrameWithCanny(const cv::Mat& input) {
    cv::Mat gray, edges, result;
    
    // Convert to grayscale
    cv::cvtColor(input, gray, cv::COLOR_RGBA2GRAY);
    
    // Apply Canny edge detection
    // Low threshold: 50, High threshold: 150
    cv::Canny(gray, edges, 50, 150);
    
    // Convert back to RGBA for display
    cv::cvtColor(edges, result, cv::COLOR_GRAY2RGBA);
    
    return result;
}

// Helper function to read texture from GPU to CPU
cv::Mat readTextureToMat(GLuint textureId, int width, int height) {
    // Create buffer for RGBA data
    std::vector<unsigned char> pixels(width * height * 4);
    
    // Create framebuffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    
    // Check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        // Read pixels
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    }
    
    // Clean up
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    
    // Create OpenCV Mat from pixels (flip vertically as OpenGL has origin at bottom-left)
    cv::Mat mat(height, width, CV_8UC4, pixels.data());
    cv::Mat flipped;
    cv::flip(mat, flipped, 0);
    
    return flipped.clone();
}

// Helper function to upload Mat to GPU texture
void uploadMatToTexture(GLuint textureId, const cv::Mat& mat) {
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mat.cols, mat.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, mat.data);
}

extern "C" JNIEXPORT void JNICALL
Java_com_opencv_edgedetector_gl_OpenGLSurfaceView_00024OpenGLRenderer_nativeOnDrawFrame(JNIEnv *env, jobject thiz, jlong rendererPtr, jboolean processEdges) {
    RendererState* renderer = reinterpret_cast<RendererState*>(rendererPtr);
    
    if (renderer->cameraTextureId == 0) {
        return;
    }
    
    // Clear screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    GLuint textureToRender = renderer->cameraTextureId;
    GLenum textureTarget = GL_TEXTURE_EXTERNAL_OES;
    
    // If edge detection is enabled, process the frame
    if (processEdges && renderer->width > 0 && renderer->height > 0) {
        // Step 1: Render camera texture to FBO to get it as regular 2D texture
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);
        
        // Make sure output texture has correct size
        glBindTexture(GL_TEXTURE_2D, renderer->outputTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderer->cameraWidth, renderer->cameraHeight, 
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        
        // Attach texture to FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                              renderer->outputTextureId, 0);
        
        // Check FBO status
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            // Render camera texture to FBO
            glViewport(0, 0, renderer->cameraWidth, renderer->cameraHeight);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            glUseProgram(renderer->program);
            
            GLint posLoc = glGetAttribLocation(renderer->program, "aPosition");
            GLint texLoc = glGetAttribLocation(renderer->program, "aTexCoord");
            GLint texUniform = glGetUniformLocation(renderer->program, "uTexture");
            GLint rotLoc = glGetUniformLocation(renderer->program, "uRotation");
            GLint frontLoc = glGetUniformLocation(renderer->program, "uIsFrontCamera");
            
            float verts[] = {
                -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
                 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
                -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
                 1.0f,  1.0f, 0.0f,  1.0f, 1.0f
            };
            
            glEnableVertexAttribArray(posLoc);
            glEnableVertexAttribArray(texLoc);
            glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), verts);
            glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), verts + 3);
            
            glUniform1f(rotLoc, 0.0f);  // No rotation in FBO pass
            glUniform1i(frontLoc, 0);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, renderer->cameraTextureId);
            glUniform1i(texUniform, 0);
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            glDisableVertexAttribArray(posLoc);
            glDisableVertexAttribArray(texLoc);
            
            // Step 2: Read pixels from FBO
            std::vector<unsigned char> pixels(renderer->cameraWidth * renderer->cameraHeight * 4);
            glReadPixels(0, 0, renderer->cameraWidth, renderer->cameraHeight, 
                        GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            
            // Step 3: Process with OpenCV
            cv::Mat frameMat(renderer->cameraHeight, renderer->cameraWidth, CV_8UC4, pixels.data());
            cv::Mat flipped;
            cv::flip(frameMat, flipped, 0);  // Flip vertically (OpenGL origin is bottom-left)
            
            // Apply Canny edge detection
            cv::Mat processed = processFrameWithCanny(flipped);
            
            // Step 4: Upload processed frame back to texture
            cv::flip(processed, frameMat, 0);  // Flip back for OpenGL
            glBindTexture(GL_TEXTURE_2D, renderer->outputTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderer->cameraWidth, renderer->cameraHeight,
                        0, GL_RGBA, GL_UNSIGNED_BYTE, frameMat.data);
        }
        
        // Unbind FBO
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // Use processed texture for final render
        textureToRender = renderer->outputTextureId;
        textureTarget = GL_TEXTURE_2D;
        
        // Restore viewport
        glViewport(0, 0, renderer->width, renderer->height);
    }
    
    // Draw quad with camera texture
    glUseProgram(textureTarget == GL_TEXTURE_EXTERNAL_OES ? renderer->program : renderer->program2D);
    
    // Simple quad vertices (full screen)
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };
    
    GLuint currentProgram = textureTarget == GL_TEXTURE_EXTERNAL_OES ? renderer->program : renderer->program2D;
    
    GLint positionLoc = glGetAttribLocation(currentProgram, "aPosition");
    GLint texCoordLoc = glGetAttribLocation(currentProgram, "aTexCoord");
    GLint textureLoc = glGetUniformLocation(currentProgram, "uTexture");
    GLint rotationLoc = glGetUniformLocation(currentProgram, "uRotation");
    GLint isFrontCameraLoc = glGetUniformLocation(currentProgram, "uIsFrontCamera");
    
    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(texCoordLoc);
    
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), vertices);
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), vertices + 3);
    
    // Set rotation and front camera uniforms
    glUniform1f(rotationLoc, static_cast<GLfloat>(renderer->cameraRotation));
    glUniform1i(isFrontCameraLoc, renderer->isFrontCamera ? 1 : 0);
    
    // Bind camera texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(textureTarget, textureToRender);
    glUniform1i(textureLoc, 0);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glDisableVertexAttribArray(positionLoc);
    glDisableVertexAttribArray(texCoordLoc);
    
    // Update FPS
    renderer->frameCount++;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - renderer->lastFpsTime).count();
    
    if (elapsed >= 1000) {
        renderer->currentFps = (renderer->frameCount * 1000) / elapsed;
        renderer->frameCount = 0;
        renderer->lastFpsTime = now;
        
        // Call Java callback
        if (renderer->fpsCallback != nullptr) {
            JNIEnv* jniEnv;
            jint result = renderer->jvm->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6);
            if (result == JNI_OK) {
                jclass callbackClass = jniEnv->GetObjectClass(renderer->fpsCallback);
                jmethodID onFpsUpdateMethod = jniEnv->GetMethodID(callbackClass, "onFpsUpdate", "(I)V");
                if (onFpsUpdateMethod != nullptr) {
                    jniEnv->CallVoidMethod(renderer->fpsCallback, onFpsUpdateMethod, renderer->currentFps);
                }
            } else if (result == JNI_EDETACHED) {
                // Attach thread if needed
                jniEnv = nullptr;
                if (renderer->jvm->AttachCurrentThread(&jniEnv, nullptr) == JNI_OK) {
                    jclass callbackClass = jniEnv->GetObjectClass(renderer->fpsCallback);
                    jmethodID onFpsUpdateMethod = jniEnv->GetMethodID(callbackClass, "onFpsUpdate", "(I)V");
                    if (onFpsUpdateMethod != nullptr) {
                        jniEnv->CallVoidMethod(renderer->fpsCallback, onFpsUpdateMethod, renderer->currentFps);
                    }
                    renderer->jvm->DetachCurrentThread();
                }
            }
        }
    }
    
    eglSwapBuffers(renderer->display, renderer->surface);
}

extern "C" JNIEXPORT void JNICALL
Java_com_opencv_edgedetector_gl_OpenGLSurfaceView_00024OpenGLRenderer_nativeSetCameraRotation(JNIEnv *env, jobject thiz, jlong rendererPtr, jint rotation, jboolean isFrontCamera) {
    RendererState* renderer = reinterpret_cast<RendererState*>(rendererPtr);
    renderer->cameraRotation = rotation;
    renderer->isFrontCamera = isFrontCamera;
}

extern "C" JNIEXPORT void JNICALL
Java_com_opencv_edgedetector_gl_OpenGLSurfaceView_00024OpenGLRenderer_nativeSetFpsCallback(JNIEnv *env, jobject thiz, jlong rendererPtr, jobject callback) {
    RendererState* renderer = reinterpret_cast<RendererState*>(rendererPtr);
    
    if (renderer->fpsCallback != nullptr) {
        env->DeleteGlobalRef(renderer->fpsCallback);
    }
    
    if (callback != nullptr) {
        renderer->fpsCallback = env->NewGlobalRef(callback);
    } else {
        renderer->fpsCallback = nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_opencv_edgedetector_gl_OpenGLSurfaceView_00024OpenGLRenderer_nativeProcessFrame(
    JNIEnv *env, jobject thiz, jlong rendererPtr, jbyteArray frameData, jint width, jint height) {
    RendererState* renderer = reinterpret_cast<RendererState*>(rendererPtr);
    
    if (frameData == nullptr || width <= 0 || height <= 0) {
        return;
    }
    
    jsize length = env->GetArrayLength(frameData);
    jbyte* data = env->GetByteArrayElements(frameData, nullptr);
    
    if (data != nullptr) {
        std::lock_guard<std::mutex> lock(renderer->frameMutex);
        
        // Create OpenCV Mat from byte array (RGBA format)
        cv::Mat frame(height, width, CV_8UC4, data);
        renderer->currentFrame = frame.clone();
        renderer->frameReady = true;
        
        env->ReleaseByteArrayElements(frameData, data, JNI_ABORT);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_opencv_edgedetector_gl_OpenGLSurfaceView_00024OpenGLRenderer_nativeRelease(JNIEnv *env, jobject thiz, jlong rendererPtr) {
    RendererState* renderer = reinterpret_cast<RendererState*>(rendererPtr);
    
    if (renderer->fpsCallback != nullptr) {
        env->DeleteGlobalRef(renderer->fpsCallback);
    }
    
    if (renderer->fbo != 0) {
        glDeleteFramebuffers(1, &renderer->fbo);
    }
    
    if (renderer->outputTextureId != 0) {
        glDeleteTextures(1, &renderer->outputTextureId);
    }
    
    if (renderer->program != 0) {
        glDeleteProgram(renderer->program);
    }
    
    if (renderer->program2D != 0) {
        glDeleteProgram(renderer->program2D);
    }
    
    if (renderer->surface != EGL_NO_SURFACE) {
        eglDestroySurface(renderer->display, renderer->surface);
    }
    
    if (renderer->context != EGL_NO_CONTEXT) {
        eglDestroyContext(renderer->display, renderer->context);
    }
    
    if (renderer->display != EGL_NO_DISPLAY) {
        eglTerminate(renderer->display);
    }
    
    if (renderer->window != nullptr) {
        ANativeWindow_release(renderer->window);
    }
    
    delete renderer;
}

