#include <SFML/Graphics.hpp>
#include <iostream>
#include <numeric>
#include <vector>
#include <stdexcept>
#include <functional>
#include <map>
#include <chrono>
#include <omp.h>


uint64_t timeNow() {
   using namespace std::chrono;
   return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
} 

/*

This program implements some image processing tools with CPU
compile: 
g++ -fopenmp -o display main.cpp -lsfml-graphics -lsfml-window -lsfml-system

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


sf::Image vectorToImage(const ImageVec& imageVec, int width, int height) {
    sf::Image image;
    image.create(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Pixel& pixel = imageVec[y * width + x];
            sf::Color color(pixel[0], pixel[1], pixel[2]);
            image.setPixel(x, y, color);
        }
    }
    return image;
}


unsigned long long factorial(const int n) {
    unsigned long long res = 1;
    for (int i = 1; i <= n; ++i) {
        res *= i;
    }
    return res;
}


unsigned long long binomialCoefficient(const int n, int k) {
    if (2*k > n) {
        k = n - k; 
    }
    unsigned long long result = 1;
    for (int i = 0; i < k; ++i) {
        result *= (n - i);
        result /= (i + 1);
    }

    return result;
}

// Note: Don't use for calculating values bigger than 2147483647
int power(const int a, const int b) {
    int res = 1;
    for (int i = 0; i < b; i++) {
        res = res*a;
    }
    return res;
}


// Applies binomial blur for ImageVec
void blur(ImageVec* image, const int widthImage, const int heightImage, const int kernelSize) {
    /*
    Params:
    image: vector of widthImage*heightImage Pixels
    kernelSize: The distance from edge of kernel to the center of the kernel. For 3x3 kernel this would be 1, 5x5-> 2, 9x9->4 ... 
    
    For the edges, we are ok losing a bit of color
    meaning we treat the image as if it was padded with zeros
    */

    // lets do first something that works
    // 2D binomial blur can be applied separately for both dimensions

    /* 
    TODO:
        ✔Implement naive blur
        ✔Separate dimensions
        Optimize algorithm
            ✔multithread
            vectorization
            ILP
        Algo for big kernels (size>8)
    */
    

    if (kernelSize > 8) {
        printf("The kenrel is too big, causes integer overflow\n");
        // The solution to this problem is maths: 
        // Applying blur once with kernel width 21 is the same as applying blur twice with
        // kernel width 11, as the convolution of pascal's triangles row 11 with it self results
        // in pascals triangle row 21
        // The inevitable side effect ofcourse is a small reduction in accuracy
        // TODO: fix accordingly

        return;
    }


    int pascalRowSum = power(2, kernelSize*2); // 2^(kernelSize*2) 
    int pascalRowNumber = kernelSize*2; // the first row has number or index "0"

    std::vector<int> pascalRow(pascalRowNumber+1);
    for (int i = 0; i <= pascalRowNumber; i++) {
        pascalRow[i] = int(binomialCoefficient(pascalRowNumber, i));
    }

    //printf("Corner before: \n");
    //printf("%d %d %d \n", int((*image)[0][0]), int((*image)[0][1]), int((*image)[0][2]));

    // horizontal blur moves data in here, and vertical blur moves it back out
    std::vector<int> buffer(widthImage * heightImage);    

    for (int color = 0; color < 3; color++) {
        // clean buffer
        std::fill(buffer.begin(), buffer.end(), 0);

        // horizontal blur
        #pragma omp parallel for schedule(dynamic)
        for (size_t y = 0; y < heightImage; y++) {
            for (size_t x = 0; x < widthImage; x++) {

                // add all squares
                for (int k = -kernelSize; k <= kernelSize; k++) {
                    if ((0>x+k)||(x+k>=widthImage)) {
                        continue;
                    }
                    buffer[y*widthImage + x] += (*image)[y*widthImage + x+k][color] * pascalRow[k+kernelSize];
                }

                //normalize
                (*image)[y*widthImage + x][color] = buffer[y*widthImage + x] / pascalRowSum;
            }
        }

        // clean buffer
        std::fill(buffer.begin(), buffer.end(), 0);

        // vertical blur
        #pragma omp parallel for schedule(dynamic)
        for (size_t y = 0; y < heightImage; y++) {
            for (size_t x = 0; x < widthImage; x++) {

                // add all squares
                for (int k = -kernelSize; k <= kernelSize; k++) {
                    if ((0>y+k)||(y+k>=heightImage)) {
                        continue;
                    }
                    buffer[y*widthImage + x] += (*image)[(y+k)*widthImage + x][color] * pascalRow[k+kernelSize];
                }

                //normalize
                (*image)[y*widthImage + x][color] = buffer[y*widthImage + x] / pascalRowSum;
            }
        }

    }

    //printf("Corner after: \n");
    //printf("%d %d %d \n", int((*image)[0][0]), int((*image)[0][1]), int((*image)[0][2]));

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

    auto before = timeNow();
    blur(&imageVector, width, height, 5);
    printf("Blurring took %.5fs\n", (timeNow() - before)/1000.0);
    sf::Image blurred = vectorToImage(imageVector, width, height);

    sf::Texture texture;
    texture.loadFromImage(blurred);
    sf::Sprite sprite(texture);
    sf::RenderWindow window(sf::VideoMode(width, height), "Testing");


    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }

    return 0;
}
