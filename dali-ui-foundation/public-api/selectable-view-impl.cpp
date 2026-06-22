/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// EXTERNAL INCLUDES
#include <dali/devel-api/object/type-registry-helper.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/selectable-view-impl.h>
#include <dali-ui-foundation/public-api/selectable-view.h>

namespace Dali
{

namespace Ui
{

namespace
{

BaseHandle Create()
{
  return SelectableView::New();
}

// Type Registration
DALI_TYPE_REGISTRATION_BEGIN(SelectableViewImpl, InteractiveViewImpl, Create)
DALI_TYPE_REGISTRATION_END()

} // namespace

SelectableViewImplPtr SelectableViewImpl::New()
{
  return new SelectableViewImpl();
}

void SelectableViewImpl::OnInitialize()
{
  InteractiveViewImpl::OnInitialize();
  EnsureSelectableTrait();
}

SelectableViewImpl::SelectableViewImpl()
{
}

SelectableViewImpl::~SelectableViewImpl()
{
}

} // namespace Ui

} // namespace Dali
