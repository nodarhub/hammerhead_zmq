# QA Findings Viewer

Real-time viewer for quality assurance findings published by Hammerhead. Displays system monitoring, Hammerhead performance metrics, and image quality assessments.

## Installation

```bash
pip install -e examples/python/qa_findings_viewer
```

## Usage

```bash
python qa_findings_viewer.py <src_ip>
```

### Parameters

- `src_ip`: IP address of the device running Hammerhead

### Examples

```bash
# View QA findings from local Hammerhead instance
python qa_findings_viewer.py 127.0.0.1

# View QA findings from remote Hammerhead device
python qa_findings_viewer.py 10.10.1.10
```

## QA Findings Types

The viewer displays three categories of quality assurance findings:

### System Monitoring
- **CPU Temperature**: Warning ≥90°C, Error ≥99°C
- **GPU Temperature**: Warning ≥90°C, Error ≥99°C  
- **GPU Utilization**: Warning ≥99.0%, Error ≥99.5%
- **GPU Memory**: Warning ≥80%, Error ≥90%
- **RAM Usage**: Warning ≥80%, Error ≥90%
- **CPU Load**: Warning ≥11.0, Error ≥20.0 (load average)
- **Disk Space**: Warning ≤10GB free per mount point

### Hammerhead Monitoring
- **Frame Sync**: Warning ≥50μs, Error ≥1000μs (synchronization timing between cameras)
- **FOM (Figure of Merit)**: Stereo vision quality metrics (0.0-1.0 scale, higher = better):
  - **Main FOM**: Warning ≤0.4, Error ≤0.1 (overall stereo matching quality)
  - **FOM Cal 1**: Warning ≤0.4, Error ≤0.1 (first calibration quality metric)
  - **FOM Cal 2**: Warning ≤0.4, Error ≤0.1 (second calibration quality metric)

### Image Monitoring
- **Underexposure**: Warning ≤-1.5 (exposure assessment)
  - *Detection method*: Analyzes pixel intensity histograms to identify when too many pixels are dark/clipped to black
  - *Metric meaning*: Negative values indicate underexposure severity; more negative = more underexposed
- **Overexposure**: Warning ≥0.3 (exposure assessment)  
  - *Detection method*: Analyzes pixel intensity histograms to identify when too many pixels are bright/clipped to white
  - *Metric meaning*: Positive values indicate overexposure severity; higher values = more overexposed
- **Blur**: Warning ≤0.58 (image sharpness, lower = more blurry)
  - *Detection method*: Uses DFT (Discrete Fourier Transform) to analyze frequency domain energy content
  - *Metric meaning*: Higher energy in high frequencies indicates sharper images; values below 0.58 indicate insufficient sharpness for reliable stereo matching

## Output Format

Each QA finding is displayed with:
- **Frame ID**: Sequential frame identifier from the stereo camera system
- **Timestamp**: When the finding was detected (corresponds to left image timestamp)
- **Domain**: Category (system/hammerhead/image)
- **Key**: Specific metric identifier
- **Severity**: ERROR, WARNING, or INFO
- **Message**: Human-readable description
- **Value**: Numeric measurement
- **Unit**: Measurement unit

A summary is provided showing the total count of INFO, WARNING, and ERROR findings for each frame.

## Monitoring Frequency

- **System Monitoring**: Periodic checks every 5000ms (5 seconds)
- **Hammerhead Monitoring**: Per-frame checks
- **Image Monitoring**: Per-frame checks

## Configuration

To enable QA findings publishing in Hammerhead, set the following in `master_config.ini`:

```ini
# Enable quality assurance monitoring
enable_quality_assurance = 1

# Enable specific monitoring types
enable_system_monitor = 1
enable_hammerhead_monitor = 1
enable_image_monitor = 1

# Enable QA findings publishing
publish_qa_findings = 1
```

## Features

- Real-time QA findings display with Python
- Frame ID tracking
- Persistent connection handling

## Troubleshooting

### General Issues
- **No findings appear**: Check that Hammerhead is running with QA enabled and `publish_qa_findings = 1`
- **Connection timeout**: Verify network connectivity and IP address

### System Monitoring Issues
- **High CPU/GPU Temperature**: Improve cooling, check thermal paste, verify fan operation
- **High RAM Memory**: Close unnecessary applications, increase swap space if needed
- **High CPU Load**: Identify resource-intensive processes, reduce concurrent operations
- **Low Disk Space**: Clean up temporary files, archive old data, expand storage

### Hammerhead Monitoring Issues
- **Frame Sync Errors**: Check camera connections, verify cable integrity, ensure cameras are properly synchronized
- **Low FOM Values**:
  - Perform initial calibration routine if mounting cameras for the first time
  - Perform refinement calibration periodically  
  - Check for obstructions in camera field of view
  - Ensure cameras are clean and focused
  - Improve lighting conditions

### Image Monitoring Issues
- **Underexposure**: Increase lighting, adjust camera exposure settings, check for lens obstruction
- **Overexposure**: Reduce lighting intensity, adjust camera exposure settings
- **High Blur**: Clean camera lenses, check focus settings, reduce camera shake, verify mounting stability

Press `Ctrl+C` to exit the viewer.