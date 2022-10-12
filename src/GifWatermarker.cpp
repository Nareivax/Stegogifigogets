#include <iostream>
#include <map>
#include <math.h>

#include "GifWatermarker.h"

std::pair<std::map<int, int>*, std::map<int, int>*> sortColorMap(ColorMapObject* cmap);

GifWatermarker::GifWatermarker(/* args */)
{
}

GifWatermarker::~GifWatermarker()
{
}

GifFileType* GifWatermarker::loadGif(std::string fileName)
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

int GifWatermarker::watermark()
{
    std::string inputFile = "/home/quagga/git/Stegogifigogets/example/TestImage2x2.gif";
    std::string watermarkFile = "/home/quagga/git/Stegogifigogets/example/4BlackPixels.gif";

    GifFileType * orig = loadGif(inputFile);
    GifFileType * watermark = loadGif(watermarkFile);

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
                // The color map index of the current watermark pixel
                int c = watermark->SavedImages[0].RasterBits[j * watermark->SWidth + k];

                // If the color is black, we want a color key from the ones map
                if(waterCM->Colors[c].Red == 0 && waterCM->Colors[c].Green == 0 && waterCM->Colors[c].Blue == 0)
                {
                    std::cout << "SWAPPING PIXEL FROM Zero to One" << std::endl;
                    // Get the color of the cover image's pixel in the same location
                    int pixInt = currentImage.RasterBits[j * watermark->SWidth + k];

                    // Find this color key index in the zeros map
                    auto itr = partnerMaps.first->find(pixInt);

                    // If it's in the zeros map (key), then we want to set the color index of it's partner in the ones map (value)
                    if(itr != partnerMaps.first->end())
                    {
                        currentImage.RasterBits[j * watermark->SWidth + k] = itr->second;
                    }
                }
                // If the color isn't black (a.k.a white), we want a color key from the zeros map
                else
                {
                    std::cout << "SWAPPING PIXEL FROM One to Zero" << std::endl;
                    // Get the color of the cover image's pixel in the same location
                    int pixInt = currentImage.RasterBits[j * watermark->SWidth + k];

                    // Find this color key index in the ones map
                    auto itr = partnerMaps.second->find(pixInt);

                    // If it's in the ones map (key), then we want to set the color index of it's partner in the zeros map (value)
                    if(itr != partnerMaps.second->end())
                    {
                        currentImage.RasterBits[j * watermark->SWidth + k] = itr->second;
                    }
                }
            }
        }
    }

    GifFileType* outGif = EGifOpenFileName("/home/quagga/git/Stegogifigogets/example/Output.gif", false, &error);

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

    EGifCloseFile(outGif, &error);
    DGifCloseFile(orig, &error);
    DGifCloseFile(watermark, &error);
    return 0;
};

std::pair<std::map<int, int>*, std::map<int, int>*> sortColorMap(ColorMapObject* inMap)
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

int main(int argc, char * argv[])
{
    GifWatermarker wm;
    return wm.watermark();
}