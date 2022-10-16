#include <iostream>
#include <cmath>

#include "GifWatermarker.h"

GifWatermarker::GifWatermarker()
{
#ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
}

GifWatermarker::~GifWatermarker()
{
}

GifFileType* GifWatermarker::loadDGif(std::string fileName)
{
    int error;
    GifFileType* gifFile = DGifOpenFileName(fileName.c_str(), &error);

    if (!gifFile) 
    {
        std::cout << "Failed opening gif with DGifOpenFileName: " << error << std::endl;
        return NULL;
    }
    if (DGifSlurp(gifFile) == GIF_ERROR) 
    {
        std::cout << "Failed reading the rest of the gif with DGifSlurp(): " << gifFile->Error << std::endl;
        DGifCloseFile(gifFile, &error);
        return NULL;
    }
    
    return gifFile;
}

GifFileType* GifWatermarker::loadEGif(std::string fileName)
{
    int error;
    GifFileType* gifFile = EGifOpenFileName(fileName.c_str(), true, &error);
    if (!gifFile) 
    {
        std::cout << "Failed opening gif with EGifOpenFileName: " << error << std::endl;
        return NULL;
    }
    
    return gifFile;
}

std::pair<std::map<int, int>*, std::map<int, int>*> GifWatermarker::sortColorMap(ColorMapObject* inMap)
{

    ColorMapObject cmap = *inMap;
    
    std::pair<std::map<int, int>*, std::map<int, int>*> partnerMaps;
    std::map<int, int>* zerosMap = new std::map<int, int>;
    std::map<int, int>* onesMap = new std::map<int, int>;

    // For each color until the second to last because they are being paired TODO: make sure it's an even colormap
    for(int i = 0; i < cmap.ColorCount-1; i++)
    {
        GifColorType current = cmap.Colors[i];
        float distance = 500; // Bigger than possible for 255, 255, 255 to 0, 0, 0

        if(zerosMap->find(i) == zerosMap->end() && onesMap->find(i) == onesMap->end())
        {
            // For each other color that isn't current and hasn't been paired yet TODO: what happens if last pair is very far
            for(int j = i+1; j < cmap.ColorCount; j++)
            {
                if(zerosMap->find(j) == zerosMap->end() && onesMap->find(j) == onesMap->end())
                {
                    GifColorType nextColor = cmap.Colors[j]; // The next color being checked
                    
                    float tempDistance = sqrt(pow((current.Red - nextColor.Red), 2) + pow((current.Blue - nextColor.Blue), 2) + pow((current.Green - nextColor.Green), 2));
                    if(tempDistance < distance)
                    {
                        distance = tempDistance;
                        (*zerosMap)[i] = j;
                    }
                }
            }
            (*onesMap)[zerosMap->find(i)->second] = i;
        }
    }
    partnerMaps.first = zerosMap;
    partnerMaps.second = onesMap;
    return partnerMaps;
}

int GifWatermarker::embed(std::string inputFile, std::string watermarkFile, std::string outputFile)
{
    GifFileType * orig = loadDGif(inputFile);
    GifFileType * watermark = loadDGif(watermarkFile);

    int error;
    std::pair<std::map<int, int>*, std::map<int, int>*> partnerMaps;

    ColorMapObject * waterCM = watermark->SColorMap;
    if(watermark->SavedImages[0].ImageDesc.ColorMap)
    {
        waterCM = watermark->SavedImages[0].ImageDesc.ColorMap;
    }

    ColorMapObject * globalCM = orig->SColorMap;
    if(globalCM)
    {
        partnerMaps = sortColorMap(globalCM);
    }

    // For each image in the GIF
    for(int i = 0; i < orig->ImageCount; ++i)
    {
        SavedImage currentImage = orig->SavedImages[i];
        ColorMapObject * currentCM = globalCM;

        if(currentImage.ImageDesc.ColorMap)
        {
            currentCM = currentImage.ImageDesc.ColorMap;
            partnerMaps = sortColorMap(currentImage.ImageDesc.ColorMap);
        }

        // For each pixel of height in the watermark
        for(int j = 0; j < watermark->SHeight; j++)
        {
            // For each pixel of width in the watermark
            for(int k = 0; k < watermark->SWidth; k++)
            {
                SPDLOG_DEBUG("image: {}\trow: {}\tcol: {}", i, j, k);
                // The color map index of the current watermark pixel
                int c = watermark->SavedImages[0].RasterBits[j * watermark->SWidth + k];

                // If the color is black, we want a color key from the ones map (color value of the zeros map)
                if(waterCM->Colors[c].Red == 0 && waterCM->Colors[c].Green == 0 && waterCM->Colors[c].Blue == 0)
                {
                    // Get the color of the cover image's pixel in the same location
                    int pixInt = currentImage.RasterBits[j * orig->SWidth + k];

                    // Find this color key index in the zeros map
                    auto itr = partnerMaps.first->find(pixInt);

                    // If it's in the zeros map (key), then we want to set the color index of it's partner in the ones map (value)
                    if(itr != partnerMaps.first->end())
                    {
                        SPDLOG_DEBUG("SWAPPING PIXEL FROM Zero to One");
                        currentImage.RasterBits[j * orig->SWidth + k] = itr->second;
                    }
                }
                // If the color isn't black (a.k.a white), we want a color key from the zeros map (color value of the ones map)
                else
                {
                    // Get the color of the cover image's pixel in the same location
                    int pixInt = currentImage.RasterBits[j * orig->SWidth + k];

                    // Find this color key index in the ones map
                    auto itr = partnerMaps.second->find(pixInt);

                    // If it's in the ones map (key), then we want to set the color index of it's partner in the zeros map (value)
                    if(itr != partnerMaps.second->end())
                    {
                        SPDLOG_DEBUG("SWAPPING PIXEL FROM One to Zero");
                        currentImage.RasterBits[j * orig->SWidth + k] = itr->second;
                    }
                }
            }
        }
    }

    GifFileType* outGif = loadEGif(outputFile);

    outGif->SavedImages = orig->SavedImages;
    outGif->SColorMap = orig->SColorMap;
    outGif->SHeight = orig->SHeight;
    outGif->SWidth = orig->SWidth;
    outGif->Image = orig->Image;
    outGif->AspectByte = orig->AspectByte;
    outGif->SBackGroundColor = orig->SBackGroundColor;
    outGif->ImageCount = orig->ImageCount;
    outGif->ExtensionBlockCount = orig->ExtensionBlockCount;
    outGif->ExtensionBlocks = orig->ExtensionBlocks;
    outGif->SColorResolution = orig->SColorResolution;

    if(EGifSpew(outGif) == GIF_ERROR)
    {
        std::cout << "Failed to spew Gif using EGifSpew(): " << orig->Error << std::endl;
        EGifCloseFile(outGif, &error);
        DGifCloseFile(orig, &error);
        DGifCloseFile(watermark, &error);
        return orig->Error;
    }
    return 0;
}

int GifWatermarker::extract(std::string inputFile, std::string outputFile)
{
    GifFileType * input = loadDGif(inputFile);

    int error;
    std::pair<std::map<int, int>*, std::map<int, int>*> partnerMaps;

    ColorMapObject * globalCM = input->SColorMap;
    if(globalCM)
    {
        partnerMaps = sortColorMap(globalCM);
    }

    GifFileType* outGif = loadEGif(outputFile);
    
    // Defining the colors for the extracted image
    GifColorType white, black;
    white.Red = 255;
    white.Green = 255;
    white.Blue = 255;
    black.Red = 0;
    black.Green = 0;
    black.Blue = 0;

    GifColorType colors[2];
    GifColorType* c = colors;
    c[0] = white;
    c[1] = black;

    outGif->SHeight = input->SHeight;
    outGif->SWidth = input->SWidth;
    outGif->SBackGroundColor = 0;
    outGif->SColorMap = GifMakeMapObject(2, colors);

    SavedImage gifImage[input->ImageCount];
    for (size_t i = 0; i < input->ImageCount; i++)
    {
        gifImage[i].ImageDesc.Left = 0;
        gifImage[i].ImageDesc.Top = 0;
        gifImage[i].ImageDesc.Width = outGif->SWidth;
        gifImage[i].ImageDesc.Height = outGif->SHeight;
        gifImage[i].ImageDesc.Interlace = false;
        gifImage[i].ImageDesc.ColorMap = nullptr;
        gifImage[i].RasterBits = (GifByteType*)malloc(outGif->SWidth*outGif->SHeight);
        gifImage[i].ExtensionBlockCount = 0;
        gifImage[i].ExtensionBlocks = nullptr;
        GifMakeSavedImage(outGif, &gifImage[i]);
    }

    // For each image in the input GIF
    for(int i = 0; i < input->ImageCount; ++i)
    {
        SavedImage currentImage = input->SavedImages[i];
        ColorMapObject * currentCM = globalCM;

        // If there is a local color map, sort that one
        if(currentImage.ImageDesc.ColorMap)
        {
            currentCM = currentImage.ImageDesc.ColorMap;
            partnerMaps = sortColorMap(currentImage.ImageDesc.ColorMap);
        } 

        // For each pixel of height in the input gif
        for(int j = 0; j < input->SHeight; j++)
        {
            // For each pixel of width in the input gif
            for(int k = 0; k < input->SWidth; k++)
            {
                SPDLOG_DEBUG("image: {}\trow: {}\tcol: {}", i, j, k);
                // The color map index of the current input gif pixel
                int c = input->SavedImages[i].RasterBits[j * input->SWidth + k];

                // Find this color key index in the zeros map
                auto itr = partnerMaps.first->find(c);

                // If it's in the zeros map (key), then we want to set the color to white, otherwise it's black
                if(itr != partnerMaps.first->end())
                {
                    SPDLOG_DEBUG("Setting white pixel");
                    outGif->SavedImages[i].RasterBits[j * input->SWidth + k] = 0;
                }
                else
                {
                    SPDLOG_DEBUG("Setting black pixel");
                    outGif->SavedImages[i].RasterBits[j * input->SWidth + k] = 1;
                }
            }
        }
    }

    if(EGifSpew(outGif) == GIF_ERROR)
    {
        std::cout << "Failed to spew Gif using EGifSpew(): " << input->Error << std::endl;
        EGifCloseFile(outGif, &error);
        DGifCloseFile(input, &error);
        return input->Error;
    }

    delete partnerMaps.first;
    delete partnerMaps.second;
    return 0;
};
