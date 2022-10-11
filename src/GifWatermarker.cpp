#include <iostream>
#include <vector>
#include <math.h>

#include "GifWatermarker.h"

void sortColorMap(ColorMapObject* cmap);

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

int GifWatermarker::run()
{
    std::string inputFile = "/home/quagga/git/Stegogifigogets/example/pokemon-catch.gif";
    std::string watermarkFile = "/home/quagga/git/Stegogifigogets/example/4BlackPixels.gif";

    GifFileType * orig = loadGif(inputFile);
    GifFileType * watermark = loadGif(watermarkFile);

    int error;

    ColorMapObject * globalCM = orig->SColorMap;
    if(globalCM)
    {
        sortColorMap(globalCM);
    }

    // For each image in the GIF
    for(int i = 0; i < orig->ImageCount; ++i)
    {
        SavedImage currentImage = orig->SavedImages[i];

        if(currentImage.ImageDesc.ColorMap)
        {
            sortColorMap(currentImage.ImageDesc.ColorMap);
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
        return false;
    }

    EGifCloseFile(outGif, &error);
    DGifCloseFile(orig, &error);
    DGifCloseFile(watermark, &error);
    return 0;
};

void sortColorMap(ColorMapObject* cmap)
{
    // For each color until the second to last because they are being paired TODO: make sure it's an even colormap
    for(int i = 0; i < cmap->ColorCount-1; i+=2)
    {
        GifColorType current = cmap->Colors[i];
        float distance = 500; // Bigger than possible for 255, 255, 255 to 0, 0, 0

        // For each other color that isn't current and hasn't been paired yet TODO: what happens if last pair is very far
        for(int j = i+1; j < cmap->ColorCount; j++)
        {
            GifColorType temp = cmap->Colors[j];

            float tempDistance = sqrt(pow((current.Red - temp.Red), 2) + pow((current.Blue - temp.Blue), 2) + pow((current.Green - temp.Green),2));
            if(tempDistance < distance)
            {
                distance = tempDistance;
                cmap->Colors[i+1] = cmap->Colors[j];
            }
        }
    }

}

int main(int argc, char * argv[])
{
    GifWatermarker wm;
    return wm.run();
}