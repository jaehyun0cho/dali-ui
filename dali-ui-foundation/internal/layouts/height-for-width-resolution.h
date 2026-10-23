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
#include <algorithm>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/layouts/standalone-bounds-utils.h>
#include <dali-ui-foundation/public-api/layouts/layout-types.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include <dali-ui-foundation/public-api/views/view.h>

/**
 * Height-for-width at the width a view is laid out at.
 *
 * WHY. Measure(w, h) offers a view an AT-MOST budget, and the parent decides the final
 * width only afterwards, while arranging: a WRAP_CONTENT container is arranged at its
 * measured width, a FILL / MATCH_PARENT / weighted / cell / flex-shrunk child at whatever
 * the parent's own rule gives it. A child whose height follows its width -- wrapped text,
 * a container holding such a child, a wrapping row of items -- is therefore measured at a
 * width it may not get. Its measured height then belongs to a different width: too short
 * when the final width is narrower (the content is clipped), too tall when it is wider.
 * Measure() cannot fix this by itself, because the width is not final yet at that point.
 *
 * WHAT. Wherever a width BECOMES final, the framework asks for the height the child needs
 * at exactly that width and uses it as the child's slot height:
 *  - a supported container's arrange (IsHeightForWidthContainer), for its children with a
 *    WRAP_CONTENT height. It adopts the resolved heights only when its resolved layout fits
 *    the bounds it was given (HEIGHT_FOR_WIDTH_FIT_EPSILON); otherwise it arranges exactly as
 *    before;
 *  - a layout root's and a STANDALONE view's own slot (ApplyRootHeightForWidth), right
 *    after the shared slot derivation, on every path that derives that slot;
 *  - ScrollView content, with an unbounded budget, so the scrollable height follows.
 * A container whose resolved content does not fit its own height budget is not resolved at
 * all (it reports its measured height): its arrange would refuse that layout, and its box
 * would otherwise be sized for content it does not show. A container sized by its parent from
 * its resolved height therefore passes its own fit test unless the inputs of the two
 * derivations differ: measured sizes of width-independent children rewritten in between by
 * arrange-owned re-measures, the AUTO columns of a GridLayout (resolved from its children's
 * last measured widths, while its own arrange re-measures them first), or the children of a
 * WRAP_CONTENT container nested inside a re-measured MATCH_PARENT container (their widths are
 * not predicted, see WIDTHS). It then arranges its children as before, inside the allocation
 * its parent derived.
 *
 * HOW. ResolveHeightAtWidth() never measures, arranges, publishes or records anything: no
 * Measure() call, no measured size, no cache key and no arrange entry changes, and no
 * layout pass is opened. It reads the subtree's last measured sizes, its layout-tracked
 * state and the text metrics of a Label, and re-derives the height through the same rules
 * the container's own measure and arrange use. It keeps no state of its own and needs no
 * per-view memory. A change to a Label's text or style, or to any layout-tracked property,
 * invalidates every ancestor as before. A measured size rewritten by an arrange-owned
 * re-measure (a MATCH_PARENT child re-measured at its slot) does not invalidate the arranging
 * ancestor -- the owner scope stops that walk -- so a later arrange miss can read it; the
 * result can differ once and then re-converges.
 *
 * COST. An arrange that misses walks, for every child with a WRAP_CONTENT height that is a
 * supported producer, that child's whole supported subtree, because whether it depends on
 * its width is only known after the walk; each visited view costs two trait lookups and a
 * type test, each visited container one temporary copy of its child list (plus per-child
 * buffers when something depends on its width), and each multi-line Label, when it is asked
 * for a width other than the one its text was last laid out at, one more text layout.
 * A ScrollView arranges on every layout pass that reaches its root (ArrangePolicy::ALWAYS),
 * so its content is walked on every such pass, even when nothing in it depends on its width;
 * a WRAP_CONTENT-height layout root resolves once per pass. Follow-up, needing separate
 * approval: a per-view "contains width-dependent content" bit, cleared together with the
 * measure cache, could prune the walk.
 *
 * SUPPORTED PRODUCERS, identified by their exact dynamic type so a subclass that redefines
 * a virtual is never guessed at:
 *  - a multi-line Label without text fit (its height is read from the text metrics);
 *  - a plain View (exactly ViewImpl) that owns no layout manager;
 *  - a view of ANY type whose layout manager is exactly a StackLayoutManager,
 *    GridLayoutManager, FlexLayoutManager or AbsoluteLayoutManager (the manager outranks
 *    the view's own measure and arrange). So StackLayout, GridLayout, FlexLayout and
 *    AbsoluteLayout participate, and so do component views built on those managers (Dialog
 *    and Toast own a StackLayoutManager, DialogContainer and Navigator an
 *    AbsoluteLayoutManager): their wrapped text can grow when they are laid out narrower
 *    than their measure budget.
 * None may carry a MeasureCallback or an ArrangeCallback, and only a WRAP_CONTENT height is
 * resolved. Every other view -- a View subclass that owns no such manager, other view types,
 * custom layout managers (including subclasses of the managers above), views with layout
 * callbacks, ScrollView's own height, text-fit and single-line Labels -- reports its measured
 * height and is not looked into, so whatever lies below it keeps its current layout.
 *
 * DEPENDENCE. A resolved height is reported only when it actually follows the width
 * (HeightAtWidth::dependsOnWidth): a multi-line Label, a supported container with at least
 * one width-dependent child, or a FlexLayoutManager with a horizontal main axis that wraps
 * (its line breaks follow the width). The flag is structural (types and parameters), never a
 * comparison of values, so a tree without width-dependent content keeps exactly its current
 * numbers.
 *
 * WIDTHS. A child is resolved at the width its parent's own arrange rule gives it; its own
 * min/max width is not applied, because an arranged width is never clamped by it. A
 * WRAP_CONTENT-width child of a MATCH_PARENT-width container is taken at the width it will
 * measure to after its parent re-measures that container at the final width: its measured
 * width capped by the constraint that re-measure hands it -- the final width clamped by the
 * container's own minimum/maximum width (ClampedWidthConstraint), less the container's padding
 * and the child's margin (PredictRemeasuredWidth). A MATCH_PARENT-width child keeps the
 * container's unclamped content width: the arrange gives it the slot, not the constraint. A
 * WRAP_CONTENT container squeezed by its parent without being re-measured (a grid cell or a
 * flex-row shrink narrower than its measured width) and the children of a WRAP_CONTENT
 * container nested inside a re-measured one keep their last measured widths; a flex row
 * builds its lines from the last measured widths.
 *
 * Height only: a width never follows a height. Every value is in VISUAL (scale-applied)
 * units. Callers skip STANDALONE children (every container loop already does); the
 * resolver itself does not, because a STANDALONE view's own slot resolves the view itself.
 */

namespace DALI_NAMESPACE
{
namespace Ui
{
class ViewImpl;
class StackLayoutManager;
class GridLayoutManager;
class FlexLayoutManager;

namespace Internal
{

/**
 * @brief The tolerance of the fit test a container applies before it adopts resolved heights.
 *
 * The same tolerance as the layout system's float compare (FloatEqual in view-data-impl.cpp). It
 * absorbs the rounding between a parent's resolution of a container and the container's own
 * arrange-time derivation, which evaluate the same expressions in a slightly different order.
 */
constexpr float HEIGHT_FOR_WIDTH_FIT_EPSILON = 0.001f;

/**
 * @brief The height a view needs when it is laid out at one exact width.
 */
struct HeightAtWidth
{
  float height;         ///< Visual units. The view's measured height when dependsOnWidth is false.
  bool  dependsOnWidth; ///< True when the height follows the width the view is laid out at.
};

/**
 * @brief The value of a predictedContentWidth parameter that requests no prediction.
 */
constexpr float NO_REMEASURE_PREDICTION = -1.0f;

/**
 * @brief The width constraint Measure() hands a view's producer for an incoming visual width.
 *
 * Measure() clamps the incoming constraint by the view's own minimum/maximum width, in natural
 * units, before the producer sees it (ViewDataImpl::Measure). A MATCH_PARENT-width container that
 * its parent re-measures at its slot is therefore re-measured at this width, not at the slot width.
 *
 * @param[in] view The view about to be measured
 * @param[in] width The incoming width constraint, visual units
 * @return The width constraint its producer receives, visual units
 */
inline float ClampedWidthConstraint(const ViewImpl& view, float width)
{
  const float s       = view.GetEffectiveScale();
  const float natural = (width > 0.0f && s > 0.0f) ? width / s : width;
  const float clamped = std::min(std::max(natural, view.GetMinimumWidth()), view.GetMaximumWidth());
  return (clamped >= 0.0f) ? clamped * s : clamped;
}

/**
 * @brief The width a WRAP_CONTENT-width child will measure to once its MATCH_PARENT-width container
 * is re-measured at its final width.
 *
 * Every supported parent re-measures a MATCH_PARENT-width child at its arranged slot right before it
 * arranges it, at the very width a resolution of that child uses, and that re-measure measures the
 * child's own children against the content width that re-measure gives it (the slot width clamped
 * by the container's own minimum/maximum width, ClampedWidthConstraint, less its padding). Every
 * first-party WRAP_CONTENT
 * producer clamps its width to its constraint (a Label's WRAP_CONTENT width rule, the default
 * measure, a layout manager whose children clamp), and Measure() then applies the child's own
 * min/max width, the maximum winning a conflict. So the last measured width, capped by the width
 * constraint the container's measure hands the child, then clamped by the child's own min/max
 * width, is exactly the width the re-measure gives a Label, and approximates a container (a
 * mismatch is left to the container's arrange-time fit test). It differs from the measured width
 * only when the container is re-measured narrower than the child's last measure.
 *
 * Used ONLY by the resolution of a MATCH_PARENT-width container, never by an arrange-time
 * derivation and never one level deeper: at arrange time the children have either been re-measured
 * already (the prediction would be the identity) or were squeezed without a re-measure and are
 * drawn at their measured width (predicting a narrower width there would size a taller box around
 * content that is drawn wider); and a deeper child's re-measure constraint equals its placement
 * width only in some containers.
 *
 * @param[in] child The WRAP_CONTENT-width child
 * @param[in] measuredWidth The child's last measured width, visual units
 * @param[in] available The width constraint the container's re-measure hands the child: the
 *                      predicted content width less the child's margin, in the form the
 *                      container's measure computes it, visual units
 * @return The width the child will measure to, visual units
 */
inline float PredictRemeasuredWidth(const ViewImpl& child, float measuredWidth, float available)
{
  const float childScale = child.GetEffectiveScale();
  return std::min(std::max(std::min(measuredWidth, available), child.GetMinimumWidth() * childScale), child.GetMaximumWidth() * childScale);
}

/**
 * @brief Whether a container's arrange resolves its children's heights at their arranged widths.
 *
 * True for an in-library container with no MeasureCallback and no ArrangeCallback that is either a
 * plain View (exactly ViewImpl) that owns no layout manager, or a view of any type whose layout
 * manager is exactly a StackLayoutManager, GridLayoutManager, FlexLayoutManager,
 * AbsoluteLayoutManager or ScrollViewLayoutManager. Any other container -- a View subclass that
 * owns no such manager, a custom or derived layout manager, a layout callback -- keeps its current
 * placement.
 *
 * @param[in] view The arranging view
 * @return True when the view's arrange resolves width-dependent children
 */
bool IsHeightForWidthContainer(ViewImpl& view);

/**
 * @brief Resolves the height @p view needs when it is laid out exactly @p width wide.
 *
 * Never measures, arranges, publishes or records anything (see the file comment). The rules:
 *  - a height that is not WRAP_CONTENT, a view with a MeasureCallback or an ArrangeCallback, and
 *    every unsupported producer report their measured height, not dependent;
 *  - a layout manager outranks the view type, as it does in Measure(): a supported manager's
 *    derivation runs at the content width (@p width less padding), and the result adds the
 *    padding back and applies the view's own min/max height;
 *  - a plain View derives its default layout at the content width (DeriveDefaultChildHeights)
 *    and reports the WRAP_CONTENT height of its default measure with the resolved child heights;
 *  - a container whose resolved content exceeds its content budget (plus
 *    HEIGHT_FOR_WIDTH_FIT_EPSILON) reports its measured height, not dependent;
 *  - a MATCH_PARENT-width container takes its WRAP_CONTENT-width children at the width its
 *    parent's re-measure will give them (PredictRemeasuredWidth);
 *  - a multi-line Label without text fit reports its text height at @p width (0 for a width of
 *    0), clamped by @p heightBudget and its own min/max height, in the order its measure applies
 *    them.
 * @p width is the width the parent's own placement rule gives the view; the view's own min/max
 * WIDTH is not applied, because an arranged width is never clamped by it. @p heightBudget is the
 * height constraint the parent's measure would hand the view, with the parent's final extent in
 * place of its measure-time extent; the view's own min/max clamps it first, as Measure() does.
 *
 * @param[in] view The view to resolve; the caller skips STANDALONE children
 * @param[in] width The exact width the view is laid out at, visual units
 * @param[in] heightBudget The height budget, visual units
 * @return The resolved height and whether it depends on the width
 */
HeightAtWidth ResolveHeightAtWidth(ViewImpl& view, float width, float heightBudget);

/**
 * @brief The default (plain View) arrange derivation, shared by ViewDataImpl::ArrangeDefault and
 * the plain-View resolution.
 *
 * Walks @p children -- ArrangeDefault's own child snapshot, or the container's children captured
 * the same way -- and, for every non-STANDALONE child with a WRAP_CONTENT height, resolves its
 * height at the width the default arrange gives it: the content width less its margin for a
 * MATCH_PARENT width, its measured width otherwise (for a WRAP_CONTENT width, when
 * @p predictedContentWidth is not negative, the width PredictRemeasuredWidth predicts from it);
 * the height budget is the
 * content height less its margin. Every other child keeps its measured height; a MATCH_PARENT
 * height contributes its measured (minimum) height, exactly as the default measure aggregates it.
 *
 * On the first width-dependent child @p resolvedHeights is assigned (children.size(), -1.0f);
 * entry i is then the resolved height of children[i] when it is >= 0. @p neededContentHeight
 * receives the largest requestedY * ownerScale + marginH + height over the non-STANDALONE
 * children: the content height the default layout needs with the resolved heights in place.
 *
 * @param[in] container The plain View whose children are derived (it provides the owner scale)
 * @param[in] children The container's children, in its child order
 * @param[in] contentWidth The container's width less its padding, visual units, not clamped
 * @param[in] contentHeight The container's height less its padding, visual units, not clamped
 * @param[in] predictedContentWidth Only for the resolution of a MATCH_PARENT-width container: the
 *                                  content width the default measure gets when its parent
 *                                  re-measures it at its slot (the clamped constraint in natural
 *                                  units less the padding, times the scale). ArrangeDefault passes
 *                                  NO_REMEASURE_PREDICTION.
 * @param[out] resolvedHeights The per-child resolved heights, left untouched when nothing depends
 * @param[out] neededContentHeight The content height needed, written only when something depends
 * @return True when at least one child depends on its width (the outputs are valid only then)
 */
bool DeriveDefaultChildHeights(ViewImpl& container, const std::vector<Ui::View>& children, float contentWidth, float contentHeight, float predictedContentWidth, std::vector<float>& resolvedHeights, float& neededContentHeight);

/**
 * @brief The height-for-width step of a layout root's or a STANDALONE view's own slot.
 *
 * Called right after DeriveStandaloneRootBounds() by all three slot derivations -- the parent's
 * placement of a STANDALONE child (ArrangeStandaloneChild) and both branches of
 * LayoutController::ProcessLayoutRoot -- with the snapshot, the height constraint the view was
 * just measured at and the bounds the derivation produced. A WRAP_CONTENT height (per the
 * snapshot) that depends on its width is replaced by the height resolved at bounds.width -- the
 * slot width, already clamped by the view's own min/max -- and clamped by the snapshot's min/max
 * exactly as the derivation clamps it. Otherwise @p bounds is left untouched. Fed the same
 * snapshot, constraint and measured subtree, every caller derives the same bounds, which keeps the
 * parent placement and the view's own pass convergent.
 *
 * @param[in] view The boundary view or layout root being placed
 * @param[in] inputs The view's slot inputs, snapshotted before the pass
 * @param[in] heightBudget The height constraint the view was measured at, visual units
 * @param[in,out] bounds The derived slot, in visual units
 */
void ApplyRootHeightForWidth(ViewImpl& view, const StandaloneSlotInputs& inputs, float heightBudget, LayoutRect& bounds);

/**
 * @brief Manager derivations: the content height a supported layout manager needs at an exact
 * content width.
 *
 * Each is defined in its manager's own source file, so it shares the file-local helpers that
 * manager's Arrange() uses, and mirrors that manager's WRAP_CONTENT measure height with every
 * width-dependent child resolved at the width the manager's arrange gives it. They are called
 * only for a view with a WRAP_CONTENT height whose manager has exactly that type.
 *
 * @param[in] manager The view's layout manager (Absolute keeps no state of its own)
 * @param[in] view The view the manager is attached to
 * @param[in] contentWidth The view's width less its padding, visual units
 * @param[in] contentHeightBudget The height budget less the view's padding, visual units
 * @param[out] contentHeight The content height needed, written only when the function returns true
 * @param[in] predictedContentWidth Not negative only when @p view is MATCH_PARENT wide, so its
 *                                  parent re-measures it at its slot before it arranges it: the
 *                                  content width the manager's measure gets in that re-measure (the
 *                                  slot width clamped by the view's own minimum/maximum width, less
 *                                  its padding). Its WRAP_CONTENT-width children are then taken at
 *                                  the width that re-measure gives them (PredictRemeasuredWidth).
 *                                  The Grid derivation ignores it (see its definition).
 * @return True only when the content height depends on the width
 */
bool ResolveStackContentHeightForWidth(const StackLayoutManager& manager, ViewImpl& view, float contentWidth, float contentHeightBudget, float& contentHeight, float predictedContentWidth);

/// @copydoc ResolveStackContentHeightForWidth
bool ResolveGridContentHeightForWidth(const GridLayoutManager& manager, ViewImpl& view, float contentWidth, float contentHeightBudget, float& contentHeight, float predictedContentWidth);

/// @copydoc ResolveStackContentHeightForWidth
bool ResolveFlexContentHeightForWidth(const FlexLayoutManager& manager, ViewImpl& view, float contentWidth, float contentHeightBudget, float& contentHeight, float predictedContentWidth);

/**
 * @brief The AbsoluteLayoutManager derivation; see ResolveStackContentHeightForWidth.
 *
 * @param[in] view The view the manager is attached to
 * @param[in] contentWidth The view's width less its padding, visual units
 * @param[in] contentHeightBudget The height budget less the view's padding, visual units
 * @param[out] contentHeight The content height needed, written only when the function returns true
 * @param[in] predictedContentWidth See ResolveStackContentHeightForWidth
 * @return True only when the content height depends on the width
 */
bool ResolveAbsoluteContentHeightForWidth(ViewImpl& view, float contentWidth, float contentHeightBudget, float& contentHeight, float predictedContentWidth);

} // namespace Internal
} // namespace Ui
} // namespace DALI_NAMESPACE
