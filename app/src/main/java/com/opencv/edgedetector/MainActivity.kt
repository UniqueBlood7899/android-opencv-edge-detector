package com.opencv.edgedetector

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.opencv.edgedetector.camera.CameraManager
import com.opencv.edgedetector.gl.OpenGLSurfaceView

class MainActivity : AppCompatActivity() {
    
    private lateinit var glSurfaceView: OpenGLSurfaceView
    private lateinit var cameraManager: CameraManager
    private lateinit var fpsTextView: TextView
    private lateinit var modeTextView: TextView
    private lateinit var toggleButton: Button
    
    private var isEdgeDetectionMode = true
    
    companion object {
        private const val CAMERA_PERMISSION_REQUEST_CODE = 100
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        glSurfaceView = findViewById(R.id.glSurfaceView)
        fpsTextView = findViewById(R.id.fpsTextView)
        modeTextView = findViewById(R.id.modeTextView)
        toggleButton = findViewById(R.id.toggleButton)
        
        // Initialize native library
        System.loadLibrary("opencv_edge_detector")
        
        // Request camera permission
        if (checkCameraPermission()) {
            initializeCamera()
        } else {
            requestCameraPermission()
        }
        
        toggleButton.setOnClickListener {
            isEdgeDetectionMode = !isEdgeDetectionMode
            glSurfaceView.setProcessingMode(isEdgeDetectionMode)
            modeTextView.text = if (isEdgeDetectionMode) {
                "Mode: Edge Detection"
            } else {
                "Mode: Raw Camera"
            }
        }
        
        // Update FPS counter
        glSurfaceView.setFpsCallback(object : com.opencv.edgedetector.gl.FpsCallback {
            override fun onFpsUpdate(fps: Int) {
                runOnUiThread {
                    fpsTextView.text = "FPS: $fps"
                }
            }
        })
    }
    
    private fun checkCameraPermission(): Boolean {
        return ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.CAMERA
        ) == PackageManager.PERMISSION_GRANTED
    }
    
    private fun requestCameraPermission() {
        ActivityCompat.requestPermissions(
            this,
            arrayOf(Manifest.permission.CAMERA),
            CAMERA_PERMISSION_REQUEST_CODE
        )
    }
    
    private fun initializeCamera() {
        cameraManager = CameraManager(this, glSurfaceView)
        cameraManager.openCamera()
    }
    
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == CAMERA_PERMISSION_REQUEST_CODE) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                initializeCamera()
            } else {
                Toast.makeText(
                    this,
                    getString(R.string.camera_permission_required),
                    Toast.LENGTH_LONG
                ).show()
            }
        }
    }
    
    override fun onResume() {
        super.onResume()
        cameraManager?.let {
            if (checkCameraPermission()) {
                it.openCamera()
            }
        }
    }
    
    override fun onPause() {
        super.onPause()
        cameraManager?.closeCamera()
    }
}

