package com.opencv.edgedetector.camera

import android.content.Context
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager as SystemCameraManager
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.os.Handler
import android.os.HandlerThread
import android.util.Size
import android.view.Surface
import android.view.WindowManager
import com.opencv.edgedetector.gl.OpenGLSurfaceView
import java.util.*

class CameraManager(
    private val context: Context,
    private val glSurfaceView: OpenGLSurfaceView
) {
    private var cameraDevice: CameraDevice? = null
    private var captureSession: CameraCaptureSession? = null
    private val cameraManager: SystemCameraManager =
        context.getSystemService(Context.CAMERA_SERVICE) as SystemCameraManager
    
    private var backgroundThread: HandlerThread? = null
    private var backgroundHandler: Handler? = null
    private var cameraId: String? = null
    private var sensorOrientation: Int = 0
    
    private val cameraStateCallback = object : CameraDevice.StateCallback() {
        override fun onOpened(camera: CameraDevice) {
            cameraDevice = camera
            startPreview()
        }
        
        override fun onDisconnected(camera: CameraDevice) {
            camera.close()
            cameraDevice = null
        }
        
        override fun onError(camera: CameraDevice, error: Int) {
            camera.close()
            cameraDevice = null
        }
    }
    
    fun openCamera() {
        startBackgroundThread()
        
        try {
            cameraId = cameraManager.cameraIdList[0]
            val characteristics = cameraManager.getCameraCharacteristics(cameraId!!)
            
            // Get sensor orientation
            sensorOrientation = characteristics.get(
                android.hardware.camera2.CameraCharacteristics.SENSOR_ORIENTATION
            ) ?: 0
            
            // Get display rotation
            val windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
            val displayRotation = when (windowManager.defaultDisplay.rotation) {
                android.view.Surface.ROTATION_0 -> 0
                android.view.Surface.ROTATION_90 -> 90
                android.view.Surface.ROTATION_180 -> 180
                android.view.Surface.ROTATION_270 -> 270
                else -> 0
            }
            
            // Get camera facing
            val facing = characteristics.get(
                android.hardware.camera2.CameraCharacteristics.LENS_FACING
            )
            val isFrontCamera = facing == android.hardware.camera2.CameraCharacteristics.LENS_FACING_FRONT
            
            // Calculate rotation needed to correct orientation
            // The camera sensor is mounted at sensorOrientation degrees (typically 90° for back camera)
            // We need to rotate the image to match the current display orientation
            // For back camera: rotation = (sensorOrientation - displayRotation + 360) % 360
            // For front camera: rotation = (sensorOrientation + displayRotation) % 360 (mirrored)
            val totalRotation = if (isFrontCamera) {
                (sensorOrientation + displayRotation) % 360
            } else {
                (sensorOrientation - displayRotation + 360) % 360
            }
            
            // Pass rotation to OpenGL renderer
            glSurfaceView.setCameraRotation(totalRotation, isFrontCamera)
            
            // Wait for SurfaceTexture to be ready
            val surfaceTexture = glSurfaceView.surfaceTexture
            if (surfaceTexture == null) {
                // SurfaceTexture not ready yet, try again after a short delay
                android.os.Handler(android.os.Looper.getMainLooper()).postDelayed({
                    openCamera()
                }, 100)
                return
            }
            
            cameraManager.openCamera(
                cameraId!!,
                cameraStateCallback,
                backgroundHandler
            )
            
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
    
    private fun startPreview() {
        val surfaceTexture = glSurfaceView.surfaceTexture
        if (surfaceTexture == null) {
            return
        }
        
        surfaceTexture.setDefaultBufferSize(1280, 720)
        val surface = Surface(surfaceTexture)
        
        try {
            val captureRequestBuilder = cameraDevice?.createCaptureRequest(
                CameraDevice.TEMPLATE_PREVIEW
            )
            captureRequestBuilder?.addTarget(surface)
            captureRequestBuilder?.set(
                CaptureRequest.CONTROL_AF_MODE,
                CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
            )
            
            cameraDevice?.createCaptureSession(
                listOf(surface),
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigured(session: CameraCaptureSession) {
                        captureSession = session
                        try {
                            val request = captureRequestBuilder?.build()
                            session.setRepeatingRequest(
                                request!!,
                                object : CameraCaptureSession.CaptureCallback() {
                                    override fun onCaptureCompleted(
                                        session: CameraCaptureSession,
                                        request: CaptureRequest,
                                        result: TotalCaptureResult
                                    ) {
                                        // Frame captured, processing happens in OpenGL renderer
                                    }
                                },
                                backgroundHandler
                            )
                        } catch (e: Exception) {
                            e.printStackTrace()
                        }
                    }
                    
                    override fun onConfigureFailed(session: CameraCaptureSession) {
                        // Handle error
                    }
                },
                backgroundHandler
            )
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
    
    fun closeCamera() {
        captureSession?.close()
        captureSession = null
        cameraDevice?.close()
        cameraDevice = null
        stopBackgroundThread()
    }
    
    private fun startBackgroundThread() {
        backgroundThread = HandlerThread("CameraBackground").also { it.start() }
        backgroundHandler = Handler(backgroundThread?.looper!!)
    }
    
    private fun stopBackgroundThread() {
        backgroundThread?.quitSafely()
        try {
            backgroundThread?.join()
            backgroundThread = null
            backgroundHandler = null
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
    }
}

