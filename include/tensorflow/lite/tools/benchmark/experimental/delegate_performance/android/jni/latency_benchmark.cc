/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/tools/benchmark/experimental/delegate_performance/android/jni/latency_benchmark.h"

#include <errno.h>
#include <sys/stat.h>

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "tensorflow/core/util/stats_calculator.h"
#include "tensorflow/lite/experimental/acceleration/configuration/configuration_generated.h"
#include "tensorflow/lite/logger.h"
#include "tensorflow/lite/minimal_logging.h"
#include "tensorflow/lite/profiling/memory_info.h"
#include "tensorflow/lite/tools/benchmark/benchmark_tflite_model.h"
#include "tensorflow/lite/tools/benchmark/experimental/delegate_performance/android/proto/delegate_performance.pb.h"

namespace tflite {
namespace benchmark {
namespace latency {
namespace {

static constexpr char kBenchmarkToolName[] = "(BenchmarkModelAndroid)";

// The listener subscribes to the benchmark lifecycle and parses the
// benchmarking results into a LatencyResults proto message. The message will be
// reported later to the app.
class DelegatePerformanceReportingListener : public BenchmarkListener {
 public:
  void OnBenchmarkStart(const BenchmarkParams& unused) override {
    results_proto_.set_event_type(proto::benchmark::BENCHMARK_EVENT_TYPE_START);
  }

  // TFLite Benchmark Tool triggers this method at the end of a benchmark for
  // logging the results.
  void OnBenchmarkEnd(const BenchmarkResults& results) override {
    ReportResult(results);
  }

  void ReportFailure(TfLiteStatus status) {
    std::string status_msg =
        status == kTfLiteError
            ? "TFLite error"
            : (status == kTfLiteDelegateError ? "TFLite delegate error"
                                              : "unexpected TFLite status");
    TFLITE_LOG_PROD(TFLITE_LOG_ERROR,
                    "Benchmark failed due to %s with status code %d.",
                    status_msg.c_str(), status);
    results_proto_.set_event_type(proto::benchmark::BENCHMARK_EVENT_TYPE_ERROR);
    results_proto_.mutable_error()->mutable_error_code()->set_tflite_error(
        status);
    results_proto_.mutable_error()->set_error_message(status_msg);
  }

  const proto::benchmark::LatencyResults& GetResults() {
    return results_proto_;
  }

 private:
  // TODO(b/262399611): Consider putting metric related logic into a separate
  // file.
  void ReportResult(const BenchmarkResults& results) {
    tensorflow::Stat<int64_t> warmup_us = results.warmup_time_us();
    tensorflow::Stat<int64_t> inference_us = results.inference_time_us();
    profiling::memory::MemoryUsage init_mem_usage = results.init_mem_usage();
    profiling::memory::MemoryUsage overall_mem_usage =
        results.overall_mem_usage();

    AddMetric(/*name=*/"model_size_megabyte",
              /*value=*/results.model_size_mb());
    AddMetric(/*name=*/"initialization_latency_us",
              /*value=*/results.startup_latency_us());
    AddMetric(/*name=*/"warmup_latency_average_us", /*value=*/warmup_us.avg());
    AddMetric(/*name=*/"warmup_latency_min_us", /*value=*/warmup_us.min());
    AddMetric(/*name=*/"warmup_latency_max_us", /*value=*/warmup_us.max());
    AddMetric(/*name=*/"warmup_latency_standard_deviation",
              /*value=*/warmup_us.std_deviation());
    AddMetric(/*name=*/"inference_latency_average_us",
              /*value=*/inference_us.avg());
    AddMetric(/*name=*/"inference_latency_min_us",
              /*value=*/inference_us.min());
    AddMetric(/*name=*/"inference_latency_max_us",
              /*value=*/inference_us.max());
    AddMetric(/*name=*/"inference_latency_standard_deviation",
              /*value=*/inference_us.std_deviation());
    AddMetric(/*name=*/"initialization_memory_max_rss_mebibyte",
              /*value=*/init_mem_usage.mem_footprint_kb / 1024.0);
    AddMetric(/*name=*/"initialization_memory_total_allocated_mebibyte",
              /*value=*/init_mem_usage.total_allocated_bytes / 1024.0 / 1024.0);
    AddMetric(
        /*name=*/"initialization_memory_in_use_mebibyte",
        /*value=*/init_mem_usage.in_use_allocated_bytes / 1024.0 / 1024.0);
    AddMetric(/*name=*/"overall_memory_max_rss_mebibyte",
              /*value=*/overall_mem_usage.mem_footprint_kb / 1024.0);
    AddMetric(
        /*name=*/"overall_memory_total_allocated_mebibyte",
        /*value=*/overall_mem_usage.total_allocated_bytes / 1024.0 / 1024.0);
    AddMetric(
        /*name=*/"overall_memory_in_use_mebibyte",
        /*value=*/overall_mem_usage.in_use_allocated_bytes / 1024.0 / 1024.0);
    results_proto_.set_event_type(proto::benchmark::BENCHMARK_EVENT_TYPE_END);
    TFLITE_LOG_PROD(TFLITE_LOG_INFO, "Benchmark finished.");
  }

  void AddMetric(std::string name, float value) {
    proto::benchmark::BenchmarkMetric* metric = results_proto_.add_metrics();
    metric->set_name(name);
    metric->set_value(value);
  }

  proto::benchmark::LatencyResults results_proto_;
};

// Converts the input TFLiteSettings into TFLite Benchmark Tool arguments.
// Please see tensorflow/lite/tools/benchmark.
std::vector<std::string> ParseArgumentsFromTfLiteSettings(
    const TFLiteSettings& tflite_settings,
    const std::string& tflite_settings_path) {
  std::vector<std::string> args;
  if (tflite_settings_path.empty()) {
    return args;
  }
  if (tflite_settings.stable_delegate_loader_settings()) {
    args.push_back(absl::StrFormat("--stable_delegate_settings_file=%s",
                                   tflite_settings_path));
    return args;
  }
  if (tflite_settings.delegate() == Delegate_XNNPACK) {
    args.push_back("--use_xnnpack=true");
    if (tflite_settings.xnnpack_settings() &&
        tflite_settings.xnnpack_settings()->num_threads()) {
      args.push_back(
          absl::StrFormat("--num_threads=%d",
                          tflite_settings.xnnpack_settings()->num_threads()));
    } else if (tflite_settings.delegate() == Delegate_GPU) {
      args.push_back("--use_gpu=true");
      const tflite::GPUSettings* gpu_settings = tflite_settings.gpu_settings();
      if (gpu_settings) {
        if (gpu_settings->is_precision_loss_allowed()) {
          args.push_back("--gpu_precision_loss_allowed=true");
        }
        if (gpu_settings->enable_quantized_inference()) {
          args.push_back("--gpu_experimental_enable_quant=true");
        }
        if (gpu_settings->inference_preference() ==
            GPUInferenceUsage_GPU_INFERENCE_PREFERENCE_SUSTAINED_SPEED) {
          args.push_back("--gpu_inference_for_sustained_speed=true");
        }
        if (gpu_settings->force_backend() == GPUBackend_OPENCL) {
          args.push_back("--gpu_backend=cl");
        } else if (gpu_settings->force_backend() == GPUBackend_OPENGL) {
          args.push_back("--gpu_backend=gl");
        }
        if (gpu_settings->cache_directory()) {
          args.push_back(
              absl::StrFormat("--delegate_serialize_dir=%s",
                              gpu_settings->cache_directory()->c_str()));
        }
        if (gpu_settings->model_token()) {
          args.push_back(absl::StrFormat("--delegate_serialize_token=%s",
                                         gpu_settings->model_token()->c_str()));
        }
      }
    } else if (tflite_settings.disable_default_delegates()) {
      // Currently TFLite Benchmark Tool doesn't support handling the case with
      // applying XNNPack delegate explicitly and disabling the XNNPack delegate
      // as the default delegate at the same time. When the
      // "disable_default_delegates" configuration is set to true, it only takes
      // effect if the delegate is not set to XNNPack. Otherwise, the default
      // delegates will still be enabled.
      args.push_back("--use_xnnpack=false");
    }
  }
  return args;
}
}  // namespace

proto::benchmark::LatencyResults Benchmark(
    const std::vector<std::string>& args, const TFLiteSettings& tflite_settings,
    const std::string& tflite_settings_path) {
  // Constructs a fake argv command-line object for the benchmark.
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>(kBenchmarkToolName));
  // TODO(b/250877013): Remove the "args" argument here by using model file
  // descriptors for latency benchmarking.
  for (const std::string& arg : args) {
    argv.push_back(const_cast<char*>(arg.data()));
  }
  std::vector<std::string> args_from_tflite_settings =
      ParseArgumentsFromTfLiteSettings(tflite_settings, tflite_settings_path);
  for (const std::string& arg : args_from_tflite_settings) {
    argv.push_back(const_cast<char*>(arg.data()));
  }

  BenchmarkTfLiteModel benchmark;
  DelegatePerformanceReportingListener delegatePerformanceReporting;
  benchmark.AddListener(&delegatePerformanceReporting);
  TfLiteStatus status = benchmark.Run(argv.size(), argv.data());
  if (status != kTfLiteOk) {
    delegatePerformanceReporting.ReportFailure(status);
  }
  return delegatePerformanceReporting.GetResults();
}

}  // namespace latency
}  // namespace benchmark
}  // namespace tflite
