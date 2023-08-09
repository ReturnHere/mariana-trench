/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <sparta/HashedSetAbstractDomain.h>

#include <mariana-trench/ExtraTrace.h>
#include <mariana-trench/GroupHashedSetAbstractDomain.h>

namespace marianatrench {

using ExtraTraceSet = sparta::HashedSetAbstractDomain<ExtraTrace>;

}
