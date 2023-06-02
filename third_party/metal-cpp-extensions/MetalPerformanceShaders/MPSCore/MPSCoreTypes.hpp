/*
 *
 * Copyright 2022 Mark Grimes. Most/all of the work is copied from Apple so copyright is theirs if they want it.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// MetalPerformanceShaders/MPSCore/MPSCoreTypes.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <MetalPerformanceShaders/MetalPerformanceShadersPrivate.hpp>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MPS
{
	_NS_ENUM(NS::UInteger, ImageFeatureChannelFormat) {
		ImageFeatureChannelFormatNone        = 0,
		ImageFeatureChannelFormatUnorm8      = 1,
		ImageFeatureChannelFormatUnorm16     = 2,
		ImageFeatureChannelFormatFloat16     = 3,
		ImageFeatureChannelFormatFloat32     = 4,
		ImageFeatureChannelFormat_reserved0  = 5,
		ImageFeatureChannelFormatCount
	};
}
