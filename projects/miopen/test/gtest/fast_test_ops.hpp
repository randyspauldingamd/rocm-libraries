#pragma once

#include "tensor_holder.hpp"

#include <chrono>
#include <filesystem>   // TRJS
#include <fstream>
#include <iomanip>

#define FTO_TIMING 0
#define FTO_USE_DRIVE_CACHE 0

namespace { using sc = std::chrono::steady_clock; }
#undef tomillis
#define tomillis(__DUR) (0.001 * std::chrono::duration_cast<std::chrono::microseconds>(__DUR).count())
#undef coutms
#undef coutmsreset
#if FTO_TIMING
#define coutms(__TOK, __TP) (std::cout << "ms[" << std::setw(16) << __TOK << "]: " << std::setw(12) << tomillis(sc::now() - __TP) << std::endl)
#else
#define coutms(__TOK, __TP) (__TP = __TP)
#endif
#define coutmsreset(__TOK, __TP) {coutms(__TOK, __TP); __TP = sc::now();}

#define TMP_ROOT "/ramdisk/"
#define FTO_VERBOSE false
// from test/verify.hpp
template <class R>
using range_value = typename std::decay<decltype(*std::declval<R>().begin())>::type;

namespace fto {

template <class T>
bool LoadTensorFromFile(std::string path, tensor<T>& tensor, bool verbose = FTO_VERBOSE)
{
    if (!FTO_USE_DRIVE_CACHE) { std::cout << "skip: FTO_USE_DRIVE_CACHE is 0" << std::endl; return false; }
    std::filesystem::path filePath{TMP_ROOT + path};
    if (!std::filesystem::exists(filePath))  { std::cout << "Read failure, '" << path.c_str() << "' does not exist." << std::endl; return false; }
    std::ifstream file(filePath);
    if (!file.is_open()) return false;
    if (verbose) std::cout << "Loading '" << filePath.c_str() << "'." << std::endl;
    serialize(file, tensor);
    return true;
}
template <class T>
bool WriteTensorToFile(std::string path, tensor<T>& tensor, bool verbose = FTO_VERBOSE)
{
    if (!FTO_USE_DRIVE_CACHE) { std::cout << "skip: FTO_USE_DRIVE_CACHE is 0" << std::endl; return false; }
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
    if (!LoadTensorFromFile("bni_output.dat", output))    return false;
    if (!LoadTensorFromFile("bni_scale2.dat", scale))    return false;
    if (!LoadTensorFromFile("bni_bias.dat", bias))    return false;
    if (!LoadTensorFromFile("bni_estMean2.dat", estimatedMean))    return false;
    if (!LoadTensorFromFile("bni_estVariance2.dat", estimatedVariance))    return false;
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
    if (!WriteTensorToFile("bni_output.dat", output))    return false;
    if (!WriteTensorToFile("bni_scale2.dat", scale))    return false;
    if (!WriteTensorToFile("bni_bias.dat", bias))    return false;
    if (!WriteTensorToFile("bni_estMean2.dat", estimatedMean))    return false;
    if (!WriteTensorToFile("bni_estVariance2.dat", estimatedVariance))    return false;
    return true;
}

template <class R1, class R2>
double gpu_rms_range(R1&& r1, R2&& r2)
{
    // std::size_t n = range_distance(r1);
    // if(n == range_distance(r2))
    // {
    //     if(n == 0)
    //         return 0;
    //     double square_difference = range_product(r1, r2, 0.0, sum_fn{}, square_diff);
    //     double mag1 = static_cast<double>(*std::max_element(r1.begin(), r1.end(), compare_mag));
    //     double mag2 = static_cast<double>(*std::max_element(r2.begin(), r2.end(), compare_mag));
    //     double mag =
    //         std::max({std::fabs(mag1), std::fabs(mag2), std::numeric_limits<double>::min()});
    //     return std::sqrt(square_difference) / (std::sqrt(n) * mag);
    // }
    // else
        return double(std::numeric_limits<range_value<R1>>::max());
}

} // namespace fto
