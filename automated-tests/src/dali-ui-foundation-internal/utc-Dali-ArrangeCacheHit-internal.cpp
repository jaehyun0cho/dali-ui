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
#include <algorithm>
#include <iostream>

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/extension-api/view.h>
#include <dali-ui-foundation/integration-api/label-impl.h>
#include <dali-ui-foundation/integration-api/reserved-trait-id.h>
#include <dali-ui-foundation/integration-api/view-integ.h>
#include <dali-ui-foundation/internal/layouts/layout-manager-impl.h>
#include <dali-ui-foundation/internal/views/view/view-data-impl.h>
#include <dali-ui-foundation/public-api/layouts/absolute-layout.h>
#include <dali-ui-foundation/public-api/layouts/absolute-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/flex-layout.h>
#include <dali-ui-foundation/public-api/layouts/flex-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/grid-layout.h>
#include <dali-ui-foundation/public-api/layouts/grid-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/scroll-view-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout-manager.h>
#include <dali-ui-foundation/public-api/views/scroll/scroll-view.h>
#include <dali-ui-foundation/public-api/video/video-source.h>
#include <dali-ui-foundation/public-api/video/video-view.h>
#include <dali-ui-foundation/public-api/views/canvas/canvas-view.h>
#include <dali-ui-foundation/public-api/views/image/animated-image-view.h>
#include <dali-ui-foundation/public-api/views/image/image-view.h>
#include <dali-ui-foundation/public-api/views/image/lottie-animation-view.h>
#include <dali-ui-foundation/public-api/views/text-controls/input-editor.h>
#include <dali-ui-foundation/public-api/views/text-controls/input-field.h>
#include <dali-ui-foundation/public-api/views/text-controls/label.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <dali-ui-foundation/public-api/views/web/web-view.h>
#include <dali-ui-test-suite-utils.h>
#include <dali.h>

namespace IntegrationView   = Dali::Ui::Integration::View;
namespace ReservedTraitId   = Dali::Ui::Integration::ReservedTraitId;

using namespace Dali;
using namespace Dali::Ui;

using Dali::Ui::Internal::ViewDataImpl;

void utc_dali_arrange_cache_hit_internal_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_arrange_cache_hit_internal_cleanup(void)
{
  test_return_value = TET_PASS;
}

// White-box coverage for the ARRANGE cache HIT (childless views).
//
// The black-box suite observes the hit through a counting arrange producer, which
// is the right instrument for "did the producer run" but says nothing about the
// bookkeeping state the hit leaves behind, or about the DEBUG invariant it asserts.
// Those two are pinned here, plus the one externally visible side effect a hit is
// required to keep: the LayoutFinished signal.

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

int gLayoutFinishedCount = 0;

void OnLayoutFinished(View, LayoutRect)
{
  ++gLayoutFinishedCount;
}

// A counting arrange producer that echoes its input slot and does nothing else, so
// its invocation count measures exactly one thing: whether that view's Arrange() ran
// its producer or served a cache HIT.
int gCallbackArrangeCount = 0;

LayoutRect CountingCallbackArrange(View, const LayoutRect& bounds)
{
  ++gCallbackArrangeCount;
  return bounds;
}

// VideoSource::New() forwards providerId/nativeSession opaquely to the adaptor
// plugin and never dereferences nativeSession itself, and no platform video-player
// plugin is loaded here. A dummy address stands in for a real player handle. Same
// construction as utc-Dali-VideoView.cpp.
int gDummyVideoSession = 0;

VideoSource CreateTestVideoSource()
{
  return VideoSource::New("test.provider", &gDummyVideoSession, VideoSourceOptions(), VideoRenderingMode::Underlay);
}

// A stand-in for a THIRD-PARTY view: a ViewImpl subclass that overrides OnArrange
// and counts its invocations. The only difference between two instances is whether
// the FACTORY declared the override pure, which is exactly the surface the opt-in
// mechanism protects -- an author who never reads the documentation writes the
// `declarePure == false` variant by doing nothing at all.
//
// The declaration lives in New() and not in the constructor, which is the pattern
// ViewImpl::SetArrangePurity documents: the factory is where the most-derived type is
// fixed, so a declaration made there cannot leak into a subclass.
class PurityCounterViewImpl : public ViewImpl
{
public:
  static IntrusivePtr<PurityCounterViewImpl> New(bool declarePure)
  {
    IntrusivePtr<PurityCounterViewImpl> impl(new PurityCounterViewImpl());
    if(declarePure)
    {
      impl->SetArrangePurity(ArrangePurity::PURE);
    }
    // else: nothing. ArrangePurity::IMPURE is the default, which is the whole point.
    return impl;
  }

  int GetArrangeCallCount() const
  {
    return mArrangeCount;
  }

  // SetArrangePurity is protected on ViewImpl (it is a declaration a view makes
  // about ITSELF, not something a caller may assert on someone else's behalf), so a
  // test that exercises re-declaration has to go through a subclass -- exactly as a
  // real third-party view would.
  void Declare(ArrangePurity purity)
  {
    SetArrangePurity(purity);
  }

protected:
  PurityCounterViewImpl()
  : ViewImpl()
  {
  }

  LayoutRect OnArrange(const LayoutRect& bounds) override
  {
    ++mArrangeCount;
    return ViewImpl::OnArrange(bounds);
  }

private:
  int mArrangeCount{0};
};

// Register so TypeInfo lookup can walk the chain.
Dali::TypeRegistration purityCounterViewTypeReg(
  typeid(PurityCounterViewImpl), typeid(ViewImpl), nullptr);

View CreatePurityCounterView(bool declarePure)
{
  auto impl = PurityCounterViewImpl::New(declarePure);
  return View(*impl);
}

PurityCounterViewImpl& PurityCounterImplOf(View view)
{
  return static_cast<PurityCounterViewImpl&>(GetImpl(view));
}

LayoutRect SettledSlotOf(View view)
{
  return LayoutRect(view.GetProperty<float>(Actor::Property::POSITION_X),
                    view.GetProperty<float>(Actor::Property::POSITION_Y),
                    view.GetProperty<float>(Actor::Property::SIZE_WIDTH),
                    view.GetProperty<float>(Actor::Property::SIZE_HEIGHT));
}

// A stand-in for a THIRD-PARTY subclass OF A FIRST-PARTY PURE VIEW. LabelImpl declares
// its OnArrange pure -- but from LabelImpl::New(), the one place where the object being
// declared about provably has LabelImpl as its most-derived type. This subclass builds
// itself through its OWN New(), which declares NOTHING, and overrides OnArrange with a
// stand-in for an impure body (a counter here; a real one would read world geometry or
// push to a native sink).
//
// Purity must therefore NOT reach it. If LabelImpl declared from its CONSTRUCTOR
// instead, it would: a base constructor runs for every subclass, so this view would
// carry mArrangeOverridePure == true without its author writing anything, and its
// impure override would be silently skipped in favour of a cached rect. That is the
// regression this class exists to catch.
//
// `declarePure` builds the opted-in variant through the SAME subclass, so the two
// instances differ in the declaration and nothing else -- which is what makes the
// "undeclared always misses" half non-vacuous.
class ProbeLabelImpl : public Dali::Ui::Integration::LabelImpl
{
public:
  static IntrusivePtr<ProbeLabelImpl> New(bool declarePure)
  {
    IntrusivePtr<ProbeLabelImpl> impl(new ProbeLabelImpl());
    if(declarePure)
    {
      // What a third-party author who DID read the documentation writes: the
      // declaration is made here, for the exact type this factory builds.
      impl->SetArrangePurity(ArrangePurity::PURE);
    }
    // else: nothing at all, which must leave it IMPURE despite LabelImpl being PURE.
    return impl;
  }

  int GetArrangeCallCount() const
  {
    return mArrangeCount;
  }

protected:
  ProbeLabelImpl()
  : Dali::Ui::Integration::LabelImpl()
  {
  }

  LayoutRect OnArrange(const LayoutRect& bounds) override
  {
    ++mArrangeCount;
    return bounds;
  }

private:
  int mArrangeCount{0};
};

// Register so TypeInfo lookup can walk the chain.
Dali::TypeRegistration probeLabelTypeReg(
  typeid(ProbeLabelImpl), typeid(Dali::Ui::Integration::LabelImpl), nullptr);

View CreateProbeLabel(bool declarePure)
{
  auto impl = ProbeLabelImpl::New(declarePure);
  View view(*impl);
  impl->Initialize();
  return view;
}

ProbeLabelImpl& ProbeLabelImplOf(View view)
{
  return static_cast<ProbeLabelImpl&>(GetImpl(view));
}

// --- LayoutManager purity (Phase 5c) --------------------------------------
//
// A COUNTING layout manager. A manager IS its owner's arrange producer, so "did the
// producer run" can only be counted by a manager that counts itself -- and a counting
// SUBCLASS of an in-library manager is no use, because the purity declaration is per
// exact type and a subclass is therefore IMPURE by construction (which is precisely
// what UtcDaliArrangeCacheLayoutManagerPurityIsPerExactTypeP pins).
//
// This one derives straight from LayoutManager and makes its own declaration through
// exactly the mechanism the in-library managers use, so the count below measures the
// real path end to end. Its Arrange stacks children vertically from layout-tracked
// state only -- measured sizes and the child list -- so the PURE variant is honest.
//
// `declarePure == false` is what an author who declares nothing gets, and is used both
// as the mutation-free control and to plant an impure manager in a subtree.
int gManagerArrangeCount = 0;

class CountingLayoutManager : public LayoutManager
{
public:
  explicit CountingLayoutManager(bool declarePure)
  : LayoutManager()
  {
    if(declarePure)
    {
      GetImplAs<LayoutManager::Impl>()->DeclareArrangePurity(ArrangePurity::PURE, typeid(CountingLayoutManager));
    }
    // else: nothing. ArrangePurity::IMPURE is the default.
  }

  MeasuredSize Measure(ViewImpl* view, float widthConstraint, float heightConstraint) override
  {
    float          maxWidth    = 0.0f;
    float          totalHeight = 0.0f;
    const uint32_t count       = GetChildViewCount(view);

    for(uint32_t i = 0; i < count; ++i)
    {
      View child = GetChildViewAt(view, i);
      if(!child)
      {
        continue;
      }
      ViewImpl& childImpl = GetImpl(child);
      if(IsStandalone(&childImpl))
      {
        continue;
      }
      MeasuredSize childSize = childImpl.Measure(widthConstraint, heightConstraint);
      maxWidth               = std::max(maxWidth, childSize.width);
      totalHeight += childSize.height;
    }
    return MeasuredSize(maxWidth, totalHeight);
  }

  void Arrange(ViewImpl* view, const LayoutRect&) override
  {
    ++gManagerArrangeCount;

    float          y     = 0.0f;
    const uint32_t count = GetChildViewCount(view);

    for(uint32_t i = 0; i < count; ++i)
    {
      View child = GetChildViewAt(view, i);
      if(!child)
      {
        continue;
      }
      ViewImpl& childImpl = GetImpl(child);
      if(IsStandalone(&childImpl))
      {
        continue;
      }
      MeasuredSize childMeasured = childImpl.GetMeasuredSize();
      childImpl.Arrange(LayoutRect(0.0f, y, childMeasured.width, childMeasured.height));
      y += childMeasured.height;
    }
  }
};

// A third-party subclass of an in-library manager that DECLARED PURE. It declares
// nothing of its own, which is what an author who never read the documentation writes.
class SubclassedStackLayoutManager : public StackLayoutManager
{
public:
  SubclassedStackLayoutManager()
  : StackLayoutManager(StackOrientation::VERTICAL, 0.0f)
  {
  }
};

} // namespace

// A cache hit consumes nothing: the entry it served is still valid afterwards, so
// the NEXT identical pass hits as well. This is what makes the optimisation
// monotone rather than a one-shot -- and it only holds because the hit returns
// BEFORE ArrangePassGuard, whose constructor would clear mArrangeCacheValid.
//
// Non-vacuity (verified by mutation): clearing mArrangeCacheValid in the hit body
// leaves the entry dead after the first hit and the post-hit assertions below fail.
// (The related "construct ArrangePassGuard before the hit test" mistake is NOT
// caught here -- it makes every pass a miss, which re-publishes a valid cache -- but
// it is caught by the black-box producer count in
// UtcDaliViewArrangeCacheHitSkipsLeafProducerP.)
int UtcDaliArrangeCacheHitStaysValidAcrossAHitP(void)
{
  UiTestApplication application;
  tet_infoline("A leaf's arrange cache entry survives being served");

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(100.0f);
  application.GetScene().Add(root);

  View leaf = View::New();
  leaf.SetRequestedWidth(50.0f);
  leaf.SetRequestedHeight(40.0f);
  root.Add(leaf);

  Settle(application);

  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeDirty());

  const LayoutRect leafSlot(leaf.GetProperty<float>(Actor::Property::POSITION_X),
                            leaf.GetProperty<float>(Actor::Property::POSITION_Y),
                            leaf.GetProperty<float>(Actor::Property::SIZE_WIDTH),
                            leaf.GetProperty<float>(Actor::Property::SIZE_HEIGHT));

  for(int pass = 0; pass < 3; ++pass)
  {
    leaf.Arrange(leafSlot);
    DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());
    DALI_TEST_CHECK(!DataOf(leaf).IsArrangeDirty());
    DALI_TEST_EQUALS(DataOf(leaf).GetLastArrangeDirection(), leaf.GetEffectiveLayoutDirection(), TEST_LOCATION);
  }

  END_TEST;
}

// Corollary C, from the outside: a live arrange cache implies a live effective-scale
// sync bit, because every scale-context reset clears both. The hit body asserts this
// in DEBUG (DALI_ASSERT_DEBUG(mLogicalContextValid)); this test states the same claim
// as a normal assertion so it is checked in every build configuration, immediately
// before a pass that will take the hit.
//
// Non-vacuity (verified by mutation): removing the InvalidateLayoutCaches() call from
// ViewDataImpl::InvalidateMeasure breaks the pairing -- the arrange cache survives an
// invalidation that dropped the logical context, which the InvalidateMeasure half
// below catches directly (and which would make the hit's DEBUG assert fire on the
// next identical pass).
int UtcDaliArrangeCacheHitAssertsLogicalContextP(void)
{
  UiTestApplication application;
  tet_infoline("A live arrange cache implies a live effective-scale sync bit");

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(100.0f);
  application.GetScene().Add(root);

  View leaf = View::New();
  leaf.SetRequestedWidth(50.0f);
  leaf.SetRequestedHeight(40.0f);
  root.Add(leaf);

  Settle(application);

  // The state the hit's DEBUG assert relies on.
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(leaf).IsLogicalContextValid());

  const LayoutRect leafSlot(leaf.GetProperty<float>(Actor::Property::POSITION_X),
                            leaf.GetProperty<float>(Actor::Property::POSITION_Y),
                            leaf.GetProperty<float>(Actor::Property::SIZE_WIDTH),
                            leaf.GetProperty<float>(Actor::Property::SIZE_HEIGHT));

  // Takes the hit. In a DEBUG build the assert inside it is the live check; in any
  // build the state assertions around it hold.
  leaf.Arrange(leafSlot);

  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(leaf).IsLogicalContextValid());

  // The pairing itself, over both callers that drop the logical context. Each must
  // drop the arrange cache in the same breath, or a later hit would serve a result
  // computed against the old scale -- with no term in the predicate to catch it.
  GetImpl(leaf).InvalidateMeasure();
  DALI_TEST_CHECK(!DataOf(leaf).IsLogicalContextValid());
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeCacheValid());

  Settle(application);
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(leaf).IsLogicalContextValid());

  // The recursive form, which is what a global UI scale change uses.
  DataOf(leaf).InvalidateLogicalContextRecursive();
  DALI_TEST_CHECK(!DataOf(leaf).IsLogicalContextValid());
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeCacheValid());

  END_TEST;
}

// LayoutFinished semantics are pass-based, not work-based: a subscriber is told its
// view was arranged in this pass, and a cache hit IS an arrange of that view. The
// signal must therefore keep firing on frames where the leaf's producer never runs.
//
// Non-vacuity (verified by mutation): dropping the
// `if(HasLayoutFinishedSignalConnections()) LayoutController::NotifyViewArranged(...)`
// block from the hit body makes the subscribed leaf stop emitting on hitting passes
// and the second half of this test fails.
int UtcDaliArrangeCacheHitEmitsLayoutFinishedP(void)
{
  UiTestApplication application;
  tet_infoline("A settled leaf still emits LayoutFinished on a pass it serves from cache");

  gLayoutFinishedCount = 0;

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(100.0f);
  application.GetScene().Add(root);

  View leafA = View::New();
  leafA.SetRequestedWidth(50.0f);
  leafA.SetRequestedHeight(40.0f);
  root.Add(leafA);

  View leafB = View::New();
  leafB.SetRequestedWidth(50.0f);
  leafB.SetRequestedHeight(40.0f);
  root.Add(leafB);

  leafA.LayoutFinishedSignal().Connect(&OnLayoutFinished);

  Settle(application);

  const int settledEmits = gLayoutFinishedCount;
  DALI_TEST_CHECK(settledEmits > 0);
  DALI_TEST_CHECK(DataOf(leafA).IsArrangeCacheValid());

  // A pass driven entirely by the SIBLING. leafA's own inputs are unchanged, so its
  // Arrange serves the cache -- and must still register as arranged.
  leafB.SetRequestedX(11.0f);
  Settle(application);

  DALI_TEST_CHECK(DataOf(leafA).IsArrangeCacheValid());
  DALI_TEST_CHECK(gLayoutFinishedCount > settledEmits);

  END_TEST;
}

// The arrange cache-HIT serves a stored rect INSTEAD of running the producer, which
// is result-identical only if the producer is a pure function of the cache key. The
// framework never assumes that: ArrangePurity::IMPURE is the DEFAULT, and a producer
// is skipped only after whoever wrote it declared it PURE. So the property this test
// pins is audit-independent -- a first-party view nobody looked at simply does not get
// the optimisation, rather than silently desyncing. (The matching statement about
// third-party SUBCLASSES of a declared view is a different claim, and is pinned
// separately in UtcDaliArrangeCacheHitThirdPartySubclassDoesNotInheritPurityP.)
//
// VideoViewImpl::OnArrange and WebViewImpl::OnArrange are the two first-party
// producers that MUST stay undeclared: both read Actor::Property::SCREEN_POSITION (a
// function of the whole ancestor chain, in no cache key, invalidating nothing here)
// and push it to a surface outside the actor tree, so serving either from cache would
// strand that native surface at a stale offset.
//
// This pins BOTH halves: the declarations are where §5 of the design says they are
// (and, for Video/Web, absent), and the derived bit is actually honoured by the hit
// predicate.
//
// The second half instruments the two leaves with counting ArrangeCallbacks rather
// than their own OnArrange, deliberately: an identical producer on both leaves
// isolates the declaration as the only difference between them. It also keeps the
// observation headless-safe -- the native display-area push has nothing to observe
// with no platform plugin loaded. Note the callbacks carry their OWN purity, which is
// why the pure leaf's is installed through the two-argument overload.
//
// Non-vacuity (verified by mutation): dropping `mArrangeProducerPure &&` from the hit
// predicate in ViewDataImpl::Arrange lets the undeclared leaf hit, and its
// `undeclaredBase + PASSES` producer-count assertion fails; adding
// SetArrangePurity(PURE) to VideoViewImpl fails the first half.
int UtcDaliArrangeCacheHitImpureFirstPartyLeavesNeverCacheP(void)
{
  UiTestApplication application;
  tet_infoline("VideoView and WebView leave their arrange producer undeclared and never take the cache hit");

  // --- Part 1: the declarations ---------------------------------------------
  //
  // Every one of these is declared pure by its OWN New() factory -- ViewImpl::New()
  // for the plain View, where the producer is provably ViewImpl::OnArrange ->
  // ArrangeDefault, and LabelImpl::New(), ImageViewImpl::New(), ... for the rest. The
  // factory is the one place where the most-derived type is fixed, which is why the
  // declaration is NOT made in a constructor (that would leak it to subclasses; see
  // UtcDaliArrangeCacheHitThirdPartySubclassDoesNotInheritPurityP). A dropped
  // declaration costs performance, never correctness, which makes it invisible in
  // behaviour -- these assertions are what make that loss visible instead.
  View              plain    = View::New();
  Label             label    = Label::New();
  ImageView         image    = ImageView::New();
  AnimatedImageView animated = AnimatedImageView::New();
  LottieAnimationView lottie = LottieAnimationView::New();
  CanvasView        canvas   = CanvasView::New(Vector2(100.0f, 100.0f));
  InputField        field    = InputField::New();
  InputEditor       editor   = InputEditor::New();

  DALI_TEST_CHECK(DataOf(plain).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(label).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(image).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(animated).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(lottie).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(canvas).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(field).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(editor).IsArrangeProducerPure());

  // The two that must never declare themselves pure. Their impurity is the DEFAULT,
  // so this also states that nothing in their construction path accidentally opts in.
  VideoView video = VideoView::New(CreateTestVideoSource());
  WebView   web   = WebView::New();

  DALI_TEST_CHECK(!DataOf(video).IsArrangeProducerPure());
  DALI_TEST_CHECK(!DataOf(web).IsArrangeProducerPure());

  // --- Part 2: the bit is honoured by the hit predicate ----------------------
  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(200.0f);
  application.GetScene().Add(root);

  // Two THIRD-PARTY-shaped subclasses with a byte-identical OnArrange override,
  // differing only in whether their factory declared it pure. The undeclared one
  // is what an author who never read the documentation writes.
  View declaredLeaf   = CreatePurityCounterView(true);
  View undeclaredLeaf = CreatePurityCounterView(false);

  DALI_TEST_CHECK(DataOf(declaredLeaf).IsArrangeProducerPure());
  DALI_TEST_CHECK(!DataOf(undeclaredLeaf).IsArrangeProducerPure());

  declaredLeaf.SetRequestedWidth(50.0f);
  declaredLeaf.SetRequestedHeight(40.0f);
  root.Add(declaredLeaf);

  undeclaredLeaf.SetRequestedWidth(50.0f);
  undeclaredLeaf.SetRequestedHeight(40.0f);
  root.Add(undeclaredLeaf);

  Settle(application);

  // ViewImpl::OnArrange echoes its input for a childless view, so the settled actor
  // geometry IS the slot each cache was keyed on.
  const LayoutRect declaredSlot   = SettledSlotOf(declaredLeaf);
  const LayoutRect undeclaredSlot = SettledSlotOf(undeclaredLeaf);

  // BOTH leaves settled with a live cache entry: being undeclared declines the HIT,
  // not the publish, so this is a genuine "entry exists but is refused" comparison
  // and not merely "the impure leaf never cached".
  DALI_TEST_CHECK(DataOf(declaredLeaf).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(undeclaredLeaf).IsArrangeCacheValid());

  const int declaredBase   = PurityCounterImplOf(declaredLeaf).GetArrangeCallCount();
  const int undeclaredBase = PurityCounterImplOf(undeclaredLeaf).GetArrangeCallCount();
  DALI_TEST_CHECK(declaredBase > 0);
  DALI_TEST_CHECK(undeclaredBase > 0);

  const int PASSES = 3;
  for(int pass = 0; pass < PASSES; ++pass)
  {
    declaredLeaf.Arrange(declaredSlot);
    undeclaredLeaf.Arrange(undeclaredSlot);
  }

  // The declared-pure leaf is served from cache on every one of those passes.
  DALI_TEST_EQUALS(PurityCounterImplOf(declaredLeaf).GetArrangeCallCount(), declaredBase, TEST_LOCATION);

  // The undeclared leaf re-runs its producer on every single pass, with the same
  // slot, the same direction, the same scale and no dirty bit -- the ONLY term
  // rejecting it is mArrangeProducerPure.
  DALI_TEST_EQUALS(PurityCounterImplOf(undeclaredLeaf).GetArrangeCallCount(), undeclaredBase + PASSES, TEST_LOCATION);

  // Always-miss must still be result-identical: refusing the hit costs work, never
  // correctness.
  DALI_TEST_EQUALS(undeclaredLeaf.GetProperty<float>(Actor::Property::POSITION_X), undeclaredSlot.x, TEST_LOCATION);
  DALI_TEST_EQUALS(undeclaredLeaf.GetProperty<float>(Actor::Property::POSITION_Y), undeclaredSlot.y, TEST_LOCATION);
  DALI_TEST_EQUALS(undeclaredLeaf.GetProperty<float>(Actor::Property::SIZE_WIDTH), undeclaredSlot.width, TEST_LOCATION);
  DALI_TEST_EQUALS(undeclaredLeaf.GetProperty<float>(Actor::Property::SIZE_HEIGHT), undeclaredSlot.height, TEST_LOCATION);

  // Nothing above re-declared anything, so the two states are stable -- including
  // Video/Web, which nothing in a layout pass can flip.
  DALI_TEST_CHECK(!DataOf(video).IsArrangeProducerPure());
  DALI_TEST_CHECK(!DataOf(web).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(plain).IsArrangeProducerPure());

  END_TEST;
}

// mArrangeProducerPure is DERIVED, not stored: it answers "is the producer that
// would actually run declared pure?", so it has to be recomputed whenever the ACTIVE
// producer changes. RefreshArrangeProducerPurity() mirrors the dispatch order in
// ViewDataImpl::Arrange -- ArrangeCallback > LayoutManager > OnArrange -- and this
// test walks every transition in that order, in both directions where one exists.
//
// The interesting case is the last pair: a view whose OnArrange was declared PURE
// must go IMPURE the moment an undeclared callback takes over as its producer, and
// come back when the callback is removed. Getting that wrong is not a missed
// optimisation; it is a third-party callback being served from cache on the strength
// of a declaration that was made about a different function.
//
// Non-vacuity (verified by mutation): dropping the RefreshArrangeProducerPurity()
// call from any one mutation point leaves that transition's assertion reading the
// previous producer's purity and the corresponding check fails.
int UtcDaliArrangeCacheProducerPurityFollowsActiveProducerP(void)
{
  UiTestApplication application;
  tet_infoline("The derived purity bit tracks whichever arrange producer is currently active");

  gCallbackArrangeCount = 0;

  // A plain View: no callback, no manager, so the producer is ViewImpl::OnArrange,
  // declared PURE in ViewImpl::New().
  View view = View::New();
  DALI_TEST_CHECK(DataOf(view).IsArrangeProducerPure());

  // An undeclared callback outranks OnArrange and drags the view impure, even though
  // the OnArrange declaration is untouched underneath.
  view.SetArrangeCallback(ArrangeCallback::New(&CountingCallbackArrange));
  DALI_TEST_CHECK(!DataOf(view).IsArrangeProducerPure());

  // A callback declared PURE is served.
  view.SetArrangeCallback(ArrangeCallback::New(&CountingCallbackArrange), ArrangePurity::PURE);
  DALI_TEST_CHECK(DataOf(view).IsArrangeProducerPure());

  // Re-installing through the one-argument overload CLEARS the declared purity: the
  // declaration belongs to the callback that was installed with it, not to the view.
  view.SetArrangeCallback(ArrangeCallback::New(&CountingCallbackArrange));
  DALI_TEST_CHECK(!DataOf(view).IsArrangeProducerPure());

  // Removing the callback hands the producer role back to OnArrange, whose own
  // declaration (PURE, from ViewImpl::New) applies again.
  view.SetArrangeCallback({});
  DALI_TEST_CHECK(DataOf(view).IsArrangeProducerPure());

  // An explicit re-declaration of the OnArrange purity is honoured immediately. This
  // goes through a subclass because SetArrangePurity is protected: it is a statement
  // a view makes about its own override, which is what keeps a caller from declaring
  // someone else's producer pure.
  View redeclarable = CreatePurityCounterView(true);
  DALI_TEST_CHECK(DataOf(redeclarable).IsArrangeProducerPure());

  PurityCounterImplOf(redeclarable).Declare(ArrangePurity::IMPURE);
  DALI_TEST_CHECK(!DataOf(redeclarable).IsArrangeProducerPure());

  // ...and is not a one-way latch, which is what lets a view that learns it is impure
  // only after construction (WebView acquiring its engine, say) correct itself.
  PurityCounterImplOf(redeclarable).Declare(ArrangePurity::PURE);
  DALI_TEST_CHECK(DataOf(redeclarable).IsArrangeProducerPure());

  // An installed callback still outranks the OnArrange declaration underneath it.
  redeclarable.SetArrangeCallback(ArrangeCallback::New(&CountingCallbackArrange));
  DALI_TEST_CHECK(!DataOf(redeclarable).IsArrangeProducerPure());
  redeclarable.SetArrangeCallback({});
  DALI_TEST_CHECK(DataOf(redeclarable).IsArrangeProducerPure());

  // A LayoutManager outranks OnArrange, and the declaration that counts is then the
  // MANAGER's own: StackLayoutManager declares its Arrange PURE, so attaching it does
  // not drag the view impure.
  View managed = View::New();
  DALI_TEST_CHECK(DataOf(managed).IsArrangeProducerPure());
  managed.AttachLayoutManager(Dali::MakeUnique<StackLayoutManager>(StackOrientation::VERTICAL, 0.0f));
  DALI_TEST_CHECK(DataOf(managed).IsArrangeProducerPure());

  // A callback outranks the manager, so an UNDECLARED callback on a managed view drags
  // it impure -- the bit follows DISPATCH order, not "most optimistic wins".
  managed.SetArrangeCallback(ArrangeCallback::New(&CountingCallbackArrange));
  DALI_TEST_CHECK(!DataOf(managed).IsArrangeProducerPure());

  // A PURE callback is pure again...
  managed.SetArrangeCallback(ArrangeCallback::New(&CountingCallbackArrange), ArrangePurity::PURE);
  DALI_TEST_CHECK(DataOf(managed).IsArrangeProducerPure());

  // ...and removing it falls back to the manager's own declaration.
  managed.SetArrangeCallback({});
  DALI_TEST_CHECK(DataOf(managed).IsArrangeProducerPure());

  // The same walk with a manager that declares NOTHING: ScrollViewLayoutManager reads
  // the scrolled child's live actor position, so it must never be skipped, and it is
  // the manager -- not the view -- that says so.
  View scrollManaged = View::New();
  DALI_TEST_CHECK(DataOf(scrollManaged).IsArrangeProducerPure());
  scrollManaged.AttachLayoutManager(Dali::MakeUnique<ScrollViewLayoutManager>());
  DALI_TEST_CHECK(!DataOf(scrollManaged).IsArrangeProducerPure());

  // A PURE callback outranks even an impure manager, for the same dispatch-order reason.
  scrollManaged.SetArrangeCallback(ArrangeCallback::New(&CountingCallbackArrange), ArrangePurity::PURE);
  DALI_TEST_CHECK(DataOf(scrollManaged).IsArrangeProducerPure());
  scrollManaged.SetArrangeCallback({});
  DALI_TEST_CHECK(!DataOf(scrollManaged).IsArrangeProducerPure());

  END_TEST;
}

// THE third-party inheritance guarantee: purity is per EXACT TYPE, never inherited.
//
// A first-party view that declares its OnArrange PURE is, by construction, also a
// public non-final base -- LabelImpl, ImageViewImpl and the rest are all DALI_UI_API
// and subclassable. So the declaration has to be made somewhere a subclass cannot pick
// it up, and the only such place is the type's own New() factory: a subclass builds
// itself through ITS factory and never runs the base's.
//
// A constructor is NOT such a place, and that is exactly what this test forbids. A base
// constructor runs for every subclass, so a declaration made there would hand every
// third-party subclass mArrangeOverridePure == true without its author writing a single
// line -- and an impure override (world geometry, a native sink) would then be silently
// replaced by a stale cached rect. Default-IMPURE for third parties is the guarantee;
// this is its regression test.
//
// Non-vacuity: the `declarePure == true` half proves this exact class SHAPE is
// cacheable, so the undeclared half's always-miss is attributable to the declaration
// and to nothing else.
//
// This case FAILS on the pre-fix code (verified by moving the declaration back into
// LabelImpl's constructor and rebuilding): the undeclared probe reports
// IsArrangeProducerPure() == true and its producer stops running.
int UtcDaliArrangeCacheHitThirdPartySubclassDoesNotInheritPurityP(void)
{
  UiTestApplication application;
  tet_infoline("A third-party subclass of a PURE first-party view does not inherit the declaration");

  // --- Part 1: the base IS pure, the undeclared subclass is NOT -----------------
  Label base = Label::New();
  DALI_TEST_CHECK(DataOf(base).IsArrangeProducerPure());

  View undeclared = CreateProbeLabel(false);
  View declared   = CreateProbeLabel(true);

  // The whole point: same base class, same base declaration, and the subclass that
  // said nothing is IMPURE.
  DALI_TEST_CHECK(!DataOf(undeclared).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(declared).IsArrangeProducerPure());

  // The base's own declaration is untouched by any of this.
  DALI_TEST_CHECK(DataOf(base).IsArrangeProducerPure());

  // --- Part 2: the undeclared subclass really does re-run its producer ----------
  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(200.0f);
  application.GetScene().Add(root);

  undeclared.SetRequestedWidth(50.0f);
  undeclared.SetRequestedHeight(40.0f);
  root.Add(undeclared);

  declared.SetRequestedWidth(50.0f);
  declared.SetRequestedHeight(40.0f);
  root.Add(declared);

  Settle(application);

  DALI_TEST_CHECK(!DataOf(undeclared).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(declared).IsArrangeProducerPure());

  // Both published an entry, so this is "the entry exists and is REFUSED", not "the
  // impure one never cached".
  DALI_TEST_CHECK(DataOf(undeclared).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(declared).IsArrangeCacheValid());

  const LayoutRect undeclaredSlot = SettledSlotOf(undeclared);
  const LayoutRect declaredSlot   = SettledSlotOf(declared);

  const int undeclaredBase = ProbeLabelImplOf(undeclared).GetArrangeCallCount();
  const int declaredBase   = ProbeLabelImplOf(declared).GetArrangeCallCount();
  DALI_TEST_CHECK(undeclaredBase > 0);
  DALI_TEST_CHECK(declaredBase > 0);

  const int PASSES = 3;
  for(int pass = 0; pass < PASSES; ++pass)
  {
    undeclared.Arrange(undeclaredSlot);
    declared.Arrange(declaredSlot);
  }

  // Every settled pass re-runs the undeclared override -- it is never served.
  DALI_TEST_EQUALS(ProbeLabelImplOf(undeclared).GetArrangeCallCount(), undeclaredBase + PASSES, TEST_LOCATION);

  // ...while the opted-in sibling, identical in every other respect, is.
  DALI_TEST_EQUALS(ProbeLabelImplOf(declared).GetArrangeCallCount(), declaredBase, TEST_LOCATION);

  // Always-miss stays result-identical: refusing the hit costs work, never geometry.
  DALI_TEST_EQUALS(undeclared.GetProperty<float>(Actor::Property::POSITION_X), undeclaredSlot.x, TEST_LOCATION);
  DALI_TEST_EQUALS(undeclared.GetProperty<float>(Actor::Property::POSITION_Y), undeclaredSlot.y, TEST_LOCATION);
  DALI_TEST_EQUALS(undeclared.GetProperty<float>(Actor::Property::SIZE_WIDTH), undeclaredSlot.width, TEST_LOCATION);
  DALI_TEST_EQUALS(undeclared.GetProperty<float>(Actor::Property::SIZE_HEIGHT), undeclaredSlot.height, TEST_LOCATION);

  END_TEST;
}

// Adding a STANDALONE child must retract the parent's arrange cache entry.
//
// The entry was published for a child set that did not include this child, and the
// only place the new child gets placed is ArrangeStandaloneChildren -- which lives on
// the arrange path, below the cache-HIT return. So a live entry across the add is an
// entry that can be served instead of placing the child.
//
// The predicate's !HasUnconsumedStandaloneChild() term does NOT cover this:
// mMeasuredSlotUnconsumed is initialised false and raised only at a measure publish,
// so a freshly added, never-measured standalone child leaves the term FALSE and the
// hit is not declined. ViewDataImpl::OnChildAdded's InvalidateArrange() on the
// standalone path is the guard, and this is its direct statement.
//
// WHITE-BOX on purpose: while the hit is childless-only, the parent stops being
// childless the moment the child is added, so it misses for that reason alone and the
// gap has no black-box symptom. The bookkeeping is the only place the fix is visible.
//
// Non-vacuity (verified by mutation): removing the InvalidateArrange() from
// OnChildAdded's standalone branch leaves the entry valid and both post-add
// assertions fail.
int UtcDaliArrangeCacheStandaloneChildAddInvalidatesParentArrangeP(void)
{
  UiTestApplication application;
  tet_infoline("Adding a standalone child drops the parent's published arrange entry");

  View parent = View::New();
  parent.SetRequestedWidth(200.0f);
  parent.SetRequestedHeight(100.0f);
  application.GetScene().Add(parent);

  View regular = View::New();
  regular.SetRequestedWidth(50.0f);
  regular.SetRequestedHeight(40.0f);
  parent.Add(regular);

  Settle(application);

  // The state the gap needs: a live entry, nothing dirty.
  DALI_TEST_CHECK(DataOf(parent).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(parent).IsArrangeDirty());

  // A never-measured standalone child, so mMeasuredSlotUnconsumed is still false and
  // !HasUnconsumedStandaloneChild() would NOT reject the hit.
  View standalone = View::New();
  standalone.SetLayoutMode(LayoutMode::STANDALONE);
  standalone.SetRequestedX(10.0f);
  standalone.SetRequestedY(20.0f);
  standalone.SetRequestedWidth(30.0f);
  standalone.SetRequestedHeight(25.0f);
  parent.Add(standalone);

  // The add itself is the whole event: no pass has run yet.
  DALI_TEST_CHECK(!DataOf(parent).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(parent).IsArrangeDirty());

  // ...and it recovers: the next pass places the child and republishes.
  Settle(application);
  DALI_TEST_CHECK(DataOf(parent).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(parent).IsArrangeDirty());

  END_TEST;
}

// The `if(mArrangeCacheValid)` TRUE branch of ViewDataImpl::SetArrangePurity, which no
// other case reaches: every first-party declaration is made from a New() factory, where
// the bit is false by construction, so only a RE-declaration on an already settled view
// runs the invalidation.
//
// It has to: the entry on a settled view was published while the OLD declaration was in
// force. Leaving it live across a PURE -> IMPURE re-declaration would let the very next
// identical pass serve a rect the view has just said may not be reused.
//
// Non-vacuity (verified by mutation): removing the `if(mArrangeCacheValid)
// InvalidateArrange();` block leaves the entry valid and both post-declaration
// assertions fail.
int UtcDaliArrangeCachePurityRedeclarationInvalidatesSettledEntryP(void)
{
  UiTestApplication application;
  tet_infoline("Re-declaring purity on a settled view drops its published arrange entry");

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(100.0f);
  application.GetScene().Add(root);

  View leaf = CreatePurityCounterView(true);
  leaf.SetRequestedWidth(50.0f);
  leaf.SetRequestedHeight(40.0f);
  root.Add(leaf);

  Settle(application);

  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeDirty());
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeProducerPure());

  // PURE -> IMPURE on a live entry: the entry must go, and the view must be scheduled.
  PurityCounterImplOf(leaf).Declare(ArrangePurity::IMPURE);
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeProducerPure());
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeDirty());

  // The reverse direction, from a freshly settled state, takes the same branch: the
  // entry published while IMPURE was never a hit candidate, but the guard does not
  // reason about that and drops it anyway.
  Settle(application);
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());

  PurityCounterImplOf(leaf).Declare(ArrangePurity::PURE);
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeProducerPure());
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeCacheValid());

  // ...and it recovers: a re-settle republishes under the new declaration.
  Settle(application);
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());

  END_TEST;
}

// The derived purity bit must not survive an EXTERNAL swap of the traits that carry the
// producer. ArrangeCallback (ReservedTraitId::LAYOUT_SIGNALS) and LayoutManager
// (ReservedTraitId::LAYOUT_MANAGER) both outrank OnArrange in Arrange()'s dispatch
// order, and both are reachable through the public Integration::View::SetTrait /
// RemoveTrait surface, which does not go through SetArrangeCallback() /
// AttachLayoutManager() and so does no purity bookkeeping of its own.
//
// The case below is the one that actually desyncs: a view whose OWN OnArrange is
// undeclared (IMPURE) is made pure only by a PURE ArrangeCallback sitting on top of it.
// Rip that callback out through the trait API and the producer falls back to the
// undeclared OnArrange -- if the derived bit were left stale-TRUE, that impure override
// would be served from the entry the callback published.
//
// Non-vacuity (verified by mutation): removing the OnArrangeProducerTraitChanged() call
// from ViewDataImpl::RemoveTrait leaves the bit TRUE and the entry live, and both
// post-removal assertions fail.
int UtcDaliArrangeCacheProducerPurityClearedByExternalTraitRemovalP(void)
{
  UiTestApplication application;
  tet_infoline("An external reserved-trait removal re-derives the arrange producer purity");

  gCallbackArrangeCount = 0;

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(100.0f);
  application.GetScene().Add(root);

  // OnArrange undeclared: this view is IMPURE on its own.
  View leaf = CreatePurityCounterView(false);
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeProducerPure());

  // A PURE callback outranks it and is the ONLY reason the view is pure at all.
  leaf.SetArrangeCallback(ArrangeCallback::New(&CountingCallbackArrange), ArrangePurity::PURE);
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeProducerPure());

  leaf.SetRequestedWidth(50.0f);
  leaf.SetRequestedHeight(40.0f);
  root.Add(leaf);

  Settle(application);

  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeProducerPure());

  // The external swap. Nothing else in the library takes this path.
  DALI_TEST_CHECK(IntegrationView::RemoveTrait(GetImpl(leaf), ReservedTraitId::LAYOUT_SIGNALS));

  // The producer is the undeclared OnArrange again, so the bit must be FALSE...
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeProducerPure());
  // ...and the entry the callback published must not outlive it.
  DALI_TEST_CHECK(!DataOf(leaf).IsArrangeCacheValid());

  // Behaviourally: the override now runs on every settled pass.
  Settle(application);

  const LayoutRect leafSlot = SettledSlotOf(leaf);
  const int        callBase = PurityCounterImplOf(leaf).GetArrangeCallCount();

  const int PASSES = 3;
  for(int pass = 0; pass < PASSES; ++pass)
  {
    leaf.Arrange(leafSlot);
  }
  DALI_TEST_EQUALS(PurityCounterImplOf(leaf).GetArrangeCallCount(), callBase + PASSES, TEST_LOCATION);

  END_TEST;
}

// ---------------------------------------------------------------------------
// Phase 5b: white-box coverage for the SUBTREE hit.
//
// The black-box suite observes the subtree replay through counting container
// producers. What it cannot see is the bookkeeping the replay leaves behind, the
// per-node DEBUG invariant it asserts, and the LayoutFinished registration it makes
// for descendants. Those three are pinned here.
// ---------------------------------------------------------------------------

// A subtree hit consumes nothing: every node's entry is still valid afterwards, so the
// NEXT identical pass hits as well. This is the subtree form of
// UtcDaliArrangeCacheHitStaysValidAcrossAHitP, and it only holds because the replay
// returns BEFORE ArrangePassGuard at every level -- the guard's constructor would clear
// mArrangeCacheValid, and the replay never constructs one.
//
// Non-vacuity (verified by mutation): clearing mArrangeCacheValid anywhere in
// ReplayArrangeSubtreeFromCache leaves the entries dead after the first hit and the
// post-hit assertions fail.
int UtcDaliArrangeCacheSubtreeHitLeavesEveryEntryValidP(void)
{
  UiTestApplication application;
  tet_infoline("A subtree served from cache still holds a valid entry at every node");

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(100.0f);
  application.GetScene().Add(root);

  View mid = View::New();
  mid.SetRequestedWidth(120.0f);
  mid.SetRequestedHeight(60.0f);
  root.Add(mid);

  View leaf = View::New();
  leaf.SetRequestedWidth(50.0f);
  leaf.SetRequestedHeight(40.0f);
  mid.Add(leaf);

  Settle(application);

  DALI_TEST_CHECK(DataOf(root).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(mid).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());

  const LayoutRect rootSlot = SettledSlotOf(root);

  for(int pass = 0; pass < 3; ++pass)
  {
    root.Arrange(rootSlot);

    DALI_TEST_CHECK(DataOf(root).IsArrangeCacheValid());
    DALI_TEST_CHECK(DataOf(mid).IsArrangeCacheValid());
    DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid());

    DALI_TEST_CHECK(!DataOf(root).IsArrangeDirty());
    DALI_TEST_CHECK(!DataOf(mid).IsArrangeDirty());
    DALI_TEST_CHECK(!DataOf(leaf).IsArrangeDirty());
  }

  END_TEST;
}

// LayoutFinished is pass-based, not work-based, and that has to survive the replay at
// every DEPTH: a subscriber on a GRANDCHILD is telling the framework "notify me when
// this view was arranged in a pass", and being replayed from cache IS being arranged.
// Dropping the per-node registration would truncate the controller's arrangedViews set
// to whatever happened to miss.
//
// The counting container is what makes this non-vacuous: it proves the pass that fired
// the signal was a HIT, not a miss that fired it for the ordinary reason.
//
// Non-vacuity (verified by mutation): removing the
// `if(HasLayoutFinishedSignalConnections()) NotifyViewArranged(...)` block from
// ReplayArrangeSubtreeFromCache stops the grandchild emitting on hitting passes.
int UtcDaliArrangeCacheSubtreeHitEmitsLayoutFinishedForDescendantsP(void)
{
  UiTestApplication application;
  tet_infoline("A grandchild still emits LayoutFinished on a pass its whole subtree serves from cache");

  gLayoutFinishedCount = 0;

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(100.0f);
  application.GetScene().Add(root);

  // A counting container so the pass can be proven to be a hit.
  View mid = CreatePurityCounterView(true);
  mid.SetRequestedWidth(120.0f);
  mid.SetRequestedHeight(60.0f);
  root.Add(mid);

  View grandchild = View::New();
  grandchild.SetRequestedWidth(50.0f);
  grandchild.SetRequestedHeight(40.0f);
  mid.Add(grandchild);

  // The reason a pass happens at all, and it is unrelated to the mid/grandchild chain.
  View sibling = View::New();
  sibling.SetRequestedWidth(30.0f);
  sibling.SetRequestedHeight(30.0f);
  root.Add(sibling);

  grandchild.LayoutFinishedSignal().Connect(&OnLayoutFinished);

  Settle(application);

  const int settledEmits = gLayoutFinishedCount;
  const int midBase      = PurityCounterImplOf(mid).GetArrangeCallCount();
  DALI_TEST_CHECK(settledEmits > 0);
  DALI_TEST_CHECK(midBase > 0);
  DALI_TEST_CHECK(DataOf(mid).IsArrangeCacheValid());

  sibling.SetRequestedX(11.0f);
  Settle(application);

  // The mid/grandchild subtree was served from cache...
  DALI_TEST_EQUALS(PurityCounterImplOf(mid).GetArrangeCallCount(), midBase, TEST_LOCATION);
  DALI_TEST_CHECK(DataOf(mid).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(grandchild).IsArrangeCacheValid());

  // ...and the grandchild still registered as arranged in that pass.
  DALI_TEST_CHECK(gLayoutFinishedCount > settledEmits);

  END_TEST;
}

// Corollary C, per node. The replay skips the GetEffectiveScale() that the miss path
// performs at every level, so a whole SUBTREE now rests on "a valid arrange cache
// implies a valid logical context" rather than a single node doing so. The replay
// asserts mLogicalContextValid at every node it visits (DEBUG); this states the same
// claim as a normal assertion, so it is checked in every build configuration, and then
// pins the pairing that makes it true.
//
// Non-vacuity (verified by mutation): removing the InvalidateLayoutCaches() call from
// InvalidateLogicalContextRecursive leaves the subtree's arrange entries live after the
// context was dropped, which the second half below catches directly (and which would
// make the replay's DEBUG assert fire on the next identical pass).
int UtcDaliArrangeCacheSubtreeHitAssertsLogicalContextP(void)
{
  UiTestApplication application;
  tet_infoline("A subtree-wide arrange cache implies a subtree-wide effective-scale sync bit");

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(100.0f);
  application.GetScene().Add(root);

  View mid = View::New();
  mid.SetRequestedWidth(120.0f);
  mid.SetRequestedHeight(60.0f);
  root.Add(mid);

  View leaf = View::New();
  leaf.SetRequestedWidth(50.0f);
  leaf.SetRequestedHeight(40.0f);
  mid.Add(leaf);

  Settle(application);

  // The state the replay's per-node DEBUG assert relies on, at every node.
  DALI_TEST_CHECK(DataOf(root).IsArrangeCacheValid() && DataOf(root).IsLogicalContextValid());
  DALI_TEST_CHECK(DataOf(mid).IsArrangeCacheValid() && DataOf(mid).IsLogicalContextValid());
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid() && DataOf(leaf).IsLogicalContextValid());

  // Takes the subtree hit. In a DEBUG build the per-node asserts inside it are the live
  // check; in any build the state assertions around it hold.
  root.Arrange(SettledSlotOf(root));

  DALI_TEST_CHECK(DataOf(root).IsLogicalContextValid());
  DALI_TEST_CHECK(DataOf(mid).IsLogicalContextValid());
  DALI_TEST_CHECK(DataOf(leaf).IsLogicalContextValid());

  // The pairing, in its recursive form -- the one a global UI scale change uses. It
  // must drop BOTH bits for EVERY node, or a descendant could be replayed against a
  // stale scale with no term in the gate to catch it.
  DataOf(root).InvalidateLogicalContextRecursive();

  DALI_TEST_CHECK(!DataOf(root).IsLogicalContextValid() && !DataOf(root).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(mid).IsLogicalContextValid() && !DataOf(mid).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(leaf).IsLogicalContextValid() && !DataOf(leaf).IsArrangeCacheValid());

  // ...and it recovers. The recursive drop is a CACHE-ONLY invalidation -- it raises
  // no dirty bit and registers no layout root, deliberately, so it cannot spin -- which
  // means a pass has to be asked for before the subtree can republish.
  root.InvalidateMeasure();
  Settle(application);
  DALI_TEST_CHECK(DataOf(root).IsArrangeCacheValid() && DataOf(root).IsLogicalContextValid());
  DALI_TEST_CHECK(DataOf(mid).IsArrangeCacheValid() && DataOf(mid).IsLogicalContextValid());
  DALI_TEST_CHECK(DataOf(leaf).IsArrangeCacheValid() && DataOf(leaf).IsLogicalContextValid());

  END_TEST;
}

// I4 at depth: the corrective re-measure for an unconsumed standalone slot lives on the
// ARRANGE path (ArrangeStandaloneChildren), and !HasUnconsumedStandaloneChild() is
// O(direct children) -- so a standalone GRANDCHILD holding one is invisible to the
// node-local predicate at the root. The subtree gate re-evaluates the term at every
// node, which is what keeps the correction reachable.
//
// The fixture is the state a plain settle leaves behind, and it is the ONLY state in
// which this term decides anything. A standalone view is its own layout root, so the
// settle batch drives it AFTER its parent (depth-sorted): its measure publish marks the
// slot unconsumed, and its own arrange republishes a live entry. So `standalone` ends
// the settle with a VALID arrange cache AND an unconsumed slot, and nothing else in the
// subtree can refuse the hit.
//
// An out-of-band Measure() would raise the bit too, but it is NOT a sharp fixture: the
// measure pass clears that view's own arrange cache in the same breath, so the gate
// would reject on mArrangeCacheValid and the term under test would decide nothing.
//
// Non-vacuity (verified by mutation): dropping
// `!childData.HasUnconsumedStandaloneChild()` from CanReplayArrangeSubtreeFromCache
// lets the ROOT hit -- and then mid's Arrange is never called at all, so its own
// node-local copy of the term never gets a chance to refuse. The mid producer count
// stays flat and ArrangeStandaloneChildren never runs.
int UtcDaliArrangeCacheSubtreeGateRejectsUnconsumedStandaloneDescendantP(void)
{
  UiTestApplication application;
  tet_infoline("An unconsumed standalone slot on a grandchild refuses the whole subtree hit");

  View root = View::New();
  root.SetRequestedWidth(200.0f);
  root.SetRequestedHeight(100.0f);
  application.GetScene().Add(root);

  View mid = CreatePurityCounterView(true);
  mid.SetRequestedWidth(120.0f);
  mid.SetRequestedHeight(60.0f);
  root.Add(mid);

  View standalone = View::New();
  standalone.SetLayoutMode(LayoutMode::STANDALONE);
  standalone.SetRequestedWidth(30.0f);
  standalone.SetRequestedHeight(25.0f);
  mid.Add(standalone);

  Settle(application);

  const LayoutRect rootSlot = SettledSlotOf(root);
  const int        midBase  = PurityCounterImplOf(mid).GetArrangeCallCount();
  DALI_TEST_CHECK(midBase > 0);

  // The precondition, stated explicitly: EVERY other term the gate could reject on is
  // satisfied at every node. The standalone grandchild in particular carries a live
  // entry of its own, so `mid` holding an unconsumed slot for it is the only refusal
  // available.
  DALI_TEST_CHECK(DataOf(root).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(mid).IsArrangeCacheValid());
  DALI_TEST_CHECK(DataOf(standalone).IsArrangeCacheValid());
  DALI_TEST_CHECK(!DataOf(mid).IsArrangeDirty());
  DALI_TEST_CHECK(!DataOf(standalone).IsArrangeDirty());

  // The gate must refuse, so mid is arranged and ArrangeStandaloneChildren is reached.
  root.Arrange(rootSlot);
  DALI_TEST_EQUALS(PurityCounterImplOf(mid).GetArrangeCallCount(), midBase + 1, TEST_LOCATION);

  // That pass consumed the slot, so the refusal costs exactly one pass -- and this is
  // also the control that makes the miss above non-vacuous: everything else about this
  // subtree is cacheable.
  root.Arrange(rootSlot);
  DALI_TEST_EQUALS(PurityCounterImplOf(mid).GetArrangeCallCount(), midBase + 1, TEST_LOCATION);
  root.Arrange(rootSlot);
  DALI_TEST_EQUALS(PurityCounterImplOf(mid).GetArrangeCallCount(), midBase + 1, TEST_LOCATION);

  // ...and the correction landed: the standalone child sits at the size its parent's
  // extent gives it.
  DALI_TEST_EQUALS(standalone.GetProperty<float>(Actor::Property::SIZE_WIDTH), 30.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(standalone.GetProperty<float>(Actor::Property::SIZE_HEIGHT), 25.0f, TEST_LOCATION);

  END_TEST;
}

// ---------------------------------------------------------------------------
// Phase 5c: the LayoutManager purity declaration.
// ---------------------------------------------------------------------------

// THE WIN, counted. A settled container whose producer is a declared-PURE LayoutManager
// runs NO manager Arrange when a layout pass sweeps past it for an unrelated reason,
// and the geometry it leaves behind is byte-identical. The same container with an
// UNDECLARED manager re-runs on every pass, which is what the whole in-library
// container population did before this increment.
//
// Non-vacuity (verified by mutation): dropping the DeclareArrangePurity call from
// CountingLayoutManager's constructor makes the pure half behave like the impure half
// and the flat-count assertion fails.
int UtcDaliArrangeCachePureLayoutManagerContainerSkipsProducerP(void)
{
  UiTestApplication application;
  tet_infoline("A declared-PURE LayoutManager is not re-run for a settled container");

  for(int declarePure = 1; declarePure >= 0; --declarePure)
  {
    gManagerArrangeCount = 0;

    View root = View::New();
    root.SetRequestedWidth(200.0f);
    root.SetRequestedHeight(200.0f);
    application.GetScene().Add(root);

    View container = View::New();
    container.SetRequestedWidth(120.0f);
    container.SetRequestedHeight(120.0f);
    container.AttachLayoutManager(Dali::UniquePtr<LayoutManager>(new CountingLayoutManager(declarePure != 0)));
    root.Add(container);

    View first = View::New();
    first.SetRequestedWidth(50.0f);
    first.SetRequestedHeight(30.0f);
    container.Add(first);

    View second = View::New();
    second.SetRequestedWidth(50.0f);
    second.SetRequestedHeight(30.0f);
    container.Add(second);

    View sibling = View::New();
    sibling.SetRequestedWidth(30.0f);
    sibling.SetRequestedHeight(30.0f);
    root.Add(sibling);

    Settle(application);

    // The declaration reached the derived bit.
    DALI_TEST_CHECK(DataOf(container).IsArrangeProducerPure() == (declarePure != 0));

    const int settledCount = gManagerArrangeCount;
    DALI_TEST_CHECK(settledCount > 0);

    const LayoutRect firstRect(first.GetProperty<float>(Actor::Property::POSITION_X),
                               first.GetProperty<float>(Actor::Property::POSITION_Y),
                               first.GetProperty<float>(Actor::Property::SIZE_WIDTH),
                               first.GetProperty<float>(Actor::Property::SIZE_HEIGHT));
    const LayoutRect secondRect(second.GetProperty<float>(Actor::Property::POSITION_X),
                                second.GetProperty<float>(Actor::Property::POSITION_Y),
                                second.GetProperty<float>(Actor::Property::SIZE_WIDTH),
                                second.GetProperty<float>(Actor::Property::SIZE_HEIGHT));
    DALI_TEST_EQUALS(secondRect.y, 30.0f, TEST_LOCATION);

    // A pass that has nothing to do with the container.
    sibling.SetRequestedX(11.0f);
    Settle(application);
    DALI_TEST_EQUALS(sibling.GetProperty<float>(Actor::Property::POSITION_X), 11.0f, TEST_LOCATION);

    if(declarePure != 0)
    {
      DALI_TEST_EQUALS(gManagerArrangeCount, settledCount, TEST_LOCATION);
    }
    else
    {
      DALI_TEST_CHECK(gManagerArrangeCount > settledCount);
    }

    // Either way the result is identical -- serving the cache optimises the WORK.
    DALI_TEST_EQUALS(first.GetProperty<float>(Actor::Property::POSITION_Y), firstRect.y, TEST_LOCATION);
    DALI_TEST_EQUALS(first.GetProperty<float>(Actor::Property::SIZE_HEIGHT), firstRect.height, TEST_LOCATION);
    DALI_TEST_EQUALS(second.GetProperty<float>(Actor::Property::POSITION_Y), secondRect.y, TEST_LOCATION);
    DALI_TEST_EQUALS(second.GetProperty<float>(Actor::Property::SIZE_HEIGHT), secondRect.height, TEST_LOCATION);

    application.GetScene().Remove(root);
  }

  END_TEST;
}

// THE third-party inheritance guarantee, for managers. Concrete managers are public,
// non-final classes with public constructors and no factory, so the declaration cannot
// be a plain constructor side effect: it is recorded WITH the declaring type and
// matched against the manager's most-derived type at read time.
//
// A subclass that overrides Arrange with an impure body would otherwise be silently
// skipped in favour of a stale cached rect -- the exact regression that made
// ViewImpl's declarations live in New() rather than in a constructor.
//
// Non-vacuity (verified by mutation): making IsArrangeProducerPure() return the stored
// flag without the type comparison reports the subclassed manager as pure and the
// second half of this test fails.
int UtcDaliArrangeCacheLayoutManagerPurityIsPerExactTypeP(void)
{
  UiTestApplication application;
  tet_infoline("A LayoutManager purity declaration is per exact type and is never inherited");

  // The declaring type itself is pure.
  View exact = View::New();
  exact.AttachLayoutManager(Dali::MakeUnique<StackLayoutManager>(StackOrientation::VERTICAL, 0.0f));
  DALI_TEST_CHECK(DataOf(exact).IsArrangeProducerPure());

  // A subclass of it, declaring nothing, is NOT.
  View subclassed = View::New();
  subclassed.AttachLayoutManager(Dali::UniquePtr<LayoutManager>(new SubclassedStackLayoutManager()));
  DALI_TEST_CHECK(!DataOf(subclassed).IsArrangeProducerPure());

  // ...and the base's own declaration is untouched by any of this.
  View exactAgain = View::New();
  exactAgain.AttachLayoutManager(Dali::MakeUnique<StackLayoutManager>(StackOrientation::HORIZONTAL, 4.0f));
  DALI_TEST_CHECK(DataOf(exactAgain).IsArrangeProducerPure());

  // The same statement through the test manager, whose two variants differ in the
  // declaration and in nothing else.
  View declared = View::New();
  declared.AttachLayoutManager(Dali::UniquePtr<LayoutManager>(new CountingLayoutManager(true)));
  DALI_TEST_CHECK(DataOf(declared).IsArrangeProducerPure());

  View undeclared = View::New();
  undeclared.AttachLayoutManager(Dali::UniquePtr<LayoutManager>(new CountingLayoutManager(false)));
  DALI_TEST_CHECK(!DataOf(undeclared).IsArrangeProducerPure());

  END_TEST;
}

// The declaration, per in-library manager and per in-library container type. This is
// the table the increment is really about: four geometry-free managers declare PURE and
// ScrollView's does not, so a StackLayout / GridLayout / FlexLayout / AbsoluteLayout
// container is cacheable and a ScrollView never is.
//
// Non-vacuity (verified by mutation): removing the DeclareArrangePurity call from any
// one of the four managers flips that manager's row; adding one to
// ScrollViewLayoutManager flips the ScrollView row.
int UtcDaliArrangeCacheInLibraryLayoutManagerPurityP(void)
{
  UiTestApplication application;
  tet_infoline("The four geometry-free layout managers declare PURE; ScrollView's does not");

  // --- the managers, attached to a bare View -----------------------------------
  View stackManaged = View::New();
  stackManaged.AttachLayoutManager(Dali::MakeUnique<StackLayoutManager>(StackOrientation::VERTICAL, 0.0f));
  DALI_TEST_CHECK(DataOf(stackManaged).IsArrangeProducerPure());

  View absoluteManaged = View::New();
  absoluteManaged.AttachLayoutManager(Dali::MakeUnique<AbsoluteLayoutManager>());
  DALI_TEST_CHECK(DataOf(absoluteManaged).IsArrangeProducerPure());

  Dali::Vector<GridLength> rows;
  Dali::Vector<GridLength> columns;
  View                     gridManaged = View::New();
  gridManaged.AttachLayoutManager(Dali::MakeUnique<GridLayoutManager>(rows, columns, 0.0f, 0.0f));
  DALI_TEST_CHECK(DataOf(gridManaged).IsArrangeProducerPure());

  View flexManaged = View::New();
  flexManaged.AttachLayoutManager(Dali::MakeUnique<FlexLayoutManager>(
    FlexDirection::ROW, FlexWrap::NO_WRAP, FlexJustify::FLEX_START, FlexAlign::FLEX_START, FlexAlign::FLEX_START));
  DALI_TEST_CHECK(DataOf(flexManaged).IsArrangeProducerPure());

  // The one exclusion, and the reason 5c is safe: it reads the scrolled child's live
  // actor position (scroll-view-layout-manager.cpp, childBounds.x = child.GetPositionX()).
  View scrollManaged = View::New();
  scrollManaged.AttachLayoutManager(Dali::MakeUnique<ScrollViewLayoutManager>());
  DALI_TEST_CHECK(!DataOf(scrollManaged).IsArrangeProducerPure());

  // --- and through the container types an application actually writes -----------
  DALI_TEST_CHECK(DataOf(StackLayout::New()).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(AbsoluteLayout::New()).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(GridLayout::New()).IsArrangeProducerPure());
  DALI_TEST_CHECK(DataOf(FlexLayout::New()).IsArrangeProducerPure());
  DALI_TEST_CHECK(!DataOf(ScrollView::New()).IsArrangeProducerPure());

  END_TEST;
}
