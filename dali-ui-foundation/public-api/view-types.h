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

namespace Dali
{
namespace Ui
{

/**
 * @brief Specifies how corner radius values are interpreted.
 *
 * @note Enum values match Ui::Visual::Transform::Policy::Type internally.
 */
enum class CornerRadiusPolicy
{
  RELATIVE = 0, ///< Relative to the view size (percentage [0.0, 0.5] of the shorter side)
  ABSOLUTE = 1, ///< Absolute value in world units (default)
};

/**
 * @brief Controls whether sibling-order changes (Raise/Lower/RaiseAbove/LowerBelow)
 * also reorder the layout children list.
 *
 * Visual z-order (Actor sibling order) and layout order are kept in sync by default.
 * Use PRESERVE when only the drawing/hit order should change while layout arrangement
 * stays the same.
 */
enum class LayoutOrderPolicy
{
  UPDATE   = 0, ///< Also reorder layout children to match the new sibling order.
  PRESERVE = 1, ///< Keep layout children order unchanged; only visual z-order is affected.
};

/**
 * @brief Controls whether a layout-order change (View::Insert) also reorders
 * the visual z-order (Actor sibling order).
 *
 * This is the CONVERSE of LayoutOrderPolicy: LayoutOrderPolicy controls whether
 * a sibling-order (z-order) change also reorders the layout children, whereas
 * ZOrderPolicy controls whether a layout-order change (View::Insert) also
 * reorders the visual z-order.
 */
enum class ZOrderPolicy
{
  UPDATE   = 0, ///< Also reorder visual z-order (Actor sibling order) to match the new layout order.
  PRESERVE = 1, ///< Keep visual z-order unchanged; only layout children order changes.
};

/**
 * @brief Controls whether a child-query operation targets only the
 * layout-participating View children or all children.
 *
 * A View's layout children are the View-typed children that participate in
 * layout. A View can also hold non-View Actor children (added via
 * IntegrationView::AddActorChild) that do not participate in layout.
 */
enum class ChildScopePolicy
{
  LAYOUT_CHILDREN = 0, ///< Only layout-participating View children (the current default).
  ALL_CHILDREN    = 1, ///< All children including non-View Actors added via IntegrationView::AddActorChild.
};

/**
 * @brief Controls how View::Remove(View, RemovePolicy) treats an attached
 * LayoutTransition's EXIT slot.
 *
 * ENTER is dispatched automatically for every add path (Actor::Add, Insert),
 * but EXIT cannot be hooked transparently on removal, so the EXIT intent must
 * be requested explicitly through this policy.
 *
 * @note IMMEDIATE is not identical to the inherited Actor::Remove(Actor): it
 * still runs the View's child bookkeeping and the in-flight-ghost guard, but
 * unparents now and skips BOTH the view's own EXIT slot and any inherited
 * SUBTREE-scope EXIT effect.
 */
enum class RemovePolicy
{
  IMMEDIATE    = 0, ///< Unparent now; do not run any EXIT transition.
  ANIMATE_EXIT = 1, ///< Run the attached EXIT transition (own or inherited SUBTREE) first, then unparent; immediate when no EXIT slot is configured.
};

} // namespace Ui
} // namespace Dali
