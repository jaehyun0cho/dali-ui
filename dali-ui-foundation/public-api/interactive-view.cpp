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
#include <dali-ui-foundation/public-api/interactive-view-impl.h>
#include <dali-ui-foundation/public-api/interactive-view.h>

namespace Dali
{

namespace Ui
{

InteractiveView::InteractiveView()
{
}

InteractiveView InteractiveView::New()
{
  InteractiveViewImplPtr impl = InteractiveViewImpl::New();

  InteractiveView handle(*impl);

  impl->Initialize();

  return handle;
}

InteractiveView InteractiveView::DownCast(BaseHandle handle)
{
  return View::DownCast<InteractiveView, InteractiveViewImpl>(handle);
}

InteractiveView::InteractiveView(const InteractiveView& view) = default;

InteractiveView::InteractiveView(InteractiveView&& rhs) noexcept = default;

InteractiveView::~InteractiveView()
{
}

InteractiveView::InteractiveView(InteractiveViewImpl& implementation)
: View(implementation)
{
}

InteractiveView::InteractiveView(Dali::Internal::CustomActor* internal)
: View(internal)
{
  VerifyCustomActorPointer<InteractiveViewImpl>(internal);
}

Signal<void(View, bool, InputEvent)>& InteractiveView::PressedChangedSignal()
{
  return GetImpl(*this).EnsureInteractiveTrait().PressedChangedSignal();
}

Signal<void(View, bool)>& InteractiveView::PseudoDisabledChangedSignal()
{
  return GetImpl(*this).EnsureInteractiveTrait().PseudoDisabledChangedSignal();
}

Signal<void(View, InputEvent)>& InteractiveView::ClickedSignal()
{
  return GetImpl(*this).EnsureInteractiveTrait().ClickedSignal();
}

Signal<bool(View, InputEvent)>& InteractiveView::LongPressedSignal()
{
  return GetImpl(*this).EnsureInteractiveTrait().LongPressedSignal();
}

bool InteractiveView::IsPressed() const
{
  return const_cast<InteractiveViewImpl&>(GetImpl(*this)).EnsureInteractiveTrait().IsPressed();
}

bool InteractiveView::IsPseudoDisabled() const
{
  return const_cast<InteractiveViewImpl&>(GetImpl(*this)).EnsureInteractiveTrait().IsPseudoDisabled();
}

void InteractiveView::SetPseudoDisabled(bool pseudoDisabled)
{
  GetImpl(*this).EnsureInteractiveTrait().SetPseudoDisabled(pseudoDisabled);
}

bool InteractiveView::IsClickable() const
{
  return const_cast<InteractiveViewImpl&>(GetImpl(*this)).EnsureInteractiveTrait().IsClickable();
}

void InteractiveView::SetClickable(bool clickable)
{
  GetImpl(*this).EnsureInteractiveTrait().SetClickable(clickable);
}

KeyClickPolicy InteractiveView::GetKeyClickPolicy() const
{
  return const_cast<InteractiveViewImpl&>(GetImpl(*this)).EnsureInteractiveTrait().GetKeyClickPolicy();
}

void InteractiveView::SetKeyClickPolicy(KeyClickPolicy policy)
{
  GetImpl(*this).EnsureInteractiveTrait().SetKeyClickPolicy(policy);
}

} // namespace Ui

} // namespace Dali
