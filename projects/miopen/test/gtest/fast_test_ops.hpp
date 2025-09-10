#pragma once

#include "tensor_holder.hpp"

#include <chrono>
#include <filesystem>   // TRJS
#include <fstream>
#include <iomanip>

namespace { using sc = std::chrono::steady_clock; }
#undef tomillis
#define tomillis(__DUR) (0.001 * std::chrono::duration_cast<std::chrono::microseconds>(__DUR).count())
#undef coutms
#define coutms(__TOK, __TP) (std::cout << "ms[" << std::setw(16) << __TOK << "]: " << std::setw(12) << tomillis(sc::now() - __TP) << std::endl)

#define TMP_ROOT "/ramdisk/"

namespace fto {

template <class T>
bool LoadTensorFromFile(std::string path, tensor<T>& tensor, bool verbose = false)
{
    std::filesystem::path filePath{TMP_ROOT + path};
    if (!std::filesystem::exists(filePath))  { std::cout << "Read failure, '" << path.c_str() << "' does not exist." << std::endl; return false; }
    std::ifstream file(filePath);
    if (!file.is_open()) return false;
    if (verbose) std::cout << "Loading '" << filePath.c_str() << "'." << std::endl;
    serialize(file, tensor);
    return true;
}
template <class T>
bool WriteTensorToFile(std::string path, tensor<T>& tensor, bool verbose = false)
{
    std::filesystem::path filePath{TMP_ROOT + path};
    if (std::filesystem::exists(filePath)) { std::cout << "Write failure, '" << path.c_str() << "' exists." << std::endl; return false; }
    std::ofstream file(filePath);
    if (!file.is_open()) return false;
    if (verbose) std::cout << "Writing '" << filePath.c_str() << "'." << std::endl;
    serialize(file, tensor);
    return true;
}

template <class T, class Tref, class U, class V = U>
bool LoadCPUBNInferenceTensorsFromFiles(
    tensor<T>& input,
                                   tensor<Tref>& output,
                                   tensor<U>& scale,
                                   tensor<U>& bias,
                                   double epsilon,
                                   tensor<V>& estimatedMean,
                                   tensor<V>& estimatedVariance)
{
//    if (!LoadTensorFromFile("bni_input.dat", input, true))    return false;
    if (!LoadTensorFromFile("bni_output.dat", output, true))    return false;
    if (!LoadTensorFromFile("bni_scale2.dat", scale, true))    return false;
    if (!LoadTensorFromFile("bni_bias.dat", bias, true))    return false;
    if (!LoadTensorFromFile("bni_estMean2.dat", estimatedMean, true))    return false;
    if (!LoadTensorFromFile("bni_estVariance2.dat", estimatedVariance, true))    return false;
    return true;
}
template <class T, class Tref, class U, class V = U>
bool WriteCPUBNInferenceTensorsToFiles(
    tensor<T>& input,
                                   tensor<Tref>& output,
                                   tensor<U>& scale,
                                   tensor<U>& bias,
                                   double epsilon,
                                   tensor<V>& estimatedMean,
                                   tensor<V>& estimatedVariance)
{
//    if (!WriteTensorToFile("bni_input.dat", input, true))    return false;
    if (!WriteTensorToFile("bni_output.dat", output, true))    return false;
    if (!WriteTensorToFile("bni_scale2.dat", scale, true))    return false;
    if (!WriteTensorToFile("bni_bias.dat", bias, true))    return false;
    if (!WriteTensorToFile("bni_estMean2.dat", estimatedMean, true))    return false;
    if (!WriteTensorToFile("bni_estVariance2.dat", estimatedVariance, true))    return false;
    return true;
}

} // namespace fto
