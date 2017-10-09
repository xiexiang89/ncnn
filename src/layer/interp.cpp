// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2017 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "interp.h"

namespace ncnn {

DEFINE_LAYER_CREATOR(Interp);

Interp::Interp()
{
    one_blob_only = true;
}

Interp::~Interp()
{
}

#if NCNN_STDIO
#if NCNN_STRING
int Interp::load_param(FILE *paramfp)
{
    int nscan = fscanf(paramfp, "%d %f %f %d %d", &resize_type, &height_scale, &width_scale, &output_height,
                       &output_width);
    if (nscan != 5)
    {
        fprintf(stderr, "Interp load_param failed %d\n", nscan);
        return -1;
    }

    return 0;
}
#endif // NCNN_STRING
int Interp::load_param_bin(FILE *paramfp)
{
    fread(&resize_type, sizeof(int), 1, paramfp);

    fread(&height_scale, sizeof(float), 1, paramfp);

    fread(&width_scale, sizeof(float), 1, paramfp);

    fread(&output_height, sizeof(int), 1, paramfp);

    fread(&output_width, sizeof(int), 1, paramfp);

    return 0;
}
#endif // NCNN_STDIO

int Interp::load_param(const unsigned char *&mem)
{
    resize_type = *(int *) (mem);
    mem += 4;

    height_scale = *(float *) (mem);
    mem += 4;

    width_scale = *(float *) (mem);
    mem += 4;

    output_height = *(int *) (mem);
    mem += 4;

    output_width = *(int *) (mem);
    mem += 4;

    return 0;
}

int Interp::forward(const Mat &bottom_blob, Mat &top_blob) const
{
    int h = bottom_blob.h;
    int w = bottom_blob.w;
    int c = bottom_blob.c;
    int oh = output_height;
    int ow = output_width;
    if (ow == 0 || ow == 0)
    {
        oh = h * height_scale;
        ow = w * width_scale;
    }
    if (oh == h && ow == w)
    {
        top_blob = bottom_blob;
        return 0;
    }
    top_blob.create(ow, oh, c);
    if (top_blob.empty())
        return -100;

    if (resize_type == 1)//nearest
    {
        #pragma omp parallel for
        for (int q = 0; q < c; ++q)
        {
            const float *ptr = bottom_blob.channel(q);
            float *output_ptr = top_blob.channel(q);
            for (int y = 0; y < oh; ++y)
            {
                const int in_y = std::min((int) (y / height_scale), (h - 1));
                for (int x = 0; x < ow; ++x)
                {
                    const int in_x = std::min((int) (x / width_scale), (w - 1));
                    output_ptr[ow * y + x] = ptr[in_y * w + in_x];
                }
            }
        }
        return 0;

        }
    else if (resize_type == 2)// bilinear
    {
        resize_bilinear(bottom_blob, top_blob, ow, oh);
        return 0;

    }
    else
    {
        fprintf(stderr, "unsupported resize type %d %d %d\n", resize_type, oh, ow);
        return -233;
    }
}


} // namespace ncnn
