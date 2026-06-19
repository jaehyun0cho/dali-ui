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
#include <dali-ui-foundation/public-api/group-selectable-trait.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/group-selectable-trait-impl.h>

namespace Dali
{

namespace Ui
{

GroupSelectableTrait::GroupSelectableTrait()
{
}

GroupSelectableTrait GroupSelectableTrait::New()
{
  IntrusivePtr<Integration::GroupSelectableTraitImpl> impl = new Integration::GroupSelectableTraitImpl();
  return GroupSelectableTrait(impl.Get());
}

GroupSelectableTrait::GroupSelectableTrait(const GroupSelectableTrait& trait)
: BaseHandle(trait)
{
}

GroupSelectableTrait::~GroupSelectableTrait()
{
}

GroupSelectableTrait::GroupSelectableTrait(Integration::GroupSelectableTraitImpl* implementation)
: BaseHandle(implementation)
{
}

GroupSelectableTrait GroupSelectableTrait::DownCast(BaseHandle handle)
{
  return GroupSelectableTrait(dynamic_cast<Integration::GroupSelectableTraitImpl*>(handle.GetObjectPtr()));
}

void GroupSelectableTrait::SetGroupName(const std::string& name)
{
  GetImpl(*this).SetGroupName(name);
}

std::string GroupSelectableTrait::GetGroupName() const
{
  return GetImpl(*this).GetGroupName();
}

SelectionGroup GroupSelectableTrait::GetGroup() const
{
  return GetImpl(*this).GetGroup();
}

} // namespace Ui

} // namespace Dali
