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

// CLASS HEADER
#include <dali-ui-foundation/integration-api/layouts/layout-impl.h>

// EXTERNAL INCLUDES
#include <dali/devel-api/object/type-registry-helper.h>
#include <dali/devel-api/object/type-registry.h>

namespace Dali
{
namespace Ui
{
namespace Integration
{

namespace
{

BaseHandle Create()
{
  return Layout::New();
}

// Type Registration
DALI_TYPE_REGISTRATION_BEGIN(Ui::Integration::LayoutImpl, Ui::ViewImpl, Create)
DALI_TYPE_REGISTRATION_END()

} // namespace

LayoutImplPtr LayoutImpl::New()
{
  LayoutImplPtr impl(new LayoutImpl());

  // PURE producer, on exactly the same grounds as ViewImpl::New(): LayoutImpl adds no
  // OnArrange override and attaches no LayoutManager of its own, so the object built
  // here -- whose most-derived type IS LayoutImpl -- has ViewImpl::OnArrange ->
  // ArrangeDefault as its provable arrange producer. That reads only the bounds, the
  // padding, each child's margin / requested position / measured size and the
  // effective scale, all of which are cache KEY terms or invalidation-tracked state,
  // and it arranges the same child set for the same inputs.
  //
  // This one matters beyond its own cache. A bare Ui::Layout is the canonical
  // container, and the arrange cache's subtree gate re-tests purity at EVERY node it
  // would elide, so leaving this undeclared kept not only the Layout itself but every
  // ANCESTOR of one permanently out of the hit.
  //
  // Declared HERE and not in the constructor, deliberately: AbsoluteLayoutImpl,
  // StackLayoutImpl, GridLayoutImpl and FlexLayoutImpl all derive from this class and
  // run their OWN New(), so they cannot inherit the declaration -- they get their
  // purity from the LayoutManager each attaches instead. See ViewImpl::New().
  impl->SetArrangePurity(ArrangePurity::PURE);

  return impl;
}

LayoutImpl::LayoutImpl()
: ViewImpl()
{
}

LayoutImpl::~LayoutImpl()
{
}

} // namespace Integration
} // namespace Ui
} // namespace Dali
