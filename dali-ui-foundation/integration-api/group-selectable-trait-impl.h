#pragma once

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
#include <dali/public-api/common/intrusive-ptr.h>
#include <dali/public-api/object/weak-handle.h>
#include <dali/public-api/signals/connection-tracker.h>
#include <string>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/group-selectable-trait.h>
#include <dali-ui-foundation/public-api/selection-group.h>
#include <dali-ui-foundation/public-api/trait-object.h>
#include <dali-ui-foundation/public-api/view.h>

namespace Dali
{

namespace Ui
{

class ViewImpl;

namespace Integration
{

/**
 * @brief Internal implementation of GroupSelectableTrait.
 *
 * GroupSelectableTraitImpl is a plain TraitObject (it does NOT inherit
 * SelectableTraitImpl). It composes the owning View's SelectableTrait, ensuring
 * one exists on attach and reusing it thereafter, and orchestrates the View's
 * membership in a SelectionGroup through the public SelectionGroup::Find / Add /
 * Remove methods. The mutual-exclusion arbitration itself lives entirely in the
 * SelectableTraitImpl; this trait never performs it.
 *
 * Two declarative grouping mechanisms with a fixed precedence:
 * - An explicit group name (mGroupName) joins SelectionGroup::Find(name) eagerly.
 *   An explicit name always wins.
 * - Parent auto-grouping (mAutoGroupByParent, on by default) joins
 *   SelectionGroup::Find(parentView) on scene connection when no explicit name
 *   is set, and leaves it on scene disconnection (scene-scoped membership).
 *
 * The trait holds no strong references: the owner View is held weakly, and group
 * membership is expressed entirely through the SelectionGroup API (which holds
 * members weakly). It is a ConnectionTracker only to subscribe to the owner
 * View's scene signals; those connections are dropped on detach/destroy.
 */
class DALI_UI_API GroupSelectableTraitImpl : public TraitObject, public ConnectionTracker
{
public:
  /**
   * @copydoc Dali::Ui::GroupSelectableTrait::GroupSelectableTrait
   */
  GroupSelectableTraitImpl();

public: // API
  /**
   * @copydoc Dali::Ui::GroupSelectableTrait::SetGroupName
   */
  void SetGroupName(const std::string& name);

  /**
   * @copydoc Dali::Ui::GroupSelectableTrait::GetGroupName
   */
  std::string GetGroupName() const;

  /**
   * @copydoc Dali::Ui::GroupSelectableTrait::GetGroup
   */
  SelectionGroup GetGroup() const;

protected:
  /**
   * @copydoc Dali::Ui::GroupSelectableTrait::~GroupSelectableTrait
   */
  ~GroupSelectableTraitImpl() override;

  /**
   * @copydoc Dali::Ui::TraitObject::OnAttached
   */
  void OnAttached(TraitId id, View& view) override;

  /**
   * @copydoc Dali::Ui::TraitObject::OnDetaching
   */
  void OnDetaching(TraitId id, View& view) override;

  /**
   * @copydoc Dali::Ui::TraitObject::OnViewDestroying
   */
  void OnViewDestroying(ViewImpl* viewImpl) override;

private:
  /**
   * @brief Called when the owner View is connected to a scene.
   */
  void OnOwnerOnScene(Dali::Actor actor);

  /**
   * @brief Called when the owner View is disconnected from a scene.
   */
  void OnOwnerOffScene(Dali::Actor actor);

  /**
   * @brief Leaves whichever group the View currently belongs to (named or
   * parent-auto), clearing the auto-group record. Does not touch mGroupName.
   */
  void LeaveCurrentGroup();

  /**
   * @brief Joins the owner View's parent auto-group when applicable: parent
   * auto-grouping is enabled, the owner and its parent View resolve, and the
   * parent group is not already the recorded auto-group. Leaves a previously
   * auto-joined parent first. Does NOT check mGroupName, so callers must only
   * invoke this when no explicit group name is set.
   */
  void JoinParentGroupIfApplicable();

private:
  WeakHandle<View> mOwner;                 ///< WEAK: the owning View
  std::string      mGroupName;             ///< Explicit group name; empty means none
  WeakHandle<View> mAutoGroupParent;       ///< WEAK: parent whose group was auto-joined, if any
  bool             mAutoGroupByParent : 1; ///< Whether parent auto-grouping is enabled (default true)
};

} // namespace Integration

inline DALI_UI_API Integration::GroupSelectableTraitImpl& GetImpl(GroupSelectableTrait& obj)
{
  return static_cast<Integration::GroupSelectableTraitImpl&>(obj.GetBaseObject());
}

inline DALI_UI_API const Integration::GroupSelectableTraitImpl& GetImpl(const GroupSelectableTrait& obj)
{
  return static_cast<const Integration::GroupSelectableTraitImpl&>(obj.GetBaseObject());
}

} // namespace Ui

} // namespace Dali
