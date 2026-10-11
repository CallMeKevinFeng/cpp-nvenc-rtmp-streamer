extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

int main() {

    //av_log_set_level(AV_LOG_DEBUG); // 【新增】开启 FFmpeg 底层调试日志
    // 1. 查找 NVENC 编码器
    const AVCodec* codec = avcodec_find_encoder_by_name("h264_nvenc");
    if (!codec) {
        printf("找不到 h264_nvenc 编码器，请检查环境！\n");
        return -1;
    }

    // 2. 初始化编码器上下文
    AVCodecContext* c = avcodec_alloc_context3(codec);
    c->bit_rate = 4000000; // 4 Mbps
    c->width = 1920;
    c->height = 1080;
    c->time_base = (AVRational){1, 30}; // 30 fps
    c->framerate = (AVRational){30, 1};
    c->gop_size = 30;
    c->max_b_frames = 0; // 1080Ti 的 H.264 NVENC 支持 B 帧，但先设为 0 简化测试
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // 【新增】RTMP/FLV 必须开启全局头

    // 3. 打开编码器
    if (avcodec_open2(c, codec, NULL) < 0) {
        printf("无法打开编码器\n");
        return -1;
    }

    // 4. 创建输出文件上下文（这里写死输出为 mp4 文件，后续换成 rtmp:// 就是推流器）
    AVFormatContext* fmt_ctx = NULL;
    avformat_alloc_output_context2(&fmt_ctx, NULL, "flv", "rtmp://127.0.0.1:1935/live/test");
    AVStream* stream = avformat_new_stream(fmt_ctx, NULL);
    stream->time_base = (AVRational){1, 1000};// 【修改】FLV 标准时间基是毫秒
    avcodec_parameters_from_context(stream->codecpar, c);

    if (avio_open(&fmt_ctx->pb, "rtmp://127.0.0.1:1935/live/test", AVIO_FLAG_WRITE) < 0) {
        printf("无法打开网络流\n");
        return -1;
    }
    avformat_write_header(fmt_ctx, NULL);

    // 5. 准备一帧图像数据
    AVFrame* frame = av_frame_alloc();
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;
    av_frame_get_buffer(frame, 0);

    AVPacket* pkt = av_packet_alloc();
    int frame_count = 0;

    // 6. 编码循环（这里模拟生成 100 帧）
    int i = 0;
    while (1) {
        av_frame_make_writable(frame);
        for (int y = 0; y < c->height; y++) {
            for (int x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3; // 画面会不断变化
            }
        }
        frame->pts = i;
        i++;

        // 发送给编码器
        if (avcodec_send_frame(c, frame) < 0) break;

        // 接收编码后的包
        while (avcodec_receive_packet(c, pkt) == 0) {
            pkt->stream_index = stream->index;
            av_packet_rescale_ts(pkt, c->time_base, stream->time_base);
            int ret = av_interleaved_write_frame(fmt_ctx, pkt); // 【修改】捕获返回值
            if (ret < 0) {
                printf("推流失败，错误码: %d\n", ret);
            }
            av_packet_unref(pkt);
            frame_count++;
        }
        // 加一个小延时，控制帧率接近 30fps（防止 CPU 跑满）
        av_usleep(33000); 
    }

    // 7. 冲刷编码器
    avcodec_send_frame(c, NULL);
    while (avcodec_receive_packet(c, pkt) == 0) {
        pkt->stream_index = stream->index;
        av_packet_rescale_ts(pkt, c->time_base, stream->time_base);
        int ret = av_interleaved_write_frame(fmt_ctx, pkt); // 【修改】捕获返回值
        if (ret < 0) {
            printf("推流失败，错误码: %d\n", ret);
        }
        av_packet_unref(pkt);
        frame_count++;
    }

    av_write_trailer(fmt_ctx);
    printf("编码完成！共编码 %d 帧。\n", frame_count);

    // 清理资源
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&c);
    avio_closep(&fmt_ctx->pb);
    avformat_free_context(fmt_ctx);

    return 0;
}
