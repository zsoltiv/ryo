/*
 * copyright (c) 2024 Zsolt Vadasz
 *
 * This file is part of ryo.
 *
 * ryo is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * ryo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ryo. If not, see <https://www.gnu.org/licenses/>. 
*/

#include <getopt.h>
#include <libavformat/avio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

struct output {
    AVFormatContext *avctx;
    struct output *next;
    bool initialized;
};

static bool input_present = false;
static AVFormatContext *inctx = NULL;
static AVInputFormat *infmt = NULL;
static AVPacket *pkt;

static inline const AVOutputFormat *find_output_format(const AVInputFormat *infmt)
{
    // TODO *_pipe `AVInputFormat`s need handling
    // for now, just default to mjpeg since this
    // program is only used for kokanyctl anyway
    if(!infmt) return NULL;
    const AVOutputFormat *outfmt = av_guess_format("mjpeg",
                                                   NULL,
                                                   infmt->mime_type);
    return outfmt;
}

static AVFormatContext *open_outctx(const char *ffout)
{
    AVFormatContext *avctx;
    const AVOutputFormat *fmt = find_output_format(inctx->iformat);
    if(!fmt) {
        fprintf(stderr, "Failed to find output format for %s\n", inctx->iformat->mime_type);
        return NULL;
    }
    int ret;
    if((ret = avformat_alloc_output_context2(&avctx, fmt, NULL, ffout)) < 0)
        fprintf(stderr, "avformat_alloc_output_context2(): %s\n", av_err2str(ret));
    return avctx;
}

static struct output *output_new(struct output *prev, const char *ffout)
{
    struct output *out = calloc(1, sizeof(struct output));
    if(prev)
        prev->next = out;
    printf("Output %s\n", ffout);
    out->next = NULL;
    out->avctx = open_outctx(ffout);
    out->initialized = false;
    return out;
}

static int output_copy_streams(struct output *out)
{
    int ret;
    for(int i = 0; i < inctx->nb_streams; i++) {
        AVStream *instream = inctx->streams[i];
        AVStream *outstream = avformat_new_stream(out->avctx, NULL);
        if((ret = avcodec_parameters_copy(outstream->codecpar, instream->codecpar)) < 0) {
            fprintf(stderr, "avcodec_parameters_copy(): %s\n", av_err2str(ret));
            return ret;
        }
        outstream->codecpar->codec_tag = 0;
        printf("Added new stream (%s) to output %s\n",
               avcodec_get_name(outstream->codecpar->codec_id),
               out->avctx->url);
    }
    return 0;
}

static int output_open(struct output *out)
{
    int ret = 0;
    AVDictionary *out_opts = NULL;
    if((ret = av_dict_set(&out_opts,
                          "listen",
                          "1",
                          0)) < 0) {
        fprintf(stderr, "av_dict_set(): %s\n", av_err2str(ret));
        return ret;
    }
    printf("URL %s\n", out->avctx->url);
    if(!(out->avctx->flags & AVFMT_NOFILE) &&
       (ret = avio_open2(&out->avctx->pb, out->avctx->url, AVIO_FLAG_WRITE, NULL, &out_opts)) < 0)
        fprintf(stderr, "avio_open(): %s\n", av_err2str(ret));
    return ret;
}

static int output_init_ctx(struct output *out)
{
    int ret;
    if((ret = output_copy_streams(out)) < 0)
        return ret;
    if((ret = output_open(out)) < 0)
        return ret;
    if((ret = avformat_write_header(out->avctx, NULL)) < 0) {
        fprintf(stderr, "avformat_write_header(): %s\n", av_err2str(ret));
        return ret;
    }
    out->initialized = true;

    return 0;
}

int main(int argc, char **argv)
{
    int opt, ret;
    struct output *outputs = NULL, *current = NULL;
    pkt = av_packet_alloc();
    while((opt = getopt(argc, argv, "i:o:")) != -1) {
        switch(opt) {
            case 'i':
                if(input_present) {
                    fprintf(stderr, "Only one input is allowed\n");
                    return 1;
                }
                input_present = true;
                inctx = avformat_alloc_context();
                printf("Input: %s\n", optarg);
                if((ret = avformat_open_input(&inctx, optarg, NULL, NULL)) < 0) {
                    fprintf(stderr, "avformat_open_input(): %s", av_err2str(ret));
                    return 1;
                }
                if((ret = avformat_find_stream_info(inctx, NULL)) < 0) {
                    fprintf(stderr, "avformat_find_stream_info(): %s", av_err2str(ret));
                    return 1;
                }
                break;
            case 'o':
                if(!input_present) {
                    fprintf(stderr, "-i must be the first option\n");
                    return 1;
                }

                if(!outputs) {
                    outputs = output_new(NULL, optarg);
                    if(!outputs->avctx) {
                        fprintf(stderr, "Failed to create context for output %s\n", optarg);
                        return 1;
                    }
                    current = outputs;
                } else {
                    current = output_new(current, optarg);
                    if(!outputs->avctx) {
                        fprintf(stderr, "Failed to create context for output %s\n", optarg);
                        return 1;
                    }
                }
                break;
            case '?':
                return 1;
                break;
            case ':':
                return 1;
                break;
        }
    }
    printf("Options processed\n");

    while(1) {
        if((ret = av_read_frame(inctx, pkt)) < 0) {
            fprintf(stderr, "av_read_frame(): %s\n", av_err2str(ret));
            if(ret == AVERROR_EOF)
                break;
            continue;
        }
        AVStream *in_stream = inctx->streams[pkt->stream_index];
        printf("out %p next %p\n", outputs, outputs->next);
        struct output *out = outputs;
        while(out) {
            printf("out %p\n", out);
            if(!out->initialized && (ret = output_init_ctx(out)) < 0)
                return 1;
            printf("remuxing\n");
            // XXX the output stream structure matches that of `inctx`
            AVStream *out_stream = out->avctx->streams[pkt->stream_index];
            AVPacket *out_pkt = av_packet_clone(pkt);
            av_packet_rescale_ts(out_pkt, in_stream->time_base, out_stream->time_base);
            out_pkt->pos = -1;
            if((ret = av_interleaved_write_frame(out->avctx, out_pkt)) < 0) {
                // TODO Handle closed socket
                fprintf(stderr, "av_interleaved_write_frame(): %s\n", av_err2str(ret));
                if(ret == AVERROR(EPIPE)) {
                    avio_closep(&out->avctx->pb);
                    if(output_open(out) < 0)
                        continue;
                }
            }

            out = out->next;
        }
    }
}
