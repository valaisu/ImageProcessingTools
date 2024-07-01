#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <map>
#include <chrono>
#include <omp.h>
#include <immintrin.h>


uint64_t timeNow() {
   using namespace std::chrono;
   return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
} 

/*

This program implements some image processing tools with CPU
compile: 
g++ -fopenmp -o display main.cpp -lsfml-graphics -lsfml-window -lsfml-system

*/

/*
- x coordinates: 0 <= x < width
- y coordinates: 0 <= y < height
- color components: 0 <= c < 3
- input: data[c*width*height + y*width + x]
*/
using ImageVec = std::vector<unsigned char>;


// Loads an image to a flattened vector
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

    int pixels = height*width;
    ImageVec imageVector(pixels*3);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            sf::Color color = image.getPixel(x, y);
            imageVector[0*pixels + y*width + x] = color.r;
            imageVector[1*pixels + y*width + x] = color.g;
            imageVector[2*pixels + y*width + x] = color.b;
        }
    }

    return imageVector;
}


sf::Image vectorToImage(const unsigned char *imageVec, int width, int height) {
    sf::Image image;
    int size = width*height;
    image.create(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            sf::Color color(imageVec[size*0 + y*width + x], 
                            imageVec[size*1 + y*width + x], 
                            imageVec[size*2 + y*width + x]);
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
void blur(unsigned char* image, const int widthImage, const int heightImage, const int kernelSize) {
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

    const int stepSize = 1;
    const int imageSize = widthImage*heightImage;
    const int widthImagePad = (widthImage+stepSize-1)/stepSize*stepSize;
    const int heightImagePad = (heightImage+stepSize-1)/stepSize*stepSize;
    const int imageSizePad = heightImagePad * widthImagePad;

    const int pascalRowSum = power(2, kernelSize*2); // 2^(kernelSize*2) 
    const int pascalRowNumber = kernelSize*2; // the first row has number or index "0"

    std::vector<int> pascalRow(pascalRowNumber+1);
    for (int i = 0; i <= pascalRowNumber; i++) {
        pascalRow[i] = int(binomialCoefficient(pascalRowNumber, i));
    }

    //printf("Corner before: \n");
    //printf("%d %d %d \n", int((*image)[0][0]), int((*image)[0][1]), int((*image)[0][2]));

    // horizontal blur moves data in here, and vertical blur moves it back out
    std::vector<unsigned int> buffer(imageSizePad);

    for (int color = 0; color < 3; color++) {
        // clean buffer
        std::fill(buffer.begin(), buffer.end(), 0);

        // vertical blur
        #pragma omp parallel for schedule(dynamic)
        for (size_t y = 0; y < heightImage; y++) {
            for (size_t x = 0; x < widthImagePad; x+=stepSize) {
                // TODO: load 16 bytes at time, calc all simultaneously
                //__m128i dataChars = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&image[color*imageSize + y*widthImage + x]));
                //__m256i dataShorts = _mm256_cvtepu8_epi16(dataChars);
                //__m256i dataIntsLow = _mm256_cvtepu16_epi32(_mm256_castsi256_si128(dataShorts));
                //__m256i dataIntsHigh = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(dataShorts, 1));

                
                // add all squares
                int start = std::max(-kernelSize, -int(y));
                int end = std::min(kernelSize, heightImage-int(y));
                for (int k = start; k <= end; k++) {
                    buffer[y*widthImage + x] += image[color*imageSize + (y+k)*widthImage + x] * pascalRow[k+kernelSize];
                }

                //normalize
                image[color*imageSize + y*widthImage + x] = buffer[y*widthImage + x] / pascalRowSum;
            }
        }

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
                    buffer[y*widthImage + x] += image[color*imageSize + y*widthImage + x+k] * pascalRow[k+kernelSize];
                }

                //normalize
                image[color*imageSize + y*widthImage + x] = buffer[y*widthImage + x] / pascalRowSum;
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
    blur(&imageVector[0], width, height, 5);
    printf("Blurring took %.5fs\n", (timeNow() - before)/1000.0);
    sf::Image blurred = vectorToImage(&imageVector[0], width, height);

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
