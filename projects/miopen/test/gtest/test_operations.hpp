/*******************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2023 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/
#pragma once

#include "fast_test_ops.hpp"  // TRJS

namespace test {
template <typename DLModule>
void ComputeCPUBNInference(DLModule& dl_module)
{
    int size{0};
    miopenGetTensorDescriptorSize(&dl_module.input.desc, &size);
    // In case of NxCxDxHxW
    auto ReshapeIfNeeded = [size](auto& desc) {
        if(size == 5)
        {
            desc = miopen::BuildReshaped4DTensorDescriptor(desc);
        }
    };
    ReshapeIfNeeded(dl_module.input.desc);
    ReshapeIfNeeded(dl_module.out_ref.desc);
    ReshapeIfNeeded(dl_module.scale.desc);
    ReshapeIfNeeded(dl_module.shift.desc);
    ReshapeIfNeeded(dl_module.estMean.desc);
    ReshapeIfNeeded(dl_module.estVariance.desc);

    if (fto::LoadCPUBNInferenceTensorsFromFiles(dl_module.input,
                                      dl_module.out_ref,
                                      dl_module.scale,
                                      dl_module.shift,
                                      dl_module.epsilon,
                                      dl_module.estMean,
                                      dl_module.estVariance))   return; // TRJS
    auto start = sc::now(); // TRJS
    if(dl_module.bn_mode == miopenBNSpatial)
    {
        batchNormSpatialHostInference(dl_module.input,
                                      dl_module.out_ref,
                                      dl_module.scale,
                                      dl_module.shift,
                                      dl_module.epsilon,
                                      dl_module.estMean,
                                      dl_module.estVariance);
    }
    else if(dl_module.bn_mode == miopenBNPerActivation)
    {
        batchNormPerActivHostInference(dl_module.input,
                                       dl_module.out_ref,
                                       dl_module.scale,
                                       dl_module.shift,
                                       dl_module.epsilon,
                                       dl_module.estMean,
                                       dl_module.estVariance);
    }
    else
    {
        std::cout << "\nUnknown inference batch miopenBatchNormMode_t\n";
        exit(EXIT_FAILURE);
    }
    coutms("CPUInfer", start);  // TRJS
    (void)fto::WriteCPUBNInferenceTensorsToFiles(dl_module.input,
                                      dl_module.out_ref,
                                      dl_module.scale,
                                      dl_module.shift,
                                      dl_module.epsilon,
                                      dl_module.estMean,
                                      dl_module.estVariance); // TRJS
}

template <typename DLModule>
void ComputeCPUBNBwd(DLModule& dl_module)
{
    int size{0};
    miopenGetTensorDescriptorSize(&dl_module.input.desc, &size);
    // In case of NxCxDxHxW
    auto ReshapeIfNeeded = [size](auto& desc) {
        if(size == 5)
        {
            desc = miopen::BuildReshaped4DTensorDescriptor(desc);
        }
    };
    ReshapeIfNeeded(dl_module.input.desc);
    ReshapeIfNeeded(dl_module.dy.desc);
    ReshapeIfNeeded(dl_module.out_ref.desc);
    ReshapeIfNeeded(dl_module.bnScale.desc);
    ReshapeIfNeeded(dl_module.dScale_ref.desc);
    ReshapeIfNeeded(dl_module.dBias_ref.desc);
    ReshapeIfNeeded(dl_module.savedMean.desc);
    ReshapeIfNeeded(dl_module.savedInvVar.desc);

    if(dl_module.bn_mode == miopenBNSpatial)
    {
        batchNormSpatialHostBwdTrain(dl_module.input,
                                     dl_module.dy,
                                     dl_module.out_ref,
                                     dl_module.bnScale,
                                     dl_module.bnBias,
                                     dl_module.dScale_ref,
                                     dl_module.dBias_ref,
                                     dl_module.savedMean,
                                     dl_module.savedInvVar,
                                     dl_module.activ_mode,
                                     dl_module.activ_beta,
                                     dl_module.activ_alpha);
    }
    else if(dl_module.bn_mode == miopenBNPerActivation)
    {
        batchNormPerActHostBwdTrain(dl_module.input,
                                    dl_module.dy,
                                    dl_module.out_ref,
                                    dl_module.bnScale,
                                    dl_module.dScale_ref,
                                    dl_module.dBias_ref,
                                    dl_module.savedMean,
                                    dl_module.savedInvVar);
    }
    else
    {
        std::cout << "\nUnknown BwdTrain batch miopenBatchNormMode_t\n";
        exit(EXIT_FAILURE);
    }
}

template <typename DLModule>
void ComputeCPUBNFwdTrain(DLModule& dl_module)
{
    int size{0};
    miopenGetTensorDescriptorSize(&dl_module.input.desc, &size);
    // In case of NxCxDxHxW
    auto ReshapeIfNeeded = [size](auto& desc) {
        if(size == 5)
        {
            desc = miopen::BuildReshaped4DTensorDescriptor(desc);
        }
    };
    ReshapeIfNeeded(dl_module.input.desc);
    ReshapeIfNeeded(dl_module.out_ref.desc);
    ReshapeIfNeeded(dl_module.scale.desc);
    ReshapeIfNeeded(dl_module.shift.desc);
    ReshapeIfNeeded(dl_module.saveMean_ref.desc);
    ReshapeIfNeeded(dl_module.saveVariance_ref.desc);
    ReshapeIfNeeded(dl_module.runMean_ref.desc);
    ReshapeIfNeeded(dl_module.runVariance_ref.desc);

    if(dl_module.bn_mode == miopenBNSpatial)
    {
        batchNormSpatialHostFwdTrain(dl_module.input,
                                     dl_module.out_ref,
                                     dl_module.scale,
                                     dl_module.shift,
                                     dl_module.epsilon,
                                     dl_module.averageFactor,
                                     dl_module.saveMean_ref,
                                     dl_module.saveVariance_ref,
                                     dl_module.runMean_ref,
                                     dl_module.runVariance_ref);
    }
    else if(dl_module.bn_mode == miopenBNPerActivation)
    {
        batchNormPerActHostFwdTrain(dl_module.input,
                                    dl_module.out_ref,
                                    dl_module.scale,
                                    dl_module.shift,
                                    dl_module.epsilon,
                                    dl_module.averageFactor,
                                    dl_module.saveMean_ref,
                                    dl_module.saveVariance_ref,
                                    dl_module.runMean_ref,
                                    dl_module.runVariance_ref);
    }
    else
    {
        std::cout << "\nUnknown FwdTrain batch miopenBatchNormMode_t\n";
        exit(EXIT_FAILURE);
    }
}

template <typename T, typename U = double>
void CompareTensorGPU(const tensor<T>& output,
                   const tensor<U>& out_ref,
                   const double threshold,
                   std::optional<std::reference_wrapper<const tensor<U>>> out_gpu = std::nullopt,
                   std::optional<std::reference_wrapper<const tensor<U>>> ref_gpu = std::nullopt)
{
    auto timer = sc::now(); // TRJS
    EXPECT_FALSE(miopen::range_zero(out_ref)) << "CPU data is all zeros";
    coutmsreset("rzref", timer);
    EXPECT_FALSE(miopen::range_zero(output)) << "GPU data is all zeros";
    coutmsreset("rzout", timer);
    EXPECT_FALSE(miopen::find_idx(output, miopen::not_finite) >= 0)
        << "Non finite number found in the GPU data";
    coutmsreset("notfin_out", timer);
    EXPECT_TRUE(miopen::range_distance(out_ref) == miopen::range_distance(output));
    coutmsreset("lenequal", timer);
    double gpu_error = 0.0;
    if (out_gpu.has_value() && ref_gpu.has_value()){
        gpu_error = fto::gpu_rms_range(ref_gpu->get(), out_gpu->get());
        std::cout << "(gpu) " << gpu_error << " ";    // TRJS
    }
    auto error = miopen::rms_range(out_ref, output);
    coutmsreset("rms_range", timer);
    EXPECT_FALSE(miopen::find_idx(out_ref, miopen::not_finite) >= 0)
        << "Non finite number found in the CPU data";
    coutmsreset("notfin_ref", timer);
    std::cout << error << std::endl;    // TRJS
    EXPECT_TRUE(error < 0.0*threshold) // TRJS
        << "Error beyond tolerance Error:" << error << ",  Threshold: " << threshold;
}

template <typename T, typename U = double>
void CompareTensor(const tensor<T>& output,
                   const tensor<U>& out_ref,
                   const double threshold = std::numeric_limits<T>::epsilon())
{
    CompareTensorGPU(output, out_ref, threshold);
    // auto timer = sc::now(); // TRJS
    // EXPECT_FALSE(miopen::range_zero(out_ref)) << "CPU data is all zeros";
    // coutmsreset("rzref", timer);
    // EXPECT_FALSE(miopen::range_zero(output)) << "GPU data is all zeros";
    // coutmsreset("rzout", timer);
    // EXPECT_FALSE(miopen::find_idx(output, miopen::not_finite) >= 0)
    //     << "Non finite number found in the GPU data";
    // coutmsreset("notfin_out", timer);
    // EXPECT_TRUE(miopen::range_distance(out_ref) == miopen::range_distance(output));
    // coutmsreset("lenequal", timer);
    // auto error = miopen::rms_range(out_ref, output);
    // coutmsreset("rms_range", timer);
    // EXPECT_FALSE(miopen::find_idx(out_ref, miopen::not_finite) >= 0)
    //     << "Non finite number found in the CPU data";
    // coutmsreset("notfin_ref", timer);
    // std::cout << error << std::endl;    // TRJS
    // EXPECT_TRUE(error < threshold)
    //     << "Error beyond tolerance Error:" << error << ",  Threshold: " << threshold;
}
} // namespace test

namespace test {
namespace FusionPlan {
template <typename DLModule>
void InitFusionPlan(miopen::FusionPlanDescriptor& fusePlanDesc, DLModule& dl_module)
{
    fusePlanDesc = miopen::FusionPlanDescriptor(miopenVerticalFusion, dl_module.GetInputDesc());
}

template <typename DLModule>
void AddBnInfer(miopen::FusionPlanDescriptor& fusePlanDesc,
                miopen::OperatorArgs& params,
                DLModule& dl_module)
{
    auto bnOp = std::make_shared<miopen::BatchNormInferenceFusionOpDescriptor>(dl_module.bn_mode,
                                                                               dl_module.bn_desc);
    EXPECT_EQ(fusePlanDesc.AddOp(bnOp), miopenStatusSuccess);
    bnOp->SetArgs(params,
                  &dl_module.alpha,
                  &dl_module.beta,
                  dl_module.scale_dev.get(),
                  dl_module.shift_dev.get(),
                  dl_module.estMean_dev.get(),
                  dl_module.estVariance_dev.get(),
                  dl_module.epsilon);
}

template <typename DLModule>
void AddActiv(miopen::FusionPlanDescriptor& fusePlanDesc,
              miopen::OperatorArgs& params,
              DLModule& dl_module,
              miopenActivationMode_t activ_mode)
{
    auto activOp = std::make_shared<miopen::ActivFwdFusionOpDescriptor>(activ_mode);
    EXPECT_EQ(fusePlanDesc.AddOp(activOp), miopenStatusSuccess);
    EXPECT_EQ(activOp->SetArgs(params,
                               &dl_module.alpha,
                               &dl_module.beta,
                               dl_module.activ_alpha,
                               dl_module.activ_beta,
                               dl_module.activ_gamma),
              miopenStatusSuccess);
}

inline bool Skip(const miopen::Handle& handle)
{
    const std::string arch = handle.GetDeviceName();
    bool skip_test         = (arch != "gfx908" && arch != "gfx90a");

    return skip_test;
}
} // namespace FusionPlan
} // namespace test
