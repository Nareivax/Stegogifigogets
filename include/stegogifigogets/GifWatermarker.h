#ifndef _GIF_WATERMARKER_H_
#define _GIF_WATERMARKER_H_

#ifdef DEBUG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include <string>
#include <map>
#include <vector>

#include "spdlog/spdlog.h"
#include "gif_lib.h"

class GifWatermarker
{

public:
    GifWatermarker();
    ~GifWatermarker();

    int embed(std::string inputFile, std::string watermarkFile, std::string outputFile, std::string keyphrase="0");
    int extract(std::string inputFile, std::string outputFile, std::string keyphrase="0");

private:
    GifFileType* loadDGif(std::string fileName);
    GifFileType* loadEGif(std::string fileName);
    void sortColorMap(ColorMapObject * inMap);
    void xorCrypt(GifFileType* inGif, std::string key);
    void improveColorRedundancy(ColorMapObject * inMap);
    void expandWatermark(int height, int width, GifFileType * watermark);
    void findUsedBlack(GifFileType* inGif);

    std::pair<std::map<int, int>*, std::map<int, int>*> partnerMaps;
    std::vector<int> blackIndex;
    std::vector<std::pair<int, float>> distanceSorted;
    int numOfBlackPairs;

    int _error;
};

#endif /*_GIF_WATERMARKER_H_*/