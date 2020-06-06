#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include "utils.h"
#include <cuda.h>
#include <cuda_runtime.h>
#include <string>

cv::Mat imageRGBA;
cv::Mat imageOutput;

uchar4        *d_rgbaImage__;
uchar4        *d_outputImage__;

size_t numRows() { return imageRGBA.rows; }
size_t numCols() { return imageRGBA.cols; }

// los tipos de retorno son void ya que cualquier error interno resultará en un cierre
// por eso no se retornan errores...
// Devuelve un puntero de la version RGBA de la imagen de entrada
// y un puntero a la imagend e canal unico de la salida
// para ambos huesped y GPU
void preProcess(uchar4 **inputImage, uchar4  **outputImage,
                uchar4 **d_rgbaImage, uchar4 **d_outputImage,
                const std::string &filename) {
  //Comprobar que el contexto se inicializa bien
  checkCudaErrors(cudaFree(0));

  cv::Mat image;
  image = cv::imread(filename.c_str(), CV_LOAD_IMAGE_COLOR);
  
  if (image.empty()) {
    std::cerr << "No se pudo abrir el archivo: " << filename << std::endl;
    exit(1);
  }

  cv::cvtColor(image, imageRGBA, CV_BGR2RGBA);

  // Reserva memoria para el output
  imageOutput.create(image.rows, image.cols, CV_8UC4);

  //This shouldn't ever happen given the way the images are created
  //at least based upon my limited understanding of OpenCV, but better to check
  if (!imageRGBA.isContinuous() || !imageOutput.isContinuous()) {
    std::cerr << "Images aren't continuous!! Exiting." << std::endl;
    exit(1);
  }

  //Apuntamos al comienzo de las filas
  *inputImage = (uchar4 *)imageRGBA.ptr<unsigned char>(0);
  *outputImage  = (uchar4 *)imageOutput.ptr<unsigned char>(0);

  const size_t numPixels = numRows() * numCols();
  //Reserva memoria en el dispositivo
  checkCudaErrors(cudaMalloc(d_rgbaImage, sizeof(uchar4) * numPixels));
  checkCudaErrors(cudaMalloc(d_outputImage, sizeof(uchar4) * numPixels));
  checkCudaErrors(cudaMemset(*d_outputImage, 0, numPixels * sizeof(uchar4))); // Asegurate de que no queda memoria sin liberar

  // Copia el input en la GPU
  checkCudaErrors(cudaMemcpy(*d_rgbaImage, *inputImage, sizeof(uchar4) * numPixels, cudaMemcpyHostToDevice));

  d_rgbaImage__ = *d_rgbaImage;
  d_outputImage__ = *d_outputImage;
}

