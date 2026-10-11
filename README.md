# C++ Low-Latency RTMP Streamer with FFmpeg & NVENC

A high-performance, low-latency video streaming pipeline written in **pure C++** using the **FFmpeg C API** and **NVIDIA NVENC** hardware encoding. 

Designed for live streaming, surveillance, and AI video pipeline integration, this project demonstrates how to build a robust RTMP push stream from scratch without relying on high-level wrappers.

## 🚀 Key Features

- **Hardware-Accelerated Encoding**: Utilizes NVIDIA NVENC (H.264) for real-time 1080p@30fps encoding with minimal CPU overhead.
- **Pure C/C++ Implementation**: Directly uses FFmpeg C API (`libavcodec`, `libavformat`, `libavutil`) for maximum control and performance.
- **Robust RTMP Muxing**: Handles FLV encapsulation, global headers (SPS/PPS), and proper timebase conversions.
- **Low CPU / GPU Footprint**: CPU usage is typically under 5% on a GTX 1080Ti, freeing up resources for AI inference or other processing.
- **Production-Ready Boilerplate**: Clean, well-commented code that can be easily extended into RTSP-to-RTMP transcoders, AI video pipelines, or edge computing solutions.

## 📊 Performance Benchmarks

*Tested on Ubuntu 22.04, NVIDIA GTX 1080Ti, NVIDIA Driver 580, CUDA 13.0*

| Metric | Value |
|--------|-------|
| Resolution | 1920x1080 |
| Frame Rate | 30 FPS |
| Bitrate | 4 Mbps |
| CPU Usage | < 5% (Intel i7) |
| GPU Usage | ~15% (NVENC Engine) |
| End-to-End Latency | < 1 second (Local SRS Server) |
| Memory Leak | None (Tested over 24 hours) |

## 🛠️ System Requirements

- **OS**: Linux (Ubuntu 20.04 / 22.04 recommended)
- **GPU**: NVIDIA GPU with NVENC support (Pascal architecture or newer, e.g., GTX 1080Ti, RTX 3060+)
- **Driver**: NVIDIA Driver version 470+
- **CUDA Toolkit**: 11.0+ (for NVENC headers)
- **FFmpeg**: Version 4.0+ compiled with `--enable-nvenc` (The default Ubuntu `libavcodec-dev` package usually includes this)
- **Dependencies**: `build-essential`, `pkg-config`, `libavcodec-dev`, `libavformat-dev`, `libavutil-dev`

### Verify Your Environment
Before compiling, ensure your FFmpeg supports NVENC:
```bash
ffmpeg -encoders | grep nvenc
# Expected output: V....D h264_nvenc           NVIDIA NVENC H.264 encoder
