#include <iostream>
#include <cstring>

#include "GifWatermarker.h"

enum Operation
{
    EMBED = 0,
    EXTRACT = 1
};

enum Error_List
{
    OK = 0,
    BAD_ARGS = 203
};

void printHelp()
{
    std::cout << "Stegogifigogets Help:\
                    \n\tUsage: stegogifigogets [-e <path>]  [-x <path>] [-h]\
                    \n\t\t\t\t[-w <path>] [-o <path>]\
                    \n\tOptions:\
                    \n\t\t--help,      -h:\tPresents this printout\
                    \n\t\t--embed,     -e:\tFilepath for the input gif that will have a watermark embedded\
                    \n\t\t--extract,   -x:\tFilepath for the input gif to extract from\
                    \n\t\t--watermark, -w:\tFilepath for the watermark that is to be embedded\
                    \n\t\t--output,    -o:\tFilepath for the watermarked gif or extracted watermark" << std::endl;
}

int main(int argc, char * argv[])
{
    std::string gifFilePath;
    std::string wmFilePath;
    std::string outputPath;
    Operation op;

    // Parse the command line arguments
    if(argc == 1)
    {
        printHelp();
        return BAD_ARGS;
    }
    for (size_t i = 1; i < argc; i++)
    {
        if(!std::strcmp(argv[i], "--help") || !std::strcmp(argv[i], "-h"))
        {
            printHelp();
            return OK;
        }
        else if (!std::strcmp(argv[i], "--embed") || !std::strcmp(argv[i], "-e"))
        {
            op = EMBED;
            if(i+1 < argc)
            {
                gifFilePath = argv[i+1];
            }
            else
            {
                std::cout << "Please specify an input file path after --embed." << std::endl;
                return BAD_ARGS;
            }
            i++;
        }
        else if (!std::strcmp(argv[i], "--extract") || !std::strcmp(argv[i], "-x"))
        {
            op = EXTRACT;
            if(i+1 < argc)
            {
                gifFilePath = argv[i+1];
            }
            else
            {
                std::cout << "Please specify an input file path after --extract." << std::endl;
                return BAD_ARGS;
            }
            i++;
        }
        else if (!std::strcmp(argv[i], "--watermark") || !std::strcmp(argv[i], "-w"))
        {
            if(i+1 < argc)
            {
                wmFilePath = argv[i+1];
            }
            else
            {
                std::cout << "Please specify an watermark file path after --watermark." << std::endl;
                return BAD_ARGS;
            }
            i++;
        }
        else if (!std::strcmp(argv[i], "--output") || !std::strcmp(argv[i], "-o"))
        {
            if(i+1 < argc)
            {
                outputPath = argv[i+1];
            }
            else
            {
                std::cout << "Please specify an output file path after --output." << std::endl;
                return BAD_ARGS;
            }
            i++;
        }
        else
        {
            std::cout << "Incorrect argument, \"" << argv[i] << "\" see help below:" << std::endl;
            printHelp();
            return BAD_ARGS;
        }
    }

    // Object for embedding or extracting images
    GifWatermarker water;

    if(op == EMBED)
    {
        if(wmFilePath.empty())
        {
            std::cout << "Must supply a watermark file path using \'--watermark\'" << std::endl;
            return BAD_ARGS;
        }
        if(outputPath.empty())
        {
            size_t pos = 0;
            outputPath = gifFilePath;
            pos = gifFilePath.find(".gif");
            outputPath = outputPath.insert(pos, "_MARKED");
        }

        return water.embed(gifFilePath, wmFilePath, outputPath);
    }
    else
    {
        if(outputPath.empty())
        {
            size_t pos = 0;
            outputPath = gifFilePath;
            pos = gifFilePath.find(".gif");
            outputPath = outputPath.insert(pos, "_EXTRACT");
        }
        return water.extract(gifFilePath, outputPath);
    }

}