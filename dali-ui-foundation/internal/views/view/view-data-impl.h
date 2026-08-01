#ifndef DALI_UI_VIEW_DATA_IMPL_H
#define DALI_UI_VIEW_DATA_IMPL_H

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
#include <dali/devel-api/adaptor-framework/accessibility-devel.h> // LCOV_EXCL_LINE
#include <dali/devel-api/object/type-registry.h>
#include <dali/integration-api/adaptor-framework/accessibility/accessibility-bridge.h> // LCOV_EXCL_LINE
#include <dali/integration-api/adaptor-framework/accessibility/accessibility-integ.h>  // LCOV_EXCL_LINE
#include <dali/integration-api/processor-interface.h>
#include <dali/public-api/animation/constraint.h>
#include <dali/public-api/math/compile-time-math.h>
#include <dali/public-api/object/property-notification.h>
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/view-accessible.h>
#include <dali-ui-foundation/integration-api/visuals/visual-properties-integ.h>
#include <dali-ui-foundation/internal/render-effects/offscreen-rendering-impl.h>
#include <dali-ui-foundation/internal/render-effects/render-effect-impl.h>
#include <dali-ui-foundation/internal/visuals/visual-base-impl.h>
#include <dali-ui-foundation/public-api/layouts/layout-transition.h>
#include <dali-ui-foundation/public-api/traits/attachment-id.h>
#include <dali-ui-foundation/public-api/traits/trait-id.h>
#include <dali-ui-foundation/public-api/types/shadow.h>
#include <dali-ui-foundation/public-api/types/ui-property-index-ranges.h>
#include <dali-ui-foundation/public-api/types/unique-any.h>
#include <dali-ui-foundation/public-api/views/view-accessibility-types.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include <dali-ui-foundation/public-api/visuals/visual-base.h>
#include <dali/integration-api/debug.h>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <unordered_set>

namespace Dali
{
namespace Ui
{

namespace Integration
{
class SizeNegotiatedViewImpl;
} // namespace Integration

namespace Internal
{
class InteractiveTraitImpl;
class CoreInteractionObject;

/// @brief Type-level animatable property index for effective UI scale.
/// Defined here (not in the public View::Property enum) to keep it internal.
/// Value matches Dali::Ui::View::ANIMATABLE_PROPERTY_START_INDEX + 500,
/// @warning Please change this value if view.h add new enum as Dali::Ui::View::ANIMATABLE_PROPERTY_START_INDEX + 500
inline constexpr Property::Index VIEW_EFFECTIVE_SCALE_PROPERTY_INDEX = View::ANIMATABLE_PROPERTY_START_INDEX + 500;

class AttachmentContainer;

namespace Visual
{
class Base;
}

enum class TriStateProperty
{
  AUTO = 0,
  TRUE,
  FALSE
};

/**
 * @brief Layout-transition changes accumulated since the preceding layout pass.
 *
 * The layout transition dispatcher consumes this state once per pass to
 * determine ENTER and CHANGE causes.
 */
struct PendingLayoutTransitionChanges
{
  std::unordered_set<ViewImpl*> enterChildren;
  std::unordered_set<ViewImpl*> reorderedChildren;
  bool                          hadChildRemoval{false};
};

/**
 * @brief Holds the Implementation for the internal view class
 */
class ViewDataImpl : public ConnectionTracker, public Dali::Integration::Processor
{
private:
  friend class ::Dali::Ui::ViewImpl;
  friend class ::Dali::Ui::Integration::SizeNegotiatedViewImpl;
  friend std::string DumpView(const ::Dali::Ui::ViewImpl& view);

  class AccessibilityData;
  class VisualData;

public:
  using AccessibleObjectCreator = ViewAccessible* (*)(Dali::Actor);

  /**
   * @brief Retrieves the implementation of the internal view class.
   * @param[in] viewImpl A ref to the view whose internal implementation is required
   * @return The internal implementation
   */
  static ViewDataImpl& Get(ViewImpl& viewImpl);

  static const ViewDataImpl& Get(const ViewImpl& viewImpl);

  /**
   * @brief Constructor.
   * @param[in] viewImpl The view which owns this implementation
   */
  ViewDataImpl(ViewImpl& viewImpl);

  /**
   * @brief Destructor.
   */
  ~ViewDataImpl();

  bool AreVisualsEnabled() const;

  MeasuredSize Measure(float visualWidth, float visualHeight);
  LayoutRect   Arrange(const LayoutRect& bounds);

  const ViewState&                            GetState() const;
  bool                                        IsEffectivelyFocused() const;
  View::LayoutFinishedSignalType&             LayoutFinishedSignal();
  View::StateChangedSignalType&               StateChangedSignal();
  View::ResourceReadySignalType&              ResourceReadySignal();
  View::OffScreenRenderingFinishedSignalType& OffScreenRenderingFinishedSignal();
  bool                                        HasLayoutFinishedSignalConnections() const;
  void                                        EmitLayoutFinishedSignal(const LayoutRect& bounds);
  PendingLayoutTransitionChanges              TakePendingLayoutTransitionChanges();

  InteractiveTrait     EnsureInteractiveTrait();
  void                 SetStateEffect(StateEffect effect);
  void                 AttachInteractiveStateEffect();
  bool                 IsDefaultFocusIndicatorSuppressedByStateEffect() const;
  void                 RefreshDefaultFocusIndicatorSuppression();
  void                 InvalidateDefaultFocusIndicatorSuppression(const Integration::StateEffectImpl& effect);
  void                 SetStateEffectTarget(View target);
  View                 GetStateEffectTarget() const;
  bool                 IsInteractive() const;
  SelectableTrait      EnsureSelectableTrait();
  bool                 IsSelectable() const;
  GroupSelectableTrait EnsureGroupSelectableTrait();
  bool                 IsGroupSelectable() const;

  UiColor            GetBackgroundColor() const;
  void               SetBackgroundColor(const UiColor& color);
  void               SetBackgroundImage(const Dali::String& url);
  void               SetBackgroundGradient(const Gradient::Base& gradient);
  UiColor            GetColor() const;
  void               SetColor(const UiColor& color);
  UiColor            GetCurrentColor() const;
  Vector4            GetCornerRadius() const;
  void               SetCornerRadius(const Vector4& radius);
  CornerRadiusPolicy GetCornerRadiusPolicy() const;
  void               SetCornerRadiusPolicy(CornerRadiusPolicy policy);
  Vector4            GetCornerSquareness() const;
  void               SetCornerSquareness(const Vector4& squareness);
  float              GetBorderlineWidth() const;
  void               SetBorderlineWidth(float width);
  UiColor            GetBorderlineColor() const;
  void               SetBorderlineColor(const UiColor& color);
  float              GetBorderlineOffset() const;
  void               SetBorderlineOffset(float offset);
  void               ClearBackground();
  void               SetShadow(const Shadow& shadow);
  void               SetShadow(const ShadowStack& shadowStack);

  void                          SetFocusNavigationCallback(Callback<View(View, FocusDirection)> callback);
  View                          RequestFocusNavigation(View currentFocusedView, FocusDirection direction);
  View                          RequestFocus();
  bool                          IsFocusGroup() const;
  void                          SetAsFocusGroup(bool isFocusGroup);
  View::KeyEventSignalType&     KeyEventSignal();
  View::FocusChangedSignalType& FocusChangedSignal();
  bool                          NotifyKeyEvent(const KeyEvent& event);

  void                      SetRequestedX(float x);
  void                      SetRequestedY(float y);
  float                     GetRequestedX() const;
  float                     GetRequestedY() const;
  void                      SetUiScalePolicy(UiScalePolicy policy);
  UiScalePolicy             GetUiScalePolicy() const;
  float                     GetEffectiveScale() const;
  void                      InvalidateMeasure();
  void                      InvalidateArrange();
  MeasuredSize              GetMeasuredSize() const;
  void                      SetRequestedWidth(float width);
  float                     GetRequestedWidth() const;
  void                      SetRequestedHeight(float height);
  float                     GetRequestedHeight() const;
  void                      SetMinimumWidth(float width);
  float                     GetMinimumWidth() const;
  void                      SetMinimumHeight(float height);
  float                     GetMinimumHeight() const;
  void                      SetMaximumWidth(float width);
  float                     GetMaximumWidth() const;
  void                      SetMaximumHeight(float height);
  float                     GetMaximumHeight() const;
  void                      SetMargin(const Insets& margin);
  Insets                    GetMargin() const;
  void                      SetPadding(const Insets& padding);
  Insets                    GetPadding() const;
  void                      SetLayoutMode(LayoutMode mode);
  LayoutMode                GetLayoutMode() const;
  void                      SetLayoutTransition(LayoutTransition transition);
  LayoutTransition          GetLayoutTransition() const;
  LayoutRect                GetArrangedBounds() const;
  bool                      IsInitialLayoutDone() const;
  uint32_t                  GetChildViewCount() const;
  View                      GetChildViewAt(uint32_t index) const;
  Dali::Vector<View>&       GetChildren();
  const Dali::Vector<View>& GetChildren() const;
  int32_t                   IndexOfChildView(View view) const;
  void                      Insert(uint32_t index, View child);
  void                      RemoveAllChildren(RemovePolicy policy);
  void                      Remove(View child, RemovePolicy policy);
  void                      Raise(LayoutOrderPolicy policy);
  void                      Lower(LayoutOrderPolicy policy);
  void                      RaiseToTop(LayoutOrderPolicy policy);
  void                      LowerToBottom(LayoutOrderPolicy policy);
  void                      RaiseAbove(View target, LayoutOrderPolicy policy);
  void                      LowerBelow(View target, LayoutOrderPolicy policy);

  void SetMeasureCallback(MeasureCallback callback);
  void SetArrangeCallback(ArrangeCallback callback);
  void SetArrangeCallback(ArrangeCallback callback, ArrangePurity purity);
  /// Declares the purity of this view's OnArrange(). Callable from a constructor,
  /// before the CustomActor handle exists: it only invalidates when a published
  /// cache entry actually exists (see the comment on the definition).
  void             SetArrangePurity(ArrangePurity purity);
  MeasureCallback* GetMeasureCallback();
  ArrangeCallback* GetArrangeCallback();
  void             AttachLayoutManager(Dali::UniquePtr<LayoutManager> manager);
  LayoutManager*   GetLayoutManager() const;
  bool             HasLayoutManager() const;
  bool             HasLayoutCallback() const;

  void          SetLayoutParams(const AbsoluteLayoutParams& params);
  void          SetLayoutParams(const FlexLayoutParams& params);
  void          SetLayoutParams(const GridLayoutParams& params);
  void          SetLayoutParams(const StackLayoutParams& params);
  bool          TryGetLayoutParams(AbsoluteLayoutParams& params) const;
  bool          TryGetLayoutParams(FlexLayoutParams& params) const;
  bool          TryGetLayoutParams(GridLayoutParams& params) const;
  bool          TryGetLayoutParams(StackLayoutParams& params) const;
  void          GetOffScreenRenderTasks(Dali::Vector<Dali::RenderTask>& tasks, bool isForward);
  Dali::Texture GetOffScreenRenderingOutput() const;
  /// Natural size of the background visual plus padding, or ZERO when the view has
  /// no background visual. Not an override and never virtually dispatched;
  /// Actor::GetNaturalSize() reaches it only through SizeNegotiatedViewImpl.
  Vector3      GetBackgroundVisualNaturalSize();
  void         SetRenderEffect(RenderEffect effect);
  RenderEffect GetRenderEffect() const;
  void         ClearRenderEffect();

  /**
   * @brief Drops the cached logical context and the layout caches of this view
   * and of every descendant.
   *
   * Used by the paths that can move the effective scale of a whole subtree at
   * once -- a UiScalePolicy change, a global UI scale change, and a reparent --
   * where the change re-roots the INHERIT chain, so every descendant's cached
   * scale (and therefore every cached measure/arrange result derived from it)
   * is potentially stale.
   *
   * It is exactly DropCachedLogicalContext() + InvalidateLayoutCaches() applied
   * to the subtree: it raises NO dirty bit and registers nothing with the
   * LayoutController. The callers follow it with InvalidateMeasure(), which is
   * what propagates upward and schedules the re-layout.
   */
  void  InvalidateLogicalContextRecursive();
  float ComputeEffectiveScale() const;

  /// @name Layout cache-state accessors (white-box test support)
  /// Plain one-line readers of the layout bookkeeping bits. They exist so the
  /// internal UTC target can assert on cache state directly instead of inferring
  /// it from geometry; this is an internal, non-exported header, so they add no
  /// public API and no ABI surface. Nothing in the library reads them.
  /// @{
  bool IsMeasureCacheValid() const
  {
    return mMeasureCacheValid;
  }
  bool IsArrangeCacheValid() const
  {
    return mArrangeCacheValid;
  }
  Dali::LayoutDirection::Type GetLastArrangeDirection() const
  {
    return mLastArrangeDirection;
  }
  bool IsMeasureDirty() const
  {
    return mMeasureDirty;
  }
  bool IsArrangeDirty() const
  {
    return mArrangeDirty;
  }
  bool IsLogicalContextValid() const
  {
    return mLogicalContextValid;
  }
  /// The ACTOR-side half of the scale sync pair: true when the animatable
  /// VIEW_EFFECTIVE_SCALE property is known to hold mEffectiveScale, which is what
  /// lets Measure() skip reading that property on a cache hit.
  bool IsEffectiveScaleActorSynced() const
  {
    return mEffectiveScaleActorSynced;
  }
  /// The DERIVED purity bit -- the term the arrange cache-HIT predicate reads.
  /// False unless the ACTIVE producer has been declared pure, which is what makes
  /// an undeclared (third-party) producer permanently ineligible for the hit.
  bool IsArrangeProducerPure() const
  {
    return mArrangeProducerPure;
  }
  /// The epoch each axis last propagated its invalidation to a layout root in.
  /// Compared against LayoutInvalidation::CurrentEpoch() to decide whether a further
  /// invalidation may skip the ancestor walk; 0 means never propagated.
  uint32_t GetMeasurePropagationEpoch() const
  {
    return mMeasurePropagationEpoch;
  }
  uint32_t GetArrangePropagationEpoch() const
  {
    return mArrangePropagationEpoch;
  }
  /// @}

  Ui::Layout GetParentLayout() const;
  View       GetParentView() const;
  void       EmitFocusChangedSignal(bool focusGained);
  void       RegisterWithLayoutController();

  bool UpdateColorBindingInternal(StringView bindingId, const UiColor& color);
  void SetColorBindingInternal(StringView bindingId, const UiColor& color, ColorCallback callback);
  bool UpdateColorBindingInternal(StringView bindingId, const Gradient::Base& gradient);
  void SetColorBindingInternal(StringView bindingId, const Gradient::Base& gradient, Callback<void(const Gradient::Base&)> callback);
  void ClearGradientColorBinding(StringView bindingId);
  void ClearBackgroundBinding();
  void SetBackgroundColorInternal(const Vector4& color);
  void SetBackgroundGradientInternal(const Gradient::Base& gradient);
  void SetBorderlineColorInternal(const Vector4& color);
  void SetColorInternal(const Vector4& color);

  /**
   * @brief Initialize private VisualData context for this impl.
   */
  void InitializeVisualData();

  // Trait management (delegated from ViewImpl)

  /**
   * @brief Notifies all traits that the owning View is being destroyed.
   *
   * Must be called while ViewImpl members are still valid (i.e. inside ViewImpl::~ViewImpl body,
   * before `delete mImpl`).
   */
  void NotifyTraitsViewDestroying();

  /**
   * @brief Sets a trait data to the owning View.
   *
   * Lifecycle callbacks (OnAttached, OnDetaching, OnViewDestroying) are invoked
   * automatically.
   *
   * @warning Do not store Actor-derived objects as trait data. Actors are owned by
   * the scene graph and have their own parent-child lifecycle. Storing them here
   * causes ownership conflicts and potential dangling references.
   *
   * @param[in] id The key to identify the trait
   * @param[in] object The object to store
   */
  void SetTrait(TraitId id, IntrusivePtr<TraitObject> object);

  /**
   * @brief Gets a trait data from the owning View.
   *
   * @param[in] id The key to identify the trait
   * @return The stored object, or nullptr if not found
   */
  IntrusivePtr<TraitObject> GetTrait(TraitId id) const;

  /**
   * @brief Removes a trait from the owning View.
   */
  bool RemoveTrait(TraitId id);

  /**
   * @brief Sets an attachment to the owning View.
   *
   * Replaces the existing attachment when @p id is already present.
   */
  void SetAttachment(AttachmentId id, UniqueAny attachment);

  /**
   * @brief Removes an attachment from the owning View.
   *
   * @return True if an attachment was removed
   */
  bool RemoveAttachment(AttachmentId id);

  /**
   * @brief Detaches an attachment from the owning View.
   *
   * @return The stored attachment, or an empty UniqueAny if missing
   */
  UniqueAny DetachAttachment(AttachmentId id);

  /**
   * @brief Gets a raw attachment from the owning View.
   *
   * @return Pointer to the attachment, or nullptr if missing
   */
  UniqueAny* GetAttachment(AttachmentId id);

  /**
   * @brief Gets a const raw attachment from the owning View.
   *
   * @return Pointer to the attachment, or nullptr if missing
   */
  const UniqueAny* GetAttachment(AttachmentId id) const;

  // State management

  /**
   * @brief Updates a state bit and notifies ViewStateManager if the state changed.
   * @param[in] stateToChange The state to set or clear
   * @param[in] on            True to add the state, false to remove it
   * @param[in] cause         Input event that triggered the change
   */
  void SetState(ViewState stateToChange, bool on, InputEvent cause);

  /**
   * @brief Clears and sets states as a single state change notification.
   * @param[in] statesToClear The states to clear
   * @param[in] statesToSet   The states to set
   * @param[in] cause         Input event that triggered the change
   */
  void SetState(ViewState statesToClear, ViewState statesToSet, InputEvent cause);

  /**
   * @brief Registers a named state observer.
   * @param[in] id       Unique identifier for this observer
   * @param[in] tracker  ConnectionTrackerInterface for lifetime management
   * @param[in] callback Callback with signature void(View, const StateEvent&)
   */
  void SetNamedStateObserver(const Dali::String& id, Dali::ConnectionTrackerInterface* tracker, CallbackBase* callback);

  /**
   * @brief Removes a named state observer.
   * @param[in] id The observer identifier to remove
   * @return True if an observer was found and removed
   */
  bool UnsetNamedStateObserver(const Dali::String& id);

  /**
   * @brief Removes a named state observer unless its callback is currently executing.
   * @param[in] id The observer identifier to remove
   * @return True if removed; false if currently executing or not found
   */
  bool UnsetNamedStateObserverIfNotExecuting(const Dali::String& id);

  /**
   * @brief Returns the core interaction trait object pointer (may be null).
   */
  Internal::CoreInteractionObject* GetCoreInteractionObject() const;

  /**
   * @brief Called when resources of view are ready.
   */
  void ResourceReady();

  void RegisterVisual(Property::Index index, Integration::Visual::Base& visual);

  void RegisterVisual(Property::Index index, Integration::Visual::Base& visual, int depthIndex);

  void RegisterVisual(Property::Index index, Integration::Visual::Base& visual, bool enabled);

  void RegisterVisual(Property::Index index, Integration::Visual::Base& visual, bool enabled, int depthIndex);

  void UnregisterVisual(Property::Index index);

  Integration::Visual::Base GetVisual(Property::Index index) const;

  /**
   * @brief Get the raw pointer of visual impl.
   * It will be used when we want to get visual infomations without increase reference counts.
   * @note Only for internal usage.
   *
   * @param[in] index Index of parameter
   * @return Raw pointer of visual base implements. nullptr if not exist.
   */
  Visual::Base* GetVisualImplPtr(Property::Index index) const;

  /**
   * @brief Sets the background visual from a property map.
   * @param[in] map The background visual property map
   */
  void SetBackground(const Property::Map& map);

  /**
   * @brief Enables or disables overriding the given visual's corner properties to its view's
   * @param[in] visual A registered visual
   * @param[in] enable flat to set enabled or disabled.
   * @param[in] cornerRadiusConstraint Optional constraint to link the view's corner properties to the visual's.
   */
  void EnableCornerPropertiesOverridden(Integration::Visual::Base& visual, bool enable,
                                        Dali::Constraint cornerRadiusConstraint = Dali::Constraint());

  void EnableVisual(Property::Index index, bool enable);

  bool IsVisualEnabled(Property::Index index) const;

  Ui::Visual::ResourceStatus GetVisualResourceStatus(Property::Index index) const;

  void DoAction(Dali::Property::Index visualIndex, Dali::Property::Index actionId,
                const Dali::Property::Value& attributes);

  void DoActionExtension(Dali::Property::Index visualIndex, Dali::Property::Index actionId,
                         const Dali::Any& attributes);

  bool AddVisualObject(VisualBase visualBase, Integration::Visual::InternalContainerRangeType internalContainerRangeType);

  /**
   * @brief Adds a shadow visual object.
   * @param[in] visualBase The shadow visual to add
   * @param[in] internalContainerRangeType The range of visuals to be added
   * @return True if the visual was added successfully, false otherwise
   */
  bool AddShadowVisualObject(VisualBase visualBase, Integration::Visual::InternalContainerRangeType internalContainerRangeType);

  void RemoveVisualObject(VisualBase visualBase);

  uint32_t GetVisualObjectCount(Integration::Visual::InternalContainerRangeType internalContainerRangeType) const;

  VisualBase GetVisualObjectAt(Integration::Visual::InternalContainerRangeType internalContainerRangeType, uint32_t siblingOrder) const;

  /**
   * @brief Function used to set view properties.
   * @param[in] object The object whose property to set
   * @param[in] index The index of the property to set
   * @param[in] value The value of the property to set
   */
  static void SetProperty(BaseObject* object, Property::Index index, const Property::Value& value);

  /**
   * @brief Function used to retrieve the value of view properties.
   * @param[in] object The object whose property to get
   * @param[in] index The index of the property to get
   * @return The value of the property
   */
  static Property::Value GetProperty(BaseObject* object, Property::Index index);

  /**
   * @brief Whether the resource is ready
   * @return True if the resource is read.
   */
  bool IsResourceReady() const;

  void OnSceneConnection();

  void OnSceneDisconnection();

  /**
   * @brief Get private AccessibilityData context for this impl. If not created yet, it will create new data.
   * @return The l-value of AccessibilityData context.
   */
  [[nodiscard]] AccessibilityData& GetOrCreateAccessibilityData();

  /**
   * @brief Get private AccessibilityData context for this impl.
   * @return The pointer of AccessibilityData context.
   */
  [[nodiscard]] AccessibilityData* GetAccessibilityData() const;

  void                SetAccessibilityName(StringView name);
  Dali::String        GetAccessibilityName() const;
  void                SetAccessibilityDescription(StringView description);
  Dali::String        GetAccessibilityDescription() const;
  void                SetAccessibilityValue(StringView value);
  Dali::String        GetAccessibilityValue() const;
  void                SetAccessibilityRole(Accessibility::Role role);
  Accessibility::Role GetAccessibilityRole() const;
  void                SetAccessibilityHidden(bool hidden);
  bool                IsAccessibilityHidden() const;
  void                SetAccessibilityHighlightable(bool highlightable);
  void                ResetAccessibilityHighlightable();
  bool                IsAccessibilityHighlightable() const;
  void                SetAccessibilityScrollable(bool scrollable);
  bool                IsAccessibilityScrollable() const;
  void                SetAccessibilityModal(bool modal);
  bool                IsAccessibilityModal() const;
  void                SetAutomationId(StringView automationId);
  Dali::String        GetAutomationId() const;

  void         SetTranslatableAccessibilityName(StringView resourceId, StringView domain);
  Dali::String GetTranslatableAccessibilityName() const;
  void         ClearTranslatableAccessibilityName();
  void         SetTranslatableAccessibilityDescription(StringView resourceId, StringView domain);
  Dali::String GetTranslatableAccessibilityDescription() const;
  void         ClearTranslatableAccessibilityDescription();

  void AddAccessibilityRelation(Accessibility::RelationType type, View target);
  void RemoveAccessibilityRelation(Accessibility::RelationType type, View target);
  void ClearAccessibilityRelations();
  bool HasAccessibilityRelation(Accessibility::RelationType type, View target) const;

  void AddAccessibilityReadingInfo(Accessibility::ReadingInfo info);
  void RemoveAccessibilityReadingInfo(Accessibility::ReadingInfo info);
  void ClearAccessibilityReadingInfo();
  bool HasAccessibilityReadingInfo(Accessibility::ReadingInfo info) const;

  bool AddAccessibilityNameLanguageSpan(uint32_t start, uint32_t length, StringView locale);
  void ClearAccessibilityNameLanguageSpans();
  bool AddAccessibilityDescriptionLanguageSpan(uint32_t start, uint32_t length, StringView locale);
  void ClearAccessibilityDescriptionLanguageSpans();

  void    SetRequestInitialAccessibilityHighlight(bool request);
  bool    IsInitialAccessibilityHighlightRequested() const;
  void    SetAccessibilityCollectionContainer(bool container);
  bool    IsAccessibilityCollectionContainer() const;
  void    SetAccessibilityCollectionIndex(int32_t index);
  int32_t GetAccessibilityCollectionIndex() const;
  void    ClearAccessibilityCollectionIndex();

  View::AccessibilityReadingStatusChangedSignalType& AccessibilityReadingStatusChangedSignal();
  View::AccessibilityHighlightedSignalType&          AccessibilityHighlightedSignal();

  /**
   * @brief Adds accessibility attribute
   * @param[in] key Attribute name to set
   * @param[in] value Attribute value to set
   *
   * Attribute is added if not existed previously or updated
   * if existed.
   */
  void AppendAccessibilityAttribute(const Dali::String& key, const Dali::String& value);

  /**
   * @brief Removes accessibility attribute
   * @param[in] key Attribute name to remove
   *
   * Function does nothing if attribute doesn't exist.
   */
  void RemoveAccessibilityAttribute(const Dali::String& key);

  /**
   * @brief Removes all accessibility attributes
   */
  void ClearAccessibilityAttributes();

  /**
   * @brief Sets reading info type attributes
   * @param[in] types info type attributes to set
   *
   * This function sets, which part of object will be read out
   * by screen-reader.
   */
  void SetAccessibilityReadingInfoType(const Dali::Integration::Accessibility::ReadingInfoTypes types); // LCOV_EXCL_LINE

  /**
   * @brief Gets currently active reading info type attributes
   */
  Dali::Integration::Accessibility::ReadingInfoTypes GetAccessibilityReadingInfoType() const; // LCOV_EXCL_LINE

  View::VisualEventSignalType& VisualEventSignal();

  /**
   * @brief Replaces all shadows with a single shadow described by a property map.
   *
   * This is the View::Property::SHADOW setter path. It clears both the first
   * shadow and any additional shadows, then installs @p map as the first shadow.
   *
   * @param[in] map The shadow property map
   */
  void SetShadow(const Property::Map& map);

  /**
   * @brief Sets only the first shadow visual.
   *
   * The first shadow is registered as View::Property::SHADOW so property lookup
   * and typed shadow animations can target it directly.
   *
   * @param[in] map The shadow property map
   */
  void SetFirstShadow(const Property::Map& map);

  /**
   * @brief Appends a shadow value to the shadow stack.
   *
   * The first appended shadow is installed through SetFirstShadow() so it keeps
   * the View::Property::SHADOW identity used by property lookup and typed
   * shadow animations. Later shadows are appended as container visuals.
   *
   * @param[in] shadow The shadow value to append
   */
  void AppendShadow(const Shadow& shadow);

  /**
   * @brief Clears the first shadow and all additional shadow visuals.
   */
  void ClearShadow();

  /**
   * @brief Sets the inner shadow with a property map.
   * @param[in] map The inner shadow property map
   */
  void SetInnerShadow(const Property::Map& map);

  /**
   * @brief Clear the inner shadow.
   */
  void ClearInnerShadow();

  /**
   * @brief Sets the borderline with a property map.
   * @param[in] map The borderline property map
   * @param[in] forciblyCreate Create new visual forcibly, False if we only need to update properties.
   */
  void SetBorderline(const Property::Map& map, bool forciblyCreate);

  /**
   * @brief Clear the borderline.
   */
  void ClearBorderline();

  Dali::Property GetVisualProperty(Dali::Property::Index index, Dali::Property::Key visualPropertyKey);

  /**
   * @brief Create constraints to animate animatable properties.
   * @param[in] animationObject BaseObject of Animation or Constraint
   * @param[in] index The animatable property
   */
  void CreateAnimationConstraints(const Dali::BaseObject& animationObject, Property::Index index);

  /**
   * @brief Clear animatable constraints
   * @param[in] animationObject BaseObject of Animation or Constraint
   * @param[in] index The animatable property
   */
  void ClearAnimationConstraints(const Dali::BaseObject& animationObject, Property::Index index);

  SharedPtr<ViewAccessible> GetAccessibleObject();

  Dali::Vector<Dali::Devel::Accessibility::Relation> GetAccessibilityRelations(); // LCOV_EXCL_LINE

  /**
   * @brief Sets the accessibility states.
   * @param[in] states The accessibility state mask
   */
  void SetAccessibilityStates(uint32_t states);

  /**
   * @brief Gets the accessibility states.
   * @return The accessibility state mask
   */
  uint32_t GetAccessibilityStates() const;

  /**
   * @brief Adds the accessibility state.
   * @param[in] state The state to add
   */
  void AddAccessibilityState(Accessibility::State state);

  /**
   * @brief Removes the accessibility state.
   * @param[in] state The state to remove
   */
  void RemoveAccessibilityState(Accessibility::State state);

  /**
   * @brief Clears all accessibility states.
   */
  void ClearAccessibilityStates();

  /**
   * @brief Returns whether the accessibility state is set.
   * @param[in] state The state to query
   * @return True if the state is set
   */
  bool HasAccessibilityState(Accessibility::State state) const;

  bool IsAccessibleCreated() const;

  void EnableCreateAccessible(bool enable);

  bool IsCreateAccessibleEnabled() const;

  /**
   * @brief Sets the integration creator used for a custom Accessible object.
   */
  void SetAccessibleObjectCreator(AccessibleObjectCreator creator);

  /**
   * @brief Creates the configured Accessible object or the default one.
   */
  ViewAccessible* CreateAccessibleObject();

  void EmitAccessibilityStateChanged(Dali::Integration::Accessibility::State state, int newValue); // LCOV_EXCL_LINE

  /**
   * @brief Apply fittingMode
   *
   * @param[in] size The size of the view
   * @param[in] isLayoutFinishedUpdate Whether fitting mode is updated after layout has finished
   */
  void ApplyFittingMode(const Vector2& size, bool isLayoutFinishedUpdate = false);

  /**
   * @brief Ensures this view listens to its layout-finished signal for fitting mode update.
   */
  void EnsureFittingModeLayoutFinishedSignalConnected();

  /**
   * @brief Called when this view's layout is finished.
   *
   * @param[in] view The view whose layout is finished
   * @param[in] bounds The arranged bounds of the view
   */
  void OnLayoutFinished(Ui::View view, LayoutRect bounds);

  /**
   * @brief Register processor
   */
  void RegisterProcessorOnce();

  /**
   * Call if mSize or EffectiveScale changed.
   */
  void SizeOrUiScaleChanged();

  /**
   * @brief Refreshes render effects (e.g. blur) that depend on the current self size.
   *
   * Unlike SizeOrUiScaleChanged(), this does not re-register the fitting-mode
   * processor: fitting mode for layout-arranged views is already driven by the
   * layout-finished signal (see EnsureFittingModeLayoutFinishedSignalConnected()),
   * so triggering it again here would apply it twice per layout pass.
   */
  void RefreshRenderEffects();

protected: // From processor-interface
  void Process(bool postProcessor) override;

  std::string_view GetProcessorName() const override
  {
    return "ViewDataImpl";
  }

private:
  void SetResolvedAccessibilityName(const Dali::String& name);
  void SetResolvedAccessibilityDescription(const Dali::String& description);
  void ApplyLocalizedAccessibilityName(BaseHandle target, const Dali::String& name);
  void ApplyLocalizedAccessibilityDescription(BaseHandle target, const Dali::String& description);

  class ScopedSkipChildrenUpdate;

  void SetBehaviourFlags(ViewImpl::ViewBehaviour behaviourFlags);
  void Destroy();

  MeasuredSize MeasureDefault(float widthConstraint, float heightConstraint);
  LayoutRect   ArrangeDefault(const LayoutRect& bounds);
  bool         HandleKeyEventDefault(const Dali::KeyEvent& event);
  void         FinalizeKeyEventDispatchDefault();
  bool         HasIntrinsicHoverHandlingDefault() const;
  bool         HandleHoverEventDefault(const Dali::HoverEvent& event);
  bool         HasIntrinsicTouchHandlingDefault() const;
  bool         HandleTouchEventDefault(const Dali::TouchEvent& event);
  void         FinalizeTouchEventDispatchDefault(const Dali::TouchEvent& event);
  void         HandleFocusChangedDefault(bool focused);
  void         RelayoutDefault(const Vector2& size, RelayoutContainer& container);
  View         ResolveDefaultFocusRequest();
  bool         ActivateAccessibilityDefault();
  void         OnChildAdded(Actor& child, bool allowNonViewChild);
  void         OnChildRemoved(Actor& child);
  void         OnViewSceneConnection();
  void         OnViewSceneDisconnection();
  void         OnPropertySet(Property::Index index, const Property::Value& propertyValue);
  void         OnSizeSet(const Vector3& targetSize);
  void         OnSizeAnimation(Animation& animation);
  void         OnAnimateAnimatableProperty(Animation& animation, Property::Index index, Animation::State state);
  void         OnConstraintAnimatableProperty(Constraint& constraint, Property::Index index, bool applied);
  void         OnChildOrderChanged(Actor parent, Actor orderChangedChild);

  /**
   * @brief Invalidates this view's LAYOUT after its effective layout direction
   * changed.
   *
   * Connected in ViewImpl::Initialize() to the actor's layout-direction-changed
   * signal, which dali-core emits on exactly the set of actors whose RESOLVED
   * direction changed -- the actor the direction was set on plus every
   * descendant that inherits it, including descendants sitting under a non-View
   * or root-layer ancestor. Every affected View is therefore reached
   * individually, so the handler never has to walk the subtree itself.
   *
   * Invalidates the MEASURE axis (which raises the arrange dirty with it), not
   * arrange alone. Arrange alone would be exactly correct for every first-party
   * producer -- the direction is consumed by ApplyLayoutDirection, and no in-library
   * measure producer reads it (verified across the layout managers and components;
   * text views resolve direction inside their own signal handlers) -- but
   * GetEffectiveLayoutDirection() is public and OnMeasure() is virtual, so an
   * APPLICATION's measure producer can size on it, and the measure cache key
   * (mLastMeasureConstraint) has no direction term. Invalidating measure here is what
   * lets that just work instead of becoming a contract the application has to know.
   *
   * The rejected alternative was a direction term in the measure cache KEY: that puts
   * a layout-direction read into the measure HIT predicate, which runs per view per
   * pass, whereas this handler runs only on an actual direction change. The cost paid
   * here is one re-measure of the affected subtree per locale / direction switch.
   *
   * The ARRANGE cache keeps its own recorded direction (mLastArrangeDirection) as a
   * key term regardless. That is belt and braces for a different failure: the
   * direction lives in dali-core and can be moved through actors dali-ui does not
   * own, so a missed signal must degrade to a cache MISS, never to an arrangement
   * mirrored the wrong way round.
   *
   * @param[in] actor The actor whose resolved layout direction changed (this view)
   * @param[in] type The new resolved layout direction
   */
  void OnLayoutDirectionChanged(Dali::Actor actor, Dali::LayoutDirection::Type type);

  /**
   * @brief Drops the ANCESTOR measure/arrange cache entries after this view has
   * taken a full Measure() miss, up to the nearest layout dependency boundary.
   *
   * A completed Measure() rewrites this view's stored measured size, which every
   * ancestor consumes while arranging (ArrangeDefault and the five layout
   * managers all read the stored slot). When that Measure() did not originate
   * from an ancestor's own pass -- an external View::Measure(), or a measure
   * issued from an unrelated view's producer -- the ancestors' cached results
   * were produced against the PREVIOUS slot, so an ancestor cache hit on the
   * next pass would skip re-measuring this view and then arrange it from the
   * overwritten slot.
   *
   * Cache-only: no dirty bit is raised and nothing is registered with the
   * LayoutController, so this can never schedule (or spin) a layout pass. See
   * the definition in view-data-impl.cpp for the stop conditions.
   *
   * Standalone views are excluded outright (an early return): no ancestor's
   * measured value is a function of a standalone child's slot, so there is
   * nothing for this walk to invalidate. Their slot is corrected on the ARRANGE
   * side instead -- mMeasuredSlotUnconsumed plus the corrective re-measure in
   * ArrangeStandaloneChild -- which is reached whenever the parent arranges,
   * rather than only on an ancestor's measure miss. The arrange cache-HIT path
   * keeps that reachable by testing HasUnconsumedStandaloneChild(): a parent with
   * an unconsumed standalone child cannot hit.
   */
  void InvalidateAncestorLayoutCachesForMeasureMiss();

  /**
   * @brief Declines this view's arrange cache publish for the CURRENT pass because a
   * cache-ONLY invalidation reached it mid-pass.
   *
   * Unlike a poison it registers no follow-up: a cache-only invalidation must never
   * turn into a scheduled layout. Pass-local by construction (ArrangePassGuard clears
   * the bit at pass entry, the publish gate reads it at pass exit), hence the
   * precondition that an arrange pass is actually running.
   */
  void BlockArrangeCachePublishDuringPass();

  /**
   * @brief Recomputes mArrangeProducerPure from the ACTIVE producer's own declaration.
   *
   * The declaration is the callback's (mArrangeCallbackPure), the attached
   * LayoutManager's (LayoutManager::IsArrangeProducerPure(), declared per exact
   * manager type at the manager's own construction) or this view's OnArrange
   * declaration (mArrangeOverridePure), whichever producer would actually run.
   *
   * Mirrors the producer dispatch order in Arrange() exactly -- ArrangeCallback >
   * LayoutManager > OnArrange -- so the bit always describes the code that would
   * actually run on a miss. Called only from the closed set of mutation points
   * (ViewImpl::New / SetArrangePurity / both SetArrangeCallback overloads /
   * AttachLayoutManager / OnArrangeProducerTraitChanged), never from the hit path:
   * the two lookups it performs
   * (GetArrangeCallback, GetLayoutManager) are trait lookups, one of them with a
   * dynamic_cast, which is exactly why the derived value is cached in a bit rather
   * than recomputed inside the predicate.
   */
  void RefreshArrangeProducerPurity();

  /**
   * @brief Re-derives the arrange producer purity after a reserved layout trait was
   * added, replaced or removed.
   *
   * A no-op unless @p id is ReservedTraitId::LAYOUT_SIGNALS (the ArrangeCallback) or
   * ReservedTraitId::LAYOUT_MANAGER (the LayoutManager) -- the only two traits that
   * change WHICH producer Arrange() dispatches to. For LAYOUT_SIGNALS it also clears
   * the declared callback purity, since that declaration belonged to the callback
   * object being replaced. Defensive: it exists for the public
   * Integration::View::SetTrait/RemoveTrait surface, which can reach those ids
   * without going through SetArrangeCallback()/AttachLayoutManager().
   *
   * @param[in] id The trait that changed
   */
  void OnArrangeProducerTraitChanged(TraitId id);

  /**
   * @brief Whether any DIRECT child is a standalone view whose freshly measured
   * slot this view has not consumed yet.
   *
   * A term of the arrange cache-HIT predicate, and the reason the forward note on
   * ArrangeStandaloneChildren (see view-data-impl.cpp) exists: the corrective
   * re-measure for an unconsumed standalone slot lives on the ARRANGE path, so an
   * Arrange() that returns early on a cache hit would silently skip it. Declining
   * the hit while such a child exists keeps the correction reachable; the very
   * next (missing) pass consumes the slot and clears the bit, so this can decline
   * at most one pass per out-of-band Measure().
   *
   * Cost-ordered on purpose, and the SELECTIVE term goes first:
   * mMeasuredSlotUnconsumed is set unconditionally at every measure publish and is
   * cleared only by the two standalone loops, so it is TRUE for every regular child
   * in the steady state and decides nothing. IsLayoutModeStandalone is the term that
   * actually rejects, so it is tested first and the bit only qualifies the few
   * standalone children. O(direct children), no recursion.
   *
   * @note This is NOT the guard for a NEVER-MEASURED standalone child.
   * mMeasuredSlotUnconsumed is initialised false and is raised only at a measure
   * publish, so a standalone child that was just added and has not been measured yet
   * leaves this query FALSE. What keeps that child reachable is the ARRANGE
   * invalidation ViewDataImpl::OnChildAdded issues on the standalone-child path: it
   * retracts the cache entry that was published for the older child set. This query
   * covers only the other half -- an already measured standalone child whose fresh
   * slot the parent has not consumed yet.
   *
   * @note O(direct children) is exactly the scope of this query, so on its own it
   * says nothing about a DESCENDANT holding an unconsumed slot. What extends the
   * claim to a subtree is CanReplayArrangeSubtreeFromCache(), which evaluates this
   * same term at every node it would elide and refuses the whole hit if any node
   * fails it.
   *
   * @return True when at least one direct child is standalone AND has an
   *         unconsumed measured slot
   */
  bool HasUnconsumedStandaloneChild() const;

  /**
   * @brief DEBUG self-check for the effective-scale sync bit.
   *
   * Verifies the invariant that mLogicalContextValid guarantees: when the bit is
   * true, mEffectiveScale must equal what ComputeEffectiveScale() would return
   * now. A false bit claims nothing, so it trivially holds.
   *
   * Deliberately NON-MUTATING and O(1): it must never call
   * ComputeEffectiveScale() (which recurses up the parent chain and caches on
   * every ancestor it touches), because an assertion predicate that repairs the
   * very state it is checking can never fail. For INHERIT it therefore compares
   * against the immediate parent's already-cached value and only when the
   * PARENT's own bit is live; a parent whose bit is stale carries no expectation
   * to check against, so the check is skipped (returns true).
   *
   * @return True when the bit's claim holds (or the bit makes no claim)
   */
  bool IsLogicalContextInSyncNonMutating() const;

  /**
   * @brief Drops this view's cached effective scale.
   *
   * Single concern: the CACHED SCALE only. Clears BOTH of its sync bits -- the
   * one that says the cached value is usable (mLogicalContextValid), so the next
   * GetEffectiveScale() recomputes from the (possibly re-rooted) parent chain,
   * and the one that says the ACTOR already holds that value
   * (mEffectiveScaleActorSynced), whose claim names the very value being
   * retracted -- and records the drop when it lands inside a running arrange pass
   * so that pass declines to publish a result produced against the old scale.
   *
   * It touches no cache and no dirty bit: whether dropping the scale must also
   * drop cached layout results is the caller's decision, not this function's.
   *
   * @warning A scale change DOES invalidate this view's arranged result (the
   * arrangement is scale-applied). The invariant the arrange cache relies on --
   * "mArrangeCacheValid is true only while the effective scale is unchanged
   * since publish", which lets the arrange cache-HIT predicate omit a scale term
   * (and assert mLogicalContextValid instead) -- holds ONLY because every caller
   * that drops the scale ALSO calls InvalidateLayoutCaches() on the same view.
   * Do NOT add a freshness-only caller of this alone: it would leave a valid
   * arrange cache computed against the old scale, served as a hit with no test
   * to catch it. Pair the two, or use InvalidateLogicalContextRecursive() which
   * does.
   */
  void DropCachedLogicalContext();

  /**
   * @brief Drops this view's cached measure and arrange results.
   *
   * Single concern: the CACHED RESULTS only. Both caches go together because a
   * measured size is an input to this view's own arrangement, so an arrange
   * result cannot outlive the measurement it was produced against.
   *
   * Deliberately does NOT touch mMeasureDirty. Dirty means "this view has
   * layout work that has not been consumed yet"; it is consumed at pass entry
   * (MeasurePassGuard) and is not a freshness bit, so clearing it here would
   * silently DISCARD pending work rather than invalidate a stale result. That
   * matters on the recursive path: the callers' follow-up InvalidateMeasure()
   * only re-arms the node it is called on and its ANCESTORS, so a descendant
   * whose dirty was cleared here would never get it back.
   */
  void InvalidateLayoutCaches();

  /**
   * @brief The NODE-LOCAL half of the arrange cache-HIT predicate.
   *
   * "May THIS view's arrange producer be elided for THIS input?" -- the entry exists
   * and is fresh, the producer is declared PURE, the input matches the cache KEY, the
   * effective layout direction matches, and no direct standalone child is holding an
   * unconsumed measured slot. The full, cost-ordered rationale for each term is in
   * ViewDataImpl::Arrange, which is the only caller.
   *
   * Says nothing about descendants. CanReplayArrangeSubtreeFromCache() is the other
   * half, and a hit requires both.
   *
   * @param[in] bounds The candidate arrange input
   * @return True when this view's producer may be elided for @p bounds
   */
  bool CanServeArrangeFromCache(const LayoutRect& bounds) const;

  /**
   * @brief Whether every node strictly BELOW this one may have its producer elided.
   *
   * The recursive half of the arrange cache-HIT gate. Read-only and side-effect-free:
   * it is phase one of a validate-then-replay hit, so that "hit" stays atomic. A
   * fused walk that bailed out half way would already have written cached bounds into
   * part of the subtree, and the MISS that followed would not necessarily revisit
   * every node it wrote.
   *
   * Per node it re-tests the node-local terms of CanServeArrangeFromCache() MINUS the
   * cache KEY -- a descendant has no candidate bounds, and does not need one: with
   * this view's own key matched and every producer PURE, each producer hands its
   * children the same slots as last pass, which is exactly what those children
   * resolved into the arranged bounds the replay applies. Children with no arrange
   * result are skipped, because the replay does not visit them either (a Label's
   * children, for instance).
   *
   * Cost is one read-only walk, paid only by a node whose own cache is already live
   * -- which implies no non-standalone descendant is dirty, since dirtiness
   * propagates upward. A childless view never enters it at all.
   *
   * @return True when the whole subtree below this view may be replayed from cache
   */
  bool CanReplayArrangeSubtreeFromCache() const;

  /**
   * @brief Serves the arrange cache for this view and its settled subtree.
   *
   * Phase two of the hit: a pre-order walk that performs, per node, exactly the
   * observable work an arrange MISS performs -- reconcile the actor against the
   * node's cached arranged bounds, recurse into the children that hold an arrange
   * result, mirror the direct children under RTL, mark the initial layout done and
   * register for LayoutFinished -- while eliding only the PRODUCER.
   *
   * It is NOT a prune. Skipping the subtree would drop the per-level reconciliation
   * that repairs actor geometry written outside layout, which View::Arrange documents
   * as a promise ("the arranged geometry is reconciled either way").
   *
   * @pre CanServeArrangeFromCache() holds for this view and, unless it is childless,
   *      CanReplayArrangeSubtreeFromCache() does too.
   */
  void ReplayArrangeSubtreeFromCache();

  MeasuredSize ApplyConstraints(const MeasuredSize& size) const;
  void         MeasureStandaloneChildren(float effectiveWidth, float effectiveHeight);
  void         ArrangeStandaloneChildren(const LayoutRect& bounds);
  void         ApplyLayoutDirection(float parentWidth);
  MeasuredSize DispatchMeasureWithLayoutManager(LayoutManager* manager, float widthConstraint, float heightConstraint);
  void         DispatchArrangeWithLayoutManager(LayoutManager* manager, const LayoutRect& bounds);
  LayoutRect   DispatchArrangeWithCallback(ArrangeCallback* callback, const LayoutRect& bounds);
  void         ApplySelfBoundsIfChanged(const LayoutRect& bounds);
  void         OnColorTableChanged();

  /**
   * @brief Emits the resource ready signal.
   */
  void EmitResourceReadySignal();
  /**
   * @brief Callbacks called on idle.
   *
   * @return True if we need to call this idle callback one more time.
   */
  bool OnIdleCallback();

  /**
   * Set off-screen rendering.
   * @param[in] offScreenRenderingType enum OffScreenRenderingType
   * @note When offscreen rendering is on, changing visual's depth index may not apply instantaneously. Turn it off and
   * on again.
   */
  void SetOffScreenRendering(int32_t offScreenRenderingType);

  /**
   * Notify to this view's corner radius changed.
   */
  void UpdateCornerRadius();

  /**
   * Notify to this view's borderline changed.
   */
  void UpdateBorderline();

private:
  using TraitEntries = std::vector<std::pair<TraitId, IntrusivePtr<TraitObject>>>;

  /// RAII transaction guards for a single Measure() / Arrange() pass on this view.
  /// Defined in view-data-impl.cpp; they own the pass-local in-progress / poison
  /// bits and re-arm the dirty bit when a pass is left before it publishes.
  struct MeasurePassGuard;
  struct ArrangePassGuard;

  struct SizeConstraints
  {
    float minWidth  = 0.0f;
    float minHeight = 0.0f;
    float maxWidth  = std::numeric_limits<float>::max();
    float maxHeight = std::numeric_limits<float>::max();
  };

  struct FocusNavigationData
  {
    int                     leftId             = -1;
    int                     rightId            = -1;
    int                     upId               = -1;
    int                     downId             = -1;
    int                     clockwiseId        = -1;
    int                     counterClockwiseId = -1;
    int                     forwardId          = -1;
    int                     backwardId         = -1;
    FocusNavigationCallback callback;
  };

  struct RenderEffectData
  {
    // Public effect set through View::SetRenderEffect().
    RenderEffectImplPtr renderEffect;

    // Unlike renderEffect, this handleless effect is created only by the OFFSCREEN_RENDERING property.
    std::unique_ptr<OffScreenRenderingImpl>    offScreenRendering;
    View::OffScreenRenderingType               offScreenRenderingType{View::OffScreenRenderingType::NONE};
    View::OffScreenRenderingFinishedSignalType offScreenRenderingFinishedSignal;
  };

  struct ResourceReadyData
  {
    View::ResourceReadySignalType resourceReadySignal;
    CallbackBase*                 idleCallback{nullptr};
    bool                          isEmittingResourceReadySignal{false};
    bool                          idleCallbackRegistered{false};
  };

  struct LayoutTransitionData
  {
    LayoutTransition              transition;
    std::unordered_set<ViewImpl*> pendingEnterChildren;
    std::unordered_set<ViewImpl*> pendingReorderedChildren;
    bool                          hasPendingChildRemoval{false};
  };

  SizeConstraints& EnsureSizeConstraints()
  {
    if(!mSizeConstraints)
    {
      mSizeConstraints = std::make_unique<SizeConstraints>();
    }
    return *mSizeConstraints;
  }

  FocusNavigationData& EnsureFocusNavigationData()
  {
    if(!mFocusNavigationData)
    {
      mFocusNavigationData = std::make_unique<FocusNavigationData>();
    }
    return *mFocusNavigationData;
  }

  RenderEffectData& EnsureRenderEffectData()
  {
    if(!mRenderEffectData)
    {
      mRenderEffectData = std::make_unique<RenderEffectData>();
    }
    return *mRenderEffectData;
  }

  ResourceReadyData& EnsureResourceReadyData()
  {
    if(!mResourceReadyData)
    {
      mResourceReadyData = std::make_unique<ResourceReadyData>();
    }
    return *mResourceReadyData;
  }

  LayoutTransitionData& EnsureLayoutTransitionData()
  {
    if(!mLayoutTransitionData)
    {
      mLayoutTransitionData = std::make_unique<LayoutTransitionData>();
    }
    return *mLayoutTransitionData;
  }

  bool HasLayoutTransition() const
  {
    return mLayoutTransitionData && mLayoutTransitionData->transition;
  }

  int GetFocusNavigationId(int FocusNavigationData::* field) const
  {
    return mFocusNavigationData ? mFocusNavigationData.get()->*field : -1;
  }

  ViewImpl&                            mViewImpl;
  ViewState                            mState;
  UiScalePolicy                        mScalePolicy{UiScalePolicy::INHERIT};
  mutable float                        mEffectiveScale{1.0f}; ///< Cached effective scale. Carries NO validity sentinel of its own; whether this value is usable is recorded by mLogicalContextValid. Mutable because the lazy (re)compute happens inside the const GetEffectiveScale().
  TraitEntries                         mTraits;
  Internal::CoreInteractionObject*     mCoreInteractionObject;
  std::unique_ptr<VisualData>          mVisualData;
  std::unique_ptr<AttachmentContainer> mAttachments;
  std::unique_ptr<FocusNavigationData> mFocusNavigationData;
  std::unique_ptr<RenderEffectData>    mRenderEffectData;
  std::unique_ptr<ResourceReadyData>   mResourceReadyData;
  View::StateChangedSignalType         mStateChangedSignal;
  View::KeyEventSignalType             mKeyEventSignal;
  View::FocusChangedSignalType         mFocusChangedSignal;
  View::LayoutFinishedSignalType       mLayoutFinishedSignal;

  float        mRequestedX;
  float        mRequestedY;
  MeasuredSize mMeasuredSize;          ///< Last completed measure result. Always readable (GetMeasuredSize() and layout managers consume it during Arrange regardless of cache state); mMeasureCacheValid only governs whether the KEY below may serve a cache hit.
  MeasuredSize mLastMeasureConstraint; ///< Pure cache KEY: the effective natural constraint the cached mMeasuredSize was produced for. Carries no dirty/never-measured sentinel meaning; validity lives in mMeasureCacheValid / mMeasureDirty.
  LayoutRect   mArrangedBounds;
  LayoutRect   mLastArrangeInput; ///< Pure cache KEY: the input bounds mArrangedBounds was produced for. Valid only while mArrangeCacheValid is true.
  /// @name Invalidation propagation records
  /// The epoch in which this view's last InvalidateMeasure() / InvalidateArrange()
  /// walked its ancestor chain to a layout root and registered it. While a record
  /// still equals LayoutInvalidation::CurrentEpoch(), that registration is known to
  /// be live and not yet processed, so a further invalidation on the SAME axis may
  /// skip the walk entirely. 0 = never propagated.
  ///
  /// The two axes are separate and must not be merged. An arrange walk marks the
  /// ancestors' arrange dirty but leaves their MEASURE caches valid, so an
  /// InvalidateMeasure() that skipped its walk on the strength of an arrange record
  /// would leave every ancestor's measure hitting -- and an ancestor measure hit does
  /// not re-measure its children, so this view's new measured size would never be
  /// computed at all.
  /// @{
  uint32_t mMeasurePropagationEpoch;
  uint32_t mArrangePropagationEpoch;
  /// @}

  Dali::LayoutDirection::Type           mLastArrangeDirection;         ///< Pure cache KEY: the effective layout direction mArrangedBounds was produced under. Valid only while mArrangeCacheValid is true. Unlike the effective scale -- whose freshness is carried by a sync bit this class owns -- the direction lives in dali-core and can be moved through actors dali-ui does not own, so it is recorded as a KEY: a missed invalidation then degrades to "no cache hit", never to a wrong result.
  Insets                                mMargin;                       ///< Layout margin
  Insets                                mPadding;                      ///< Layout padding
  float                                 mRequestedWidth;               ///< Requested width (WRAP_CONTENT = -1.0f, MATCH_PARENT = -2.0f)
  float                                 mRequestedHeight;              ///< Requested height (WRAP_CONTENT = -1.0f, MATCH_PARENT = -2.0f)
  LayoutMode                            mLayoutMode;                   ///< Layout mode of the view
  Vector2                               mSize;                         ///< The size of the view
  Vector2                               mLastArrangedRenderEffectSize; ///< Self size last seen by ApplySelfBoundsIfChanged, used only to dedupe render-effect refresh (kept separate from mSize, which Process()/ApplyFittingMode rely on)
  std::unique_ptr<SizeConstraints>      mSizeConstraints;              ///< Lazy-allocated measurement min/max bounds (natural units).
  Dali::Vector<View>                    mChildren;                     ///< Synchronized with Actor hierarchy via OnChildAdd/OnChildRemove.
  std::unique_ptr<LayoutTransitionData> mLayoutTransitionData;

  std::unique_ptr<AccessibilityData> mAccessibilityData;
  AccessibleObjectCreator            mAccessibleObjectCreator;
  int32_t                            mAccessibilityRole : Dali::Log<static_cast<uint32_t>(Accessibility::Role::MAX_COUNT)>::value + 2; ///< Frequently touched accessibility-related value kept here to avoid AccessibilityData creation.

  bool         mSkipChildrenUpdate : 1;
  bool         mMeasureCacheValid : 1;                            ///< True when mLastMeasureConstraint + mMeasuredSize hold a usable cache entry.
  bool         mMeasureDirty : 1;                                 ///< True when invalidated since the last measure.
  bool         mMeasureInProgress : 1;                            ///< True while this view's own Measure() is on the stack.
  bool         mMeasurePassPoisoned : 1;                          ///< True when an invalidation arrived while this view's measure pass was running.
  bool         mMeasureResultAvailable : 1;                       ///< True once at least one measure pass has published a result into mMeasuredSize.
  bool         mMeasuredSlotUnconsumed : 1;                       ///< True while the measured size published by the last completed measure pass has not been consumed by this view's parent. Set unconditionally at the publish; cleared by the parent in MeasureStandaloneChildren / ArrangeStandaloneChildren. Read only on the standalone path: it tells ArrangeStandaloneChild that the slot may be the leftover of an out-of-band Measure() and must be re-measured against the parent's extent before it is placed.
  bool         mArrangeCacheValid : 1;                            ///< True when mLastArrangeInput + mArrangedBounds hold a usable cache entry.
  bool         mArrangeDirty : 1;                                 ///< True when invalidated since the last arrange.
  bool         mArrangeInProgress : 1;                            ///< True while this view's own Arrange() is on the stack; guards same-view re-entrancy.
  bool         mArrangePassPoisoned : 1;                          ///< True when an invalidation arrived while this view's arrange pass was running.
  bool         mArrangeCacheBlockedDuringPass : 1;                ///< True when a cache-ONLY invalidation arrived while this view's arrange pass was running. Declines the cache publish without poisoning the pass, so no follow-up layout is registered. Set by InvalidateAncestorLayoutCachesForMeasureMiss on an unowned arrange-in-progress ancestor; see BlockArrangeCachePublishDuringPass.
  bool         mArrangeResultAvailable : 1;                       ///< True once at least one arrange pass has published a result into mArrangedBounds.
  bool         mArrangeOverridePure : 1;                          ///< Purity DECLARED for this view's OnArrange(), via ViewImpl::SetArrangePurity(). True for a plain View (declared in ViewImpl::New(), where the producer is provably ViewImpl::OnArrange -> ArrangeDefault). Default FALSE, so an undeclared subclass override is never skipped.
  bool         mArrangeCallbackPure : 1;                          ///< Purity DECLARED for the ArrangeCallback currently installed, via the two-argument SetArrangeCallback(). Reset to FALSE by the one-argument overload, so installing a callback always clears any previously declared callback purity. Default FALSE.
  bool         mArrangeProducerPure : 1;                          ///< DERIVED from which producer is ACTIVE plus that producer's own declaration -- the two bits above, or, when a LayoutManager is the producer, LayoutManager::IsArrangeProducerPure() (see RefreshArrangeProducerPurity). The single term the arrange cache-HIT predicate reads for producer purity. Default FALSE: an undeclared producer is never served from cache.
  mutable bool mLogicalContextValid : 1;                          ///< THE sync bit for mEffectiveScale: true exactly when mEffectiveScale equals what ComputeEffectiveScale() would return now. Set by the lazy compute in the const GetEffectiveScale() (hence mutable), cleared by every scale-context invalidation.
  bool         mEffectiveScaleActorSynced : 1;                    ///< THE sync bit for the ACTOR-side copy of the scale (the animatable VIEW_EFFECTIVE_SCALE property): true exactly when that property is known to hold mEffectiveScale. The second half of the pair whose first half is mLogicalContextValid -- that one says the CACHED scale is usable, this one says the ACTOR already has it. Set by Measure()'s push (after the write, which re-enters the clear below), cleared by DropCachedLogicalContext() (the value it names has been retracted) and by ViewDataImpl::SetProperty for that index, which is the single funnel every event-side write of the property passes through. Default false, so the first Measure() always pushes.
  bool         mLogicalContextPoisonedDuringPass : 1;             ///< True when the logical context was invalidated while an arrange pass was running.
  bool         mInitialLayoutDone : 1;                            ///< True after this view has completed at least one arrange pass; used by the dispatcher to suppress ENTER on initial mount
  bool         mIsFocusGroup : 1;                                 ///< Stores whether the view is a focus group.
  bool         mDispatchKeyEvents : 1;                            ///< Whether the actor emits key event signals
  bool         mAccessibleCreatable : 1;                          ///< Whether we can create new accessible or not.
  bool         mProcessorRegistered : 1;                          ///< Whether the processor is registered.
  bool         mFittingModeLayoutFinishedSignalConnected : 1;     ///< Whether layout-finished signal is connected for fitting mode update.
  bool         mDefaultFocusIndicatorSuppressedByStateEffect : 1; ///< Whether the current StateEffect suppresses the default focus indicator.

  /// Kept OUT of the bit-field run above, and placed after it rather than inside it:
  /// ScopedTrueFlag binds a `bool&`, which a bit-field cannot provide, so this one has
  /// to be a whole bool. Sitting between two bit-fields it would split their shared
  /// allocation unit in two and cost several bytes per View on every compiler; at the
  /// end it costs one.
  bool mKeyEventDispatchInProgress; ///< True while this view's key event dispatch is on the stack; guards unsupported same-view re-entrancy.

  static constexpr uint32_t VIEW_BEHAVIOUR_FLAG_COUNT = Dali::Log<static_cast<uint32_t>(ViewImpl::LAST_VIEW_BEHAVIOUR_FLAG) - 1>::value + 1;
  ViewImpl::ViewBehaviour   mFlags : VIEW_BEHAVIOUR_FLAG_COUNT; ///< Flags passed in from constructor.

  // Property registrations access private methods and data of ViewImpl and ViewDataImpl.
  static const PropertyRegistration           PROPERTY_1;
  static const PropertyRegistration           PROPERTY_2;
  static const PropertyRegistration           PROPERTY_3;
  static const PropertyRegistration           PROPERTY_5;
  static const PropertyRegistration           PROPERTY_6;
  static const PropertyRegistration           PROPERTY_7;
  static const PropertyRegistration           PROPERTY_8;
  static const PropertyRegistration           PROPERTY_9;
  static const PropertyRegistration           PROPERTY_10;
  static const PropertyRegistration           PROPERTY_11;
  static const PropertyRegistration           PROPERTY_12;
  static const PropertyRegistration           PROPERTY_13;
  static const PropertyRegistration           PROPERTY_14;
  static const PropertyRegistration           PROPERTY_15;
  static const PropertyRegistration           PROPERTY_22;
  static const PropertyRegistration           PROPERTY_24;
  static const PropertyRegistration           PROPERTY_25;
  static const PropertyRegistration           PROPERTY_31;
  static const PropertyRegistration           PROPERTY_32;
  static const PropertyRegistration           PROPERTY_33;
  static const PropertyRegistration           PROPERTY_34;
  static const PropertyRegistration           PROPERTY_35;
  static const PropertyRegistration           PROPERTY_36;
  static const PropertyRegistration           PROPERTY_37;
  static const PropertyRegistration           PROPERTY_38;
  static const PropertyRegistration           PROPERTY_39;
  static const PropertyRegistration           PROPERTY_40;
  static const PropertyRegistration           PROPERTY_42;
  static const PropertyRegistration           PROPERTY_43;
  static const PropertyRegistration           PROPERTY_44;
  static const AnimatablePropertyRegistration ANIMATABLE_PROPERTY_1;
  static const AnimatablePropertyRegistration ANIMATABLE_PROPERTY_2;
  static const AnimatablePropertyRegistration ANIMATABLE_PROPERTY_3;
  static const AnimatablePropertyRegistration ANIMATABLE_PROPERTY_4;
  static const AnimatablePropertyRegistration ANIMATABLE_PROPERTY_5;
  static const AnimatablePropertyRegistration ANIMATABLE_PROPERTY_6;
  static const AnimatablePropertyRegistration ANIMATABLE_PROPERTY_7;
};

} // namespace Internal

} // namespace Ui

} // namespace Dali

#endif // DALI_UI_VIEW_DATA_IMPL_H
