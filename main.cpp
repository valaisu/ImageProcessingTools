#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <stdexcept>

/*

This program implements some image processing tools with CPU

*/

using Pixel = std::vector<unsigned char>;
using ImageVec = std::vector<Pixel>;

// Loads an image to essentially a 2D vector of chars with shape [pixels][3]
ImageVec loadImageToVector(const std::string &filename, int &width, int &height) {
    
    sf::Texture texture;
    if (!texture.loadFromFile(filename)) {
        throw std::runtime_error("Failed to load image from " + filename);
    }

    sf::Vector2u size = texture.getSize();
    width = size.x;
    height = size.y;

    int channels = 3;
    sf::Image image = texture.copyToImage();

    ImageVec imageVector(height*width, Pixel(channels));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            sf::Color color = image.getPixel(x, y);
            imageVector[y*width + x][0] = color.r;
            imageVector[y*width + x][1] = color.g;
            imageVector[y*width + x][2] = color.b;
        }
    }

    return imageVector;
}


void blur(ImageVec image, int imageWidth, int imageHeight, std::vector<float> kernel, int kenrelDim) {
    /*
    Params:
    image: vector of imageWidth*imageHeight Pixels
    kernel: precalculated 3*3 matrix
    
    For the edges, we are ok losing a bit of color
    meaning we treat the image as if it was padded with zeros
    */

    // lets do first something that works
    // 2D binomial blur can be applied separately for both dimensions
    
    // TODO:

    // calc 1D kernel values from pascals triangle
    // for dimensions
    //      create integral table
    //      calc and update blur

    

}


int main() {
    int width, height, channels = 3;
    ImageVec imageVector;

    try {
        imageVector = loadImageToVector("Mayhem.jpg", width, height);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::cout << "Dimensions: " << width << "x" << height << std::endl;

    // testing
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            std::cout << "(";
            for (int c = 0; c < channels; ++c) {
                std::cout << static_cast<int>(imageVector[y*width+x][c]);
                if (c < channels - 1) std::cout << ", ";
            }
            std::cout << ") ";
        }
        std::cout << std::endl;
    }

    float corner = 0.0625, edge = 0.125, mid = 0.25;
    std::vector<float> exampleKernel = {
        corner, edge, corner, 
        edge,   mid,  edge, 
        corner, edge, corner};

    blur(imageVector, width, height, exampleKernel, 3);

    return 0;
}
