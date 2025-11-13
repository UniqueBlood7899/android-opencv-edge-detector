import { EdgeDetectionViewer, FrameStats } from './viewer';

/**
 * Main entry point for the web viewer
 */
class App {
    private viewer: EdgeDetectionViewer;
    
    constructor() {
        // Initialize viewer
        this.viewer = new EdgeDetectionViewer('frameCanvas', 'statsContainer');
        
        // Load sample image if available
        this.loadSampleImage();
        
        // Set up demo mode
        this.setupDemoMode();
    }
    
    /**
     * Load a sample processed frame (placeholder)
     */
    private loadSampleImage(): void {
        // In a real scenario, this would load from an API endpoint
        // For demo purposes, we'll create a sample edge-detected image
        this.createSampleEdgeImage();
    }
    
    /**
     * Create a sample edge-detected image for demonstration
     */
    private createSampleEdgeImage(): void {
        const canvas = document.getElementById('frameCanvas') as HTMLCanvasElement;
        if (!canvas) return;
        
        const ctx = canvas.getContext('2d');
        if (!ctx) return;
        
        canvas.width = 640;
        canvas.height = 480;
        
        // Draw a simple pattern with edges
        ctx.fillStyle = '#000000';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        
        // Draw some shapes that will show edges
        ctx.strokeStyle = '#FFFFFF';
        ctx.lineWidth = 2;
        
        // Rectangle
        ctx.strokeRect(50, 50, 200, 150);
        
        // Circle
        ctx.beginPath();
        ctx.arc(400, 200, 80, 0, 2 * Math.PI);
        ctx.stroke();
        
        // Triangle
        ctx.beginPath();
        ctx.moveTo(300, 350);
        ctx.lineTo(450, 350);
        ctx.lineTo(375, 280);
        ctx.closePath();
        ctx.stroke();
        
        // Update stats
        const stats: Partial<FrameStats> = {
            fps: 15,
            processingTime: 33,
            timestamp: Date.now()
        };
        
        this.viewer.displayFrame('', stats);
    }
    
    /**
     * Set up demo mode with simulated frame updates
     */
    private setupDemoMode(): void {
        // Simulate frame updates every 66ms (~15 FPS)
        setInterval(() => {
            const stats: Partial<FrameStats> = {
                fps: Math.floor(Math.random() * 5) + 12, // 12-17 FPS
                processingTime: Math.floor(Math.random() * 10) + 28, // 28-38ms
                timestamp: Date.now()
            };
            
            // In a real implementation, this would receive actual frame data
            // For now, we just update the stats by displaying empty frame with new stats
            this.viewer.displayFrame('', stats);
        }, 1000);
    }
    
    /**
     * Handle file upload (for testing with saved frames)
     */
    public handleFileUpload(file: File): void {
        const reader = new FileReader();
        reader.onload = (e) => {
            if (e.target?.result) {
                this.viewer.displayFrame(e.target.result as string);
            }
        };
        reader.readAsDataURL(file);
    }
}

// Initialize app when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        new App();
    });
} else {
    new App();
}

// Export for potential module use
export { App };

