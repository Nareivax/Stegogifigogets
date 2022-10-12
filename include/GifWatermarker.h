#ifndef _GIF_WATERMARKER_H_
#define _GIF_WATERMARKER_H_

#include <string>
#include <gif_lib.h>


class GifWatermarker
{
private:
    std::string inputFileName;

    GifFileType* inputGif;

public:
    GifWatermarker(/* args */);
    ~GifWatermarker();

    GifFileType* loadGif(std::string fileName);

    int watermark();
};

#endif /*_GIF_WATERMARKER_H_*/