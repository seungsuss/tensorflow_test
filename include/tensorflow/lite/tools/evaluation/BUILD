# Copyright 2019 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

load("//tensorflow/lite:build_def.bzl", "tflite_copts", "tflite_linkopts")
load("//tensorflow/lite/core/shims:cc_library_with_tflite.bzl", "cc_library_with_stable_tflite_abi", "cc_test_with_tflite")

package(
    # copybara:uncomment default_applicable_licenses = ["//tensorflow:license"],
    default_visibility = ["//visibility:public"],
    licenses = ["notice"],
)

exports_files(glob([
    "testdata/**",
]))

cc_library(
    name = "evaluation_stage",
    hdrs = ["evaluation_stage.h"],
    copts = tflite_copts(),
    deps = [
        "//tensorflow/lite/core/c:common",
        "//tensorflow/lite/tools/evaluation/proto:evaluation_config_cc_proto",
    ],
)

cc_library_with_stable_tflite_abi(
    name = "utils",
    srcs = ["utils.cc"],
    hdrs = ["utils.h"],
    copts = tflite_copts(),
    non_stable_abi_deps = [
        "//tensorflow/lite/delegates/nnapi:nnapi_delegate",
    ] + select({
        "//tensorflow:ios": [
            "//tensorflow/lite/delegates/coreml:coreml_delegate",
        ],
        "//tensorflow:macos_arm64": [
            "//tensorflow/lite/delegates/coreml:coreml_delegate",
        ],
        "//conditions:default": [],
    }) + select({
        "//tensorflow/lite/delegates/gpu:supports_gpu_delegate": [
            "//tensorflow/lite/delegates/gpu:delegate",
        ],
        "//conditions:default": [],
    }) + select({
        "//tensorflow:arm_any": [
            "//tensorflow/lite/delegates/hexagon:hexagon_delegate",
        ],
        "//conditions:default": [],
    }),
    tflite_deps = [
        "//tensorflow/lite/core/shims:c_api",
        "//tensorflow/lite/core/shims:delegate_plugin",
        "//tensorflow/lite/core/shims:common",
        "//tensorflow/lite/tools/delegates:delegate_provider_hdr",
        "//tensorflow/lite/tools/delegates:delegate_provider_lib",
    ],
    tflite_deps_selects = [{
        # XNNPACK does not support s390x
        # (see <https://github.com/tensorflow/tensorflow/pull/51655>).
        "//tensorflow:linux_s390x": [],
        "//tensorflow/lite:tflite_with_xnnpack_explicit_false": [],
        "//conditions:default": [
            "//tensorflow/lite/core/shims:xnnpack_plugin",
        ],
    }],
    deps = select({
        # XNNPACK does not support s390x
        # (see <https://github.com/tensorflow/tensorflow/pull/51655>).
        "//tensorflow:linux_s390x": [],
        "//tensorflow/lite:tflite_with_xnnpack_explicit_false": [],
        "//conditions:default": [
            "//tensorflow/lite/delegates/xnnpack:xnnpack_delegate_hdrs_only",
            "//tensorflow/lite/experimental/acceleration/configuration:configuration_fbs",
        ],
    }),
)

cc_library(
    name = "evaluation_delegate_provider",
    srcs = ["evaluation_delegate_provider.cc"],
    hdrs = ["evaluation_delegate_provider.h"],
    copts = tflite_copts(),
    deps = [
        ":utils",
        "//tensorflow/lite/tools:command_line_flags",
        "//tensorflow/lite/tools:logging",
        "//tensorflow/lite/tools:tool_params",
        "//tensorflow/lite/tools/delegates:delegate_provider_hdr",
        "//tensorflow/lite/tools/delegates:tflite_execution_providers",
        "//tensorflow/lite/tools/evaluation/proto:evaluation_stages_cc_proto",
    ],
)

cc_test_with_tflite(
    name = "utils_test",
    srcs = ["utils_test.cc"],
    data = [
        "testdata/empty.txt",
        "testdata/labels.txt",
    ],
    linkopts = tflite_linkopts(),
    linkstatic = 1,
    tflite_deps = [
        ":utils",
    ],
    deps = [
        "//tensorflow/lite:context",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "evaluation_delegate_provider_test",
    srcs = ["evaluation_delegate_provider_test.cc"],
    linkopts = tflite_linkopts(),
    deps = [
        ":evaluation_delegate_provider",
        "//tensorflow/lite/tools:tool_params",
        "//tensorflow/lite/tools/evaluation/proto:evaluation_stages_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)