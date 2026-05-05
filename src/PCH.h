#pragma once

#pragma warning(push)
#include "RE/Fallout.h"
#include "F4SE/F4SE.h"
#pragma warning(pop)

#define DLLEXPORT __declspec(dllexport)

#include "shim/bhkPickData.h"

#include <SimpleIni.h>

#include <spdlog/sinks/basic_file_sink.h>

namespace logger = F4SE::log;
using namespace std::literals;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <numbers>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
