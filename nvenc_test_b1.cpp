extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

#include <cstdio>
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 往音频帧里填一段正弦波
static void fill_sine(AVFrame* aframe, int64_t start_sample, int sample_rate, double freq)
{
	for(int ch = 0; ch < aframe->ch_layout.nb_channels; ch++){
		float* data = reinterpret_cast<float*>(aframe->data[ch]);
		for (int i = 0; i < aframe->nb_samples; i++) {
			double t = static_cast<double>(start_sample + i) / sample_rate;
			data[i] = static_cast<float>(0.3 * sin(2.0 * M_PI * freq *t));
		}
	}

}

int main() {
    av_log_set_level(AV_LOG_DEBUG);

    // 1. 查找 NVENC 编码器
    // ================== 视频编码器 ================================
    const AVCodec* vcodec = avcodec_find_encoder_by_name("h264_nvenc");
    if (!vcodec) {
        printf("找不到 h264_nvenc 编码器，请检查环境！\n");
        return -1;
    }

    // 2. 初始化编码器上下文
    AVCodecContext* vc = avcodec_alloc_context3(vcodec);
    if (!c) {
        printf("avcodec_alloc_context3 失败\n");
        return -1;
    }

    vc->bit_rate = 4000000; // 4 Mbps
    vc->width = 1920;
    vc->height = 1080;
    vc->time_base = (AVRational){1, 30};
    vc->framerate = (AVRational){30, 1};
    vc->gop_size = 30;
    vc->max_b_frames = 0;
    vc->pix_fmt = AV_PIX_FMT_YUV420P;

    // 3. 打开编码器
    if (avcodec_open2(vc, vcodec, NULL) < 0) {
        printf("无法打开视频编码器\n");
        return -1;
    }

    // ============== 音频编码器 ===========================
    const AVCode* acodec = avcodec_fine_encoder(AV_CODEC_ID_AAC);
    if(!acodec){
         printf("找不到 AAC 编码器\n");
         return -1;
    }

    AVCodecContext* ac = avcodec_alloc_context3(acodec);
    ac->bit_rate = 128000;
    ac->sample_rate = 48000;
    ac->sample_fmt = acodec->sample_fmts ? acodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    av_channel_layout_default(&ac->ch_layout, 2); // 立体声
    ac->time_base = (AVRational){1, ac->sample_rate};

    if (avcode_open2(ac, acodec, NULL) < 0) {
         printf("无法打开音频编码器\n");
        return -1;
    }

    // 4. 创建输出文件上下文
    AVFormatContext* fmt_ctx = NULL;
    const char* out_filename = "nvenc_output.mp4";

    if (avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, out_filename) < 0 || !fmt_ctx) {
        printf("avformat_alloc_output_context2 失败\n");
        return -1;
    }

    // 视频流
    AVStream* vstream = avformat_new_stream(fmt_ctx, NULL);
    vstream->time_base = vc->time_base;
    avcodec_parameters_from_context(vstream->codecpar, vc);

    // 音频流
    AVStream* astream = avformat_new_stream(fmt_ctx, NULL);
    asteam->time_base = ac->time_base;
    avcodec_parameters_from_context(astream->codecpar, ac);

    if (avio_open(&fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
        printf("无法打开输出文件\n");
        return -1;
    }

    if (avformat_write_header(fmt_ctx, NULL) < 0) {
        printf("avformat_write_header 失败\n");
        return -1;
    }

    // =============== 视频帧准备 =================
    // 5. 准备一帧图像数据
    AVFrame* vframe = av_frame_alloc();
    vframe->format = vc->pix_fmt;
    vframe->width = vc->width;
    vframe->height = vc->height;
    av_frame_get_buffer(vframe, 0);

    // =============== 音频帧准备 ======================
    AVFrame* aframe = av_frame_alloc();
    aframe->format = ac->sample_fmt;
    av_channel_layout_copy(&aframe->ch_layout, &ac->ch_layout);
    aframe->sample_rate = ac->sample_rate;
    aframe->nb_samples = ac->frame_size > 0 ? ac->frame_size : 1024;
    av_frame_get_buffer(aframe, 0);

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        printf("av_packet_alloc 失败\n");
        return -1;
    }

    int frame_count = 0;

    // 6. 编码循环，模拟 100 帧
    for (int i = 0; i < 100; i++) {
        if (av_frame_make_writable(frame) < 0) {
            printf("av_frame_make_writable 失败\n");
            break;
        }

        // 填充 Y 平面
        for (int y = 0; y < c->height; y++) {
            for (int x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }

        // 填充 U/V 平面为 128，避免颜色异常
        for (int y = 0; y < c->height / 2; y++) {
            memset(frame->data[1] + y * frame->linesize[1], 128, c->width / 2);
            memset(frame->data[2] + y * frame->linesize[2], 128, c->width / 2);
        }

        frame->pts = i;

        if (avcodec_send_frame(c, frame) < 0) {
            printf("avcodec_send_frame 失败\n");
            break;
        }

        while (avcodec_receive_packet(c, pkt) == 0) {
            pkt->stream_index = stream->index;
            av_packet_rescale_ts(pkt, c->time_base, stream->time_base);
            av_interleaved_write_frame(fmt_ctx, pkt);
            av_packet_unref(pkt);
            frame_count++;
        }
    }

    // 7. 冲刷编码器
    avcodec_send_frame(c, NULL);
    while (avcodec_receive_packet(c, pkt) == 0) {
        pkt->stream_index = stream->index;
        av_packet_rescale_ts(pkt, c->time_base, stream->time_base);
        av_interleaved_write_frame(fmt_ctx, pkt);
        av_packet_unref(pkt);
        frame_count++;
    }

    av_write_trailer(fmt_ctx);
    printf("编码完成！共编码 %d 帧。\n", frame_count);

    // 清理资源
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&c);
    if (fmt_ctx->pb) {
        avio_closep(&fmt_ctx->pb);
    }
    avformat_free_context(fmt_ctx);

    return 0;
}
