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
#include <stdlib.h>
#include <stdbool.h>

#include <libavformat/avformat.h>

struct output {
    AVFormatContext *avctx;
    struct output *next;
};

static bool input_present = false;
static AVFormatContext *inctx = NULL;

AVFormatContext *open_outctx(const char *ffout)
{
    AVFormatContext *avctx = avformat_alloc_context();
    return avctx;
}

struct output *output_new(struct output *prev, const char *ffout)
{
    struct output *out = calloc(1, sizeof(struct output));
    if(prev)
        prev->next = out;
    out->next = NULL;
    out->avctx = open_outctx(ffout);
    return out;
}

void input_init(const char *ffinput)
{
    inctx = avformat_alloc_context();
}

int main(int argc, char **argv)
{
    int opt;
    while((opt = getopt(argc, argv, "i:o:")) != -1) {
        switch(opt) {
            case 'i':
                if(input_present) {
                    fprintf(stderr, "Only one input is allowed\n");
                    return 1;
                }
                input_present = true;
                break;
            case 'o':
                if(!input_present) {
                    fprintf(stderr, "-i must be the first option\n");
                    return 1;
                }
                break;
            case '?':
                break;
            case ':':
                break;
        }
    }
}
