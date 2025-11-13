package com.opencv.edgedetector

object NativeBridge {
    init {
        System.loadLibrary("opencv_edge_detector")
    }
    
    /**
     * Process image using OpenCV Canny edge detection
     * @param inputData Input image data (RGBA bytes)
     * @param width Image width
     * @param height Image height
     * @param outputData Output buffer for processed image
     * @return Processing time in milliseconds
     */
    external fun processImage(
        inputData: ByteArray,
        width: Int,
        height: Int,
        outputData: ByteArray
    ): Long
    
    /**
     * Initialize OpenCV
     */
    external fun initOpenCV()
}

