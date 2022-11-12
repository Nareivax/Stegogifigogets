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

struct CompareGreaterColor
{
    bool operator()(GifColorType a, GifColorType b)
    {
        return ((((int)a.Red & 0xff) << 16) + (((int)a.Green & 0xff) << 8) + ((int)a.Blue & 0xff)) < \
                ((((int)b.Red & 0xff) << 16) + (((int)b.Green & 0xff) << 8) + ((int)b.Blue & 0xff));
    }
};

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
    void findDuplicateColors(SavedImage* inImage);
    void findDuplicateColors(GifFileType* inGif);

    std::pair<std::map<int, int>*, std::map<int, int>*> partnerMaps;

    std::map<int, int>* globalZeroes;
    std::map<int, int>* globalOnes;
    std::map<int, int>* localZeroes;
    std::map<int, int>* localOnes;

    std::map<GifColorType, std::vector<int>, CompareGreaterColor> duplicateColors;
    std::vector<std::pair<int, float>> distanceSorted;

    GifColorType replaceable;

    int _error;
};

#endif /*_GIF_WATERMARKER_H_*/
