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
    GifFileType* gifFile = DGifOpenFileName(fileName.c_str(), &_error);

    if (!gifFile) 
    {
        std::cout << "Failed opening gif with DGifOpenFileName: " << _error << std::endl;
        return nullptr;
    }
    if (DGifSlurp(gifFile) == GIF_ERROR) 
    {
        std::cout << "Failed reading the rest of the gif with DGifSlurp(): " << gifFile->Error << std::endl;
        DGifCloseFile(gifFile, &_error);
        return nullptr;
    }
    
    return gifFile;
}

GifFileType* GifWatermarker::loadEGif(std::string fileName)
{
    GifFileType* gifFile = EGifOpenFileName(fileName.c_str(), true, &_error);
    if (!gifFile) 
    {
        std::cout << "Failed opening gif with EGifOpenFileName: " << _error << std::endl;
        return nullptr;
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

void GifWatermarker::xorCrypt(GifFileType* inGif, std::string key)
{
    int numPixels = inGif->SWidth*inGif->SHeight;

    GifColorType black;
    black.Red = 0;
    black.Green = 0;
    black.Blue = 0;

    GifColorType white;
    white.Red = 255;
    white.Green = 255;
    white.Blue = 255;

    // Setting black and white manually to ensure they're set correctly
    inGif->SColorMap->Colors[0] = black;
    inGif->SColorMap->Colors[1] = white;

    float x[numPixels+1];
    // TODO: This is supposed to be between 3.57 and 4.00, I don't know why...
    float mew = 3.59;

    int initialVal = 0;

    // Generate Initial value based on the key
    for(size_t i = 0; i < key.size(); i++)
    {
        initialVal = initialVal ^ key[i];
    }
    // This helps keep values small, but is completely arbitrary and self imposed
    x[0] = (float)initialVal/1000.00;

    // Generate x values used for determining chaotic bits
    for(size_t i = 1; i < numPixels+1; i++)
    {
        x[i] = mew*x[i-1]*(1-x[i-1]);
    }

    // Let's generate some chaotic bits
    int chaoticBits[numPixels];

    // Generate a chaotic bit for each pixel in the watermark
    for (size_t i = 0; i < numPixels; i++)
    {
        int count = 1;
        float value;

        // Generate until the value is sufficiently big
        while(x[i+1]*pow(10, count) <= pow(10, 6)) //TODO: The 6 here should be made an adjustable value or constant
        {
            value = (x[i+1]*pow(10, count));
            count++;
        }
        // Take the LSB of the value as the chaotic bit
        chaoticBits[i] = (int)value & 1;
    }

    // For each image in the watermark
    for (size_t i = 0; i < inGif->ImageCount; i++)
    {
        // For each pixel in the image of the watermark
        for (size_t j = 0; j < numPixels; j++)
        {   
            // XOR Encrypt/Decrypt the pixel, this is modulo divided to get the LSB
            int value = (inGif->SavedImages[i].RasterBits[j] ^ chaoticBits[j])%2;

            // If the LSB is 1 set the color to white
            if(value)
            {
                inGif->SavedImages[i].RasterBits[j] = 1;
            }
            // If the LSB is 0 set the color to black
            else
            {
                inGif->SavedImages[i].RasterBits[j] = 0;
            }
        }
    }
}

void GifWatermarker::expandWatermark(int height, int width, GifFileType * watermark)
{
    // We can mess with the watermark RasterBits, since we don't save it
    for (size_t i = 0; i < watermark->ImageCount; i++)
    {
        // TODO: Check for memory leak, I'm not sure if gif_lib will clean it up when we reassign the pointers
        GifByteType * expanded = new unsigned char[height*width];

        for (size_t j = 0; j < height; j++)
        {
            for (size_t k = 0; k < width; k++)
            {
                expanded[j * width + k] = watermark->SavedImages[i].RasterBits[j%watermark->SHeight * watermark->SWidth + k%watermark->SWidth];
            }
        }
        watermark->SavedImages[i].RasterBits = expanded;
    }
    watermark->SWidth = width;
    watermark->SHeight = height;
}

int GifWatermarker::embed(std::string inputFile, std::string watermarkFile, std::string outputFile, std::string keyphrase)
{
    GifFileType * orig = loadDGif(inputFile);
    if(!orig)
    {
        return _error;
    }

    GifFileType * watermark = loadDGif(watermarkFile);
    if(!watermark)
    {
        return _error;
    }

    if(orig->SHeight > watermark->SHeight || orig->SWidth > watermark->SWidth)
    {
        expandWatermark(orig->SHeight, orig->SWidth, watermark);
    }
    xorCrypt(watermark, keyphrase);

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

    // For each image in the original GIF
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
                if(!(c % 2))
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
        EGifCloseFile(outGif, &_error);
        DGifCloseFile(orig, &_error);
        DGifCloseFile(watermark, &_error);
        return orig->Error;
    }
    return 0;
}

int GifWatermarker::extract(std::string inputFile, std::string outputFile, std::string keyphrase)
{
    GifFileType * input = loadDGif(inputFile);

    //TODO: This error check was causing us to fail (error 6) which seems incorrect
    // if(input->Error == GIF_ERROR)
    // {
    //     SPDLOG_ERROR("Error loading gif {}: {}", inputFile, _error);
    //     return _error;
    // }

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

                // If it's in the zeros map (key), then we want to set the color to black, otherwise it's white
                if(itr != partnerMaps.first->end())
                {
                    SPDLOG_DEBUG("Setting black pixel");
                    outGif->SavedImages[i].RasterBits[j * input->SWidth + k] = 1;
                }
                else
                {
                    SPDLOG_DEBUG("Setting white pixel");
                    outGif->SavedImages[i].RasterBits[j * input->SWidth + k] = 0;
                }
            }
        }
    }

    xorCrypt(outGif, keyphrase);

    if(EGifSpew(outGif) == GIF_ERROR)
    {
        std::cout << "Failed to spew Gif using EGifSpew(): " << input->Error << std::endl;
        EGifCloseFile(outGif, &_error);
        DGifCloseFile(input, &_error);
        return input->Error;
    }

    delete partnerMaps.first;
    delete partnerMaps.second;
    return 0;
};
