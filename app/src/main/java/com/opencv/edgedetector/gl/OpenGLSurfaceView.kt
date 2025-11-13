package com.opencv.edgedetector.gl

import android.content.Context
import android.graphics.SurfaceTexture
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.view.Surface
import android.opengl.GLES20
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class OpenGLSurfaceView : GLSurfaceView {
    
    private lateinit var renderer: OpenGLRenderer
    var surfaceTexture: SurfaceTexture? = null
        get() = if (::renderer.isInitialized) renderer.getSurfaceTexture() else null
    
    constructor(context: Context) : super(context) {
        initialize(context)
    }
    
    constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
        initialize(context)
    }
    
    private fun initialize(context: Context) {
        setEGLContextClientVersion(2)
        renderer = OpenGLRenderer(context, this)
        setRenderer(renderer)
        renderMode = RENDERMODE_CONTINUOUSLY
    }
    
    fun setProcessingMode(edgeDetection: Boolean) {
        if (::renderer.isInitialized) {
            renderer.setProcessingMode(edgeDetection)
        }
    }
    
    fun setFpsCallback(callback: FpsCallback) {
        if (::renderer.isInitialized) {
            renderer.setFpsCallback(callback)
        }
    }
    
    fun setCameraRotation(rotation: Int, isFrontCamera: Boolean) {
        if (::renderer.isInitialized) {
            renderer.setCameraRotation(rotation, isFrontCamera)
        }
    }
    
    companion object {
        init {
            System.loadLibrary("opencv_edge_detector")
        }
    }
    
    inner class OpenGLRenderer(
        private val context: Context,
        private val glSurfaceView: OpenGLSurfaceView
    ) : GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {
        private var nativeRenderer: Long = 0
        private var processingMode = true
        private var fpsCallback: FpsCallback? = null
        private var cameraSurfaceTexture: SurfaceTexture? = null
        private var cameraSurface: Surface? = null
        private var textureId: Int = 0
        private var cameraRotation: Int = 0
        private var isFrontCamera: Boolean = false
        
        init {
            nativeRenderer = nativeInit()
        }
        
        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            // Generate texture for SurfaceTexture using GLES20
            val textures = IntArray(1)
            GLES20.glGenTextures(1, textures, 0)
            textureId = textures[0]
            
            // GL_TEXTURE_EXTERNAL_OES constant value
            val GL_TEXTURE_EXTERNAL_OES = 0x8D65
            
            // Bind as external texture
            GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId)
            GLES20.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR)
            GLES20.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR)
            GLES20.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE)
            GLES20.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE)
            
            // Create SurfaceTexture with the texture
            cameraSurfaceTexture = SurfaceTexture(textureId)
            cameraSurfaceTexture?.setOnFrameAvailableListener(this)
            cameraSurfaceTexture?.setDefaultBufferSize(1280, 720)
            
            // Create Surface from SurfaceTexture for camera
            cameraSurface = Surface(cameraSurfaceTexture)
            
            // Initialize native renderer
            nativeOnSurfaceCreated(nativeRenderer, textureId)
        }
        
        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
            nativeOnSurfaceChanged(nativeRenderer, width, height)
        }
        
        override fun onDrawFrame(gl: GL10?) {
            // Update the SurfaceTexture to get latest frame
            cameraSurfaceTexture?.updateTexImage()
            
            // Render frame
            nativeOnDrawFrame(nativeRenderer, processingMode)
        }
        
        override fun onFrameAvailable(surfaceTexture: SurfaceTexture?) {
            // Request render when new frame is available
            glSurfaceView.requestRender()
        }
        
        fun getSurfaceTexture(): SurfaceTexture? {
            return cameraSurfaceTexture
        }
        
        fun getCameraSurface(): Surface? {
            return cameraSurface
        }
        
        fun setProcessingMode(edgeDetection: Boolean) {
            processingMode = edgeDetection
        }
        
        fun setFpsCallback(callback: FpsCallback) {
            fpsCallback = callback
            nativeSetFpsCallback(nativeRenderer, callback)
        }
        
        fun setCameraRotation(rotation: Int, isFront: Boolean) {
            cameraRotation = rotation
            isFrontCamera = isFront
            nativeSetCameraRotation(nativeRenderer, rotation, isFront)
        }
        
        protected fun finalize() {
            cameraSurfaceTexture?.release()
            cameraSurface?.release()
            nativeRelease(nativeRenderer)
        }
        
        fun processFrame(frameData: ByteArray, width: Int, height: Int) {
            nativeProcessFrame(nativeRenderer, frameData, width, height)
        }
        
        // Native methods
        private external fun nativeInit(): Long
        private external fun nativeOnSurfaceCreated(renderer: Long, textureId: Int)
        private external fun nativeOnSurfaceChanged(renderer: Long, width: Int, height: Int)
        private external fun nativeOnDrawFrame(renderer: Long, processEdges: Boolean)
        private external fun nativeSetFpsCallback(renderer: Long, callback: FpsCallback)
        private external fun nativeSetCameraRotation(renderer: Long, rotation: Int, isFrontCamera: Boolean)
        private external fun nativeProcessFrame(renderer: Long, frameData: ByteArray, width: Int, height: Int)
        private external fun nativeRelease(renderer: Long)
    }
}

