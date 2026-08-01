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

#include <stdlib.h>
#include <iostream>

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/internal/views/view/view-data-impl.h>
#include <dali-ui-foundation/public-api/configuration/ui-scale-manager.h>
#include <dali-ui-foundation/public-api/configuration/ui-scale-policy.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <dali-ui-test-suite-utils.h>
#include <dali.h>

using namespace Dali;
using namespace Dali::Ui;

using Dali::Ui::Internal::ViewDataImpl;

void utc_dali_ui_scale_logical_context_internal_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_ui_scale_logical_context_internal_cleanup(void)
{
  test_return_value = TET_PASS;
}

// White-box coverage for the effective-scale sync bit (mLogicalContextValid) and
// for the arrange-cache bookkeeping that depends on it.
//
// The arrange cache now has a hit path, but it serves CHILDLESS views only, so the
// bookkeeping pinned here is still not observable through geometry from most of the
// shapes these tests build. These tests therefore read the bits directly through
// ViewDataImpl's white-box accessors, which states the claim about the bits
// themselves rather than about a hit that happens to depend on them. The
// producer-level, geometry-observable side is covered by the arrange cache suites
// (utc-Dali-ArrangeCacheHit-internal.cpp and the black-box cases in utc-Dali-View.cpp).
//
// UiScaleManagerImpl keeps the scale in a process-wide singleton, so every test
// here records the incoming scale and restores it before returning.

namespace
{
ViewDataImpl& DataOf(View view)
{
  return ViewDataImpl::Get(GetImpl(view));
}

// Drives one full layout batch to completion.
void Settle(UiTestApplication& application)
{
  application.SendNotification();
  application.Render();
  application.SendNotification();
  application.Render();
}

} // namespace

// Required test: a UI scale change must invalidate the arrange cache of every
// view in the affected subtree, not just the registered layout root.
//
// Non-vacuity (verified by mutation): removing `mArrangeCacheValid = false` from
// InvalidateLayoutCaches -- reached by the invalidation path SetScale runs over
// the subtree -- leaves both views' arrange caches valid and the second half of
// this test fails.
int UtcDaliUiScaleArrangeCacheInvalidatedByScaleChangeP(void)
{
  UiTestApplication application;
  tet_infoline("Arrange cache of a settled subtree is dropped by a UI scale change");

  const float originalScale = UiScaleManager::Get().GetScale();
  UiScaleManager::Get().SetScale(1.0f);

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(200.0f);

  View child = View::New();
  child.SetRequestedWidth(50.0f);
  child.SetRequestedHeight(50.0f);
  root.Add(child);

  application.GetScene().Add(root);
  Settle(application);

  DALI_TEST_CHECK(DataOf(root).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(child).IsArrangeCacheValid());

  // No layout pass is run after the scale change: the invalidation itself, not a
  // subsequent pass, is what must clear the caches.
  UiScaleManager::Get().SetScale(1.5f);

  DALI_TEST_CHECK(!DataOf(root).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(child).IsArrangeCacheValid());

  UiScaleManager::Get().SetScale(originalScale);
  END_TEST;
}

// Anti-vacuity partner for the test above: a settled pass really does publish the
// arrange cache, so "false after SetScale" is a transition and not the only state
// this bit is ever in.
int UtcDaliUiScaleArrangeCachePublishedAfterCleanPassP(void)
{
  UiTestApplication application;
  tet_infoline("A clean arrange pass publishes the arrange cache");

  const float originalScale = UiScaleManager::Get().GetScale();
  UiScaleManager::Get().SetScale(1.0f);

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(200.0f);

  View child = View::New();
  child.SetRequestedWidth(50.0f);
  child.SetRequestedHeight(50.0f);
  root.Add(child);

  DALI_TEST_CHECK(!DataOf(root).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(child).IsArrangeCacheValid());

  application.GetScene().Add(root);
  Settle(application);

  DALI_TEST_CHECK(DataOf(root).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(child).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(root).IsArrangeDirty());
  DALI_TEST_CHECK(!DataOf(child).IsArrangeDirty());

  UiScaleManager::Get().SetScale(originalScale);
  END_TEST;
}

// mLogicalContextValid is the sync bit for mEffectiveScale: false until the first
// GetEffectiveScale() computes a value, true afterwards, and false again after
// anything that can move the effective scale.
int UtcDaliUiScaleLogicalContextSyncBitP(void)
{
  UiTestApplication application;
  tet_infoline("mLogicalContextValid tracks whether the cached effective scale is usable");

  const float originalScale = UiScaleManager::Get().GetScale();
  UiScaleManager::Get().SetScale(1.0f);

  View view = View::New();
  view.SetRequestedWidth(100.0f);
  view.SetRequestedHeight(100.0f);

  // Nothing has asked for the effective scale yet.
  DALI_TEST_CHECK(!DataOf(view).IsLogicalContextValid());

  DALI_TEST_EQUALS(GetImpl(view).GetEffectiveScale(), 1.0f, 0.001f, TEST_LOCATION);
  DALI_TEST_CHECK(DataOf(view).IsLogicalContextValid());

  // On-scene so the view is a registered layout root and a global scale change
  // reaches it.
  application.GetScene().Add(view);
  Settle(application);
  DALI_TEST_CHECK(DataOf(view).IsLogicalContextValid());

  UiScaleManager::Get().SetScale(1.5f);
  DALI_TEST_CHECK(!DataOf(view).IsLogicalContextValid());

  DALI_TEST_EQUALS(GetImpl(view).GetEffectiveScale(), 1.5f, 0.001f, TEST_LOCATION);
  DALI_TEST_CHECK(DataOf(view).IsLogicalContextValid());

  // A policy change re-roots this view's scale derivation.
  view.SetUiScalePolicy(UiScalePolicy::DISABLED);
  DALI_TEST_CHECK(!DataOf(view).IsLogicalContextValid());

  DALI_TEST_EQUALS(GetImpl(view).GetEffectiveScale(), 1.0f, 0.001f, TEST_LOCATION);
  DALI_TEST_CHECK(DataOf(view).IsLogicalContextValid());

  UiScaleManager::Get().SetScale(originalScale);
  END_TEST;
}

// Arrange() establishes this view's logical context at pass entry.
//
// The default arrange of a CHILDLESS view reads the effective scale nowhere --
// ArrangeDefault's only read sits behind its child loop -- so without the read at
// pass entry an arrange-only visit leaves the sync bit false.
//
// Non-vacuity (verified by mutation): removing the `mViewImpl.GetEffectiveScale()`
// at the top of ViewDataImpl::Arrange makes the first check below fail.
int UtcDaliUiScaleArrangeEntryEstablishesLogicalContextP(void)
{
  UiTestApplication application;
  tet_infoline("Arrange() establishes the logical context even for a childless view");

  const float originalScale = UiScaleManager::Get().GetScale();
  UiScaleManager::Get().SetScale(1.0f);

  View leaf = View::New();
  leaf.SetRequestedWidth(50.0f);
  leaf.SetRequestedHeight(50.0f);

  // Never measured, never arranged: the context has not been established yet.
  DALI_TEST_CHECK(!DataOf(leaf).IsLogicalContextValid());

  // Arrange alone -- no Measure() -- must leave the context established.
  GetImpl(leaf).Arrange(LayoutRect(0.0f, 0.0f, 50.0f, 50.0f));
  DALI_TEST_CHECK(DataOf(leaf).IsLogicalContextValid());

  // And a settled childless leaf publishes its arrange cache, which the publish
  // gate only does while the logical context is valid.
  application.GetScene().Add(leaf);
  Settle(application);

  DALI_TEST_CHECK(DataOf(leaf).IsLogicalContextValid());
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());

  UiScaleManager::Get().SetScale(originalScale);
  END_TEST;
}

// A scale-context reset invalidates cached RESULTS; it must not swallow pending
// WORK.
//
// The reset recurses over the subtree, but the InvalidateMeasure() its callers
// follow it with only re-arms the node it is called on and that node's
// ANCESTORS. So a descendant's dirty bit, once cleared by the reset, has nothing
// to give it back -- it is dropped, not deferred. (Its one remaining reader is
// OnViewSceneConnection's standalone self-registration.)
//
// Non-vacuity (verified by mutation): restoring `mMeasureDirty = false` in
// ViewDataImpl::InvalidateLayoutCaches makes the post-reset check below fail.
int UtcDaliUiScaleResetKeepsDescendantMeasureDirtyP(void)
{
  UiTestApplication application;
  tet_infoline("An ancestor scale reset preserves a descendant's unconsumed measure dirty");

  const float originalScale = UiScaleManager::Get().GetScale();
  UiScaleManager::Get().SetScale(1.0f);

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(200.0f);

  View child = View::New();
  child.SetRequestedWidth(50.0f);
  child.SetRequestedHeight(50.0f);
  root.Add(child);

  application.GetScene().Add(root);
  Settle(application);

  // Baseline: the settled pass consumed every dirty bit, so the dirty asserted
  // on below is genuinely raised by the next statement and not left over.
  DALI_TEST_CHECK(!DataOf(root).IsMeasureDirty());
  DALI_TEST_CHECK(!DataOf(child).IsMeasureDirty());

  // Raise real, unconsumed work on the DESCENDANT and do not run a pass: the
  // dirty is still standing when the reset arrives.
  child.SetRequestedWidth(80.0f);
  DALI_TEST_CHECK(DataOf(child).IsMeasureDirty());

  // Reset the scale context from the ANCESTOR. This recurses into the child;
  // the follow-up InvalidateMeasure() runs on the root and walks upward, so it
  // never reaches the child.
  root.SetUiScalePolicy(UiScalePolicy::DISABLED);

  // The reset retracts cached results; the pending work must survive it.
  // (The cache bits are not asserted here: the invalidation that raised the
  // dirty above already cleared them, so they would say nothing about the
  // reset. What the reset reaches on the descendant is covered by
  // UtcDaliUiScaleResetClearsWholeSubtreeLogicalContextP.)
  DALI_TEST_CHECK(DataOf(child).IsMeasureDirty());

  // And the layout that follows still services it.
  Settle(application);
  DALI_TEST_EQUALS(child.GetProperty<float>(Actor::Property::SIZE_WIDTH), 80.0f, 0.001f, TEST_LOCATION);

  UiScaleManager::Get().SetScale(originalScale);
  END_TEST;
}

// The reset is RECURSIVE: it must reach every descendant, not just the node it
// is called on and its direct children.
//
// Non-vacuity (verified by mutation): dropping the mChildren loop from
// ViewDataImpl::InvalidateLogicalContextRecursive leaves the child and the
// grandchild with a live sync bit and the last two checks fail.
int UtcDaliUiScaleResetClearsWholeSubtreeLogicalContextP(void)
{
  UiTestApplication application;
  tet_infoline("A scale reset clears the logical context of the whole subtree");

  const float originalScale = UiScaleManager::Get().GetScale();
  UiScaleManager::Get().SetScale(1.0f);

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(200.0f);

  View child = View::New();
  child.SetRequestedWidth(100.0f);
  child.SetRequestedHeight(100.0f);
  root.Add(child);

  View grandChild = View::New();
  grandChild.SetRequestedWidth(50.0f);
  grandChild.SetRequestedHeight(50.0f);
  child.Add(grandChild);

  application.GetScene().Add(root);
  Settle(application);

  // Every level has a live sync bit before the reset, so each check below is a
  // transition rather than a state the tree happened to be in already.
  DALI_TEST_CHECK(DataOf(root).IsLogicalContextValid());
  DALI_TEST_CHECK(DataOf(child).IsLogicalContextValid());
  DALI_TEST_CHECK(DataOf(grandChild).IsLogicalContextValid());

  // No layout pass afterwards: the reset itself must clear the whole subtree.
  root.SetUiScalePolicy(UiScalePolicy::DISABLED);

  DALI_TEST_CHECK(!DataOf(root).IsLogicalContextValid());
  DALI_TEST_CHECK(!DataOf(child).IsLogicalContextValid());
  DALI_TEST_CHECK(!DataOf(grandChild).IsLogicalContextValid());

  UiScaleManager::Get().SetScale(originalScale);
  END_TEST;
}
