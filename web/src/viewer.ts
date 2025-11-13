/**
 * OpenCV Edge Detection Web Viewer
 * Displays processed frames from Android app
 */

interface FrameStats {
    fps: number;
    resolution: string;
    processingTime: number;
    timestamp: number;
}

class EdgeDetectionViewer {
    private canvas: HTMLCanvasElement;
    private ctx: CanvasRenderingContext2D;
    private statsElement: HTMLElement;
    private imageElement: HTMLImageElement;
    
    private currentStats: FrameStats = {
        fps: 0,
        resolution: '0x0',
        processingTime: 0,
        timestamp: Date.now()
    };
    
    constructor(canvasId: string, statsId: string) {
        const canvas = document.getElementById(canvasId) as HTMLCanvasElement;
        const stats = document.getElementById(statsId);
        
        if (!canvas || !stats) {
            throw new Error('Required DOM elements not found');
        }
        
        this.canvas = canvas;
        const context = canvas.getContext('2d');
        if (!context) {
            throw new Error('Could not get 2D rendering context');
        }
        this.ctx = context;
        this.statsElement = stats;
        
        this.imageElement = new Image();
        this.imageElement.onload = () => this.drawFrame();
    }
    
    /**
     * Display a processed frame from base64 image data
     */
    public displayFrame(base64Data: string, stats?: Partial<FrameStats>): void {
        if (stats) {
            this.updateStats(stats);
        }
        
        if (base64Data) {
            this.imageElement.src = base64Data;
        } else {
            // Just update stats without changing image
            this.updateStatsDisplay();
        }
    }
    
    /**
     * Display a processed frame from ImageData
     */
    public displayFrameFromData(imageData: ImageData, stats?: Partial<FrameStats>): void {
        if (stats) {
            this.updateStats(stats);
        }
        
        this.canvas.width = imageData.width;
        this.canvas.height = imageData.height;
        this.ctx.putImageData(imageData, 0, 0);
        
        this.updateStatsDisplay();
    }
    
    /**
     * Update frame statistics
     */
    private updateStats(stats: Partial<FrameStats>): void {
        this.currentStats = {
            ...this.currentStats,
            ...stats,
            timestamp: Date.now()
        };
    }
    
    /**
     * Draw the loaded image to canvas
     */
    private drawFrame(): void {
        this.canvas.width = this.imageElement.width;
        this.canvas.height = this.imageElement.height;
        this.ctx.drawImage(this.imageElement, 0, 0);
        this.updateStatsDisplay();
    }
    
    /**
     * Update the statistics display element
     */
    private updateStatsDisplay(): void {
        const resolution = `${this.canvas.width}x${this.canvas.height}`;
        const stats = {
            ...this.currentStats,
            resolution
        };
        
        this.statsElement.innerHTML = `
            <div class="stat-item">
                <span class="stat-label">FPS:</span>
                <span class="stat-value">${stats.fps}</span>
            </div>
            <div class="stat-item">
                <span class="stat-label">Resolution:</span>
                <span class="stat-value">${stats.resolution}</span>
            </div>
            <div class="stat-item">
                <span class="stat-label">Processing Time:</span>
                <span class="stat-value">${stats.processingTime}ms</span>
            </div>
            <div class="stat-item">
                <span class="stat-label">Last Update:</span>
                <span class="stat-value">${new Date(stats.timestamp).toLocaleTimeString()}</span>
            </div>
        `;
    }
    
    /**
     * Get current canvas as base64 image
     */
    public getCanvasAsBase64(): string {
        return this.canvas.toDataURL('image/png');
    }
    
    /**
     * Clear the canvas
     */
    public clear(): void {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    }
}

// Export for use in other modules
export { EdgeDetectionViewer, FrameStats };

