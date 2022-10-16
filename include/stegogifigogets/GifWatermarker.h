#ifndef _GIF_WATERMARKER_H_
#define _GIF_WATERMARKER_H_

#ifdef DEBUG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include <string>
#include <map>

#include "spdlog/spdlog.h"
#include "gif_lib.h"

class GifWatermarker
{

public:
    GifWatermarker();
    ~GifWatermarker();

    int embed(std::string inputFile, std::string watermarkFile, std::string outputFile);
    int extract(std::string inputFile, std::string outputFile);

private:
    GifFileType* loadDGif(std::string fileName);
    GifFileType* loadEGif(std::string fileName);
    std::pair<std::map<int, int>*, std::map<int, int>*> sortColorMap(ColorMapObject* inMap);

    int _error;
};

#endif /*_GIF_WATERMARKER_H_*/