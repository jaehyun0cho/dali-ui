/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
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
 */

#include "window-frame-view-tree.h"
#include "window-frame-layout-engine.h"
#include "window-frame-style-resolver.h"

namespace DALI_NAMESPACE::Ui::Internal
{
namespace
{
// AbsoluteLayoutManager multiplies a child's bounds by the child's own effective scale, so a
// rectangle in window surface pixels is handed to @p view in that view's natural units.
Dali::Ui::AbsoluteLayoutParams MakeBoundsLayoutParams(const Dali::Ui::View& view, const Dali::Ui::LayoutRect& rect)
{
  const float scale = Dali::Ui::GetImpl(view).GetEffectiveLayoutScale();
  return Dali::Ui::AbsoluteLayoutParams::New().SetBounds(Dali::Ui::LayoutRect(rect.x / scale, rect.y / scale, rect.width / scale, rect.height / scale));
}
} //namespace

void WindowFrameViewTree::Create(const WindowFrameStyle& style)
{
  if(mSurfaceRoot)
  {
    return;
  }

  mSurfaceRoot = Dali::Ui::AbsoluteLayout::New();
  mSurfaceRoot.SetRequestedWidth(Dali::Ui::MATCH_PARENT);
  mSurfaceRoot.SetRequestedHeight(Dali::Ui::MATCH_PARENT);

  mFrameEffectVisual = Dali::Ui::AbsoluteLayout::New();
  mFrameEffectVisual.SetSensitive(false);
  mSurfaceRoot.Add(mFrameEffectVisual);

  mFrameRoot = Dali::Ui::AbsoluteLayout::New();
  mFrameRoot.SetClippingMode(Dali::ClippingMode::CLIP_CHILDREN);
  mSurfaceRoot.Add(mFrameRoot);

  mFrameBackground = Dali::Ui::View::New();
  mFrameBackground.SetSensitive(false);
  mFrameRoot.Add(mFrameBackground);

  mContentRoot = Dali::Ui::AbsoluteLayout::New();
  mContentRoot.SetClippingMode(Dali::ClippingMode::CLIP_CHILDREN);
  mFrameRoot.Add(mContentRoot);

  mDecorationRoot = Dali::Ui::AbsoluteLayout::New();
  mFrameRoot.Add(mDecorationRoot);

  SetFrameStyle(style);
  AddDecorationViews();
}

void WindowFrameViewTree::Destroy()
{
  Unparent();
  mDecoration         = {};
  mDecorationRoot     = {};
  mContentRoot        = {};
  mFrameBackground    = {};
  mFrameRoot          = {};
  mFrameEffectContent = {};
  mFrameEffectVisual  = {};
  mSurfaceRoot        = {};
}

void WindowFrameViewTree::Unparent()
{
  if(mSurfaceRoot)
  {
    mSurfaceRoot.Unparent();
  }
}

void WindowFrameViewTree::SetDecoration(const WindowFrameDecoration& decoration)
{
  RemoveDecorationViews();
  mDecoration = decoration;
  AddDecorationViews();
}

void WindowFrameViewTree::SetFrameStyle(const WindowFrameStyle& style)
{
  if(mFrameBackground)
  {
    mFrameBackground.SetBackgroundColor(style.GetFrameBackgroundColor());
    mFrameBackground.SetCornerRadius(style.GetFrameCornerRadius());
    mFrameBackground.SetCornerRadiusPolicy(style.GetFrameCornerRadiusPolicy());
  }
  if(mFrameRoot)
  {
    mFrameRoot.SetCornerRadius(style.GetFrameCornerRadius());
    mFrameRoot.SetCornerRadiusPolicy(style.GetFrameCornerRadiusPolicy());
  }
  if(!mFrameEffectVisual)
  {
    return;
  }

  if(mFrameEffectContent)
  {
    mFrameEffectContent.Unparent();
    mFrameEffectContent.Reset();
  }

  const ResolvedWindowFrameStyle resolved = ResolveWindowFrameStyle(style, false);
  if(resolved.shadowSource == WindowFrameShadowSource::COLOR)
  {
    Dali::Ui::View shadowView = Dali::Ui::View::New();
    shadowView.SetShadow(style.GetShadow());
    shadowView.SetCornerRadius(style.GetFrameCornerRadius());
    shadowView.SetCornerRadiusPolicy(style.GetFrameCornerRadiusPolicy());
    mFrameEffectContent = shadowView;
  }
  else if(resolved.shadowSource == WindowFrameShadowSource::IMAGE)
  {
    Dali::Ui::ImageView shadowImage = Dali::Ui::ImageView::New(style.GetShadowImageUrl());
    shadowImage.SetNPatchBorder(style.GetShadowImageBorder());
    shadowImage.SetNPatchBorderOnly(style.IsShadowImageBorderOnly());
    mFrameEffectContent = shadowImage;
  }

  if(mFrameEffectContent)
  {
    mFrameEffectContent.SetSensitive(false);
    mFrameEffectVisual.Add(mFrameEffectContent);
  }
}

void WindowFrameViewTree::ApplyLayout(const Dali::PositionSize& positionSize,
                                      const WindowFrameLayout&  frameLayout,
                                      bool                      maximized,
                                      bool                      decorationOverlay,
                                      const WindowFrameStyle&   style,
                                      float                     scale)
{
  if(!mSurfaceRoot)
  {
    return;
  }

  // The surface is laid out in window surface pixels, so the frame's own natural lengths are
  // taken at the frame's effective scale before they meet it.
  const ResolvedWindowFrameStyle resolvedStyle = ResolveWindowFrameStyle(style, maximized);
  const WindowFrameLayoutResult  layout        = WindowFrameLayoutEngine::Calculate(static_cast<float>(positionSize.width),
                                                                                    static_cast<float>(positionSize.height),
                                                                                    WindowFrameLayoutEngine::ScaleLayout(frameLayout, scale),
                                                                                    WindowFrameLayoutOptions{maximized, decorationOverlay, WindowFrameLayoutEngine::ScaleInsets(resolvedStyle.shadowOutsets, scale)});
  const Dali::Ui::LayoutRect     frame         = layout.GetFrame();
  const Dali::Ui::LayoutRect     effect        = layout.GetEffect();
  const Dali::Ui::LayoutRect     frameLocalBounds(0.0f, 0.0f, frame.width, frame.height);

  mFrameEffectVisual.SetLayoutParams(MakeBoundsLayoutParams(mFrameEffectVisual, effect));
  mFrameEffectVisual.SetVisible(resolvedStyle.shadowVisible);
  if(mFrameEffectContent)
  {
    const Dali::Ui::LayoutRect effectContentBounds = resolvedStyle.shadowSource == WindowFrameShadowSource::COLOR ? frame : effect;
    mFrameEffectContent.SetLayoutParams(MakeBoundsLayoutParams(mFrameEffectContent, effectContentBounds));
    if(resolvedStyle.shadowSource == WindowFrameShadowSource::COLOR)
    {
      mFrameEffectContent.SetCornerRadius(maximized ? Dali::Vector4::ZERO : style.GetFrameCornerRadius());
    }
  }
  mFrameRoot.SetLayoutParams(MakeBoundsLayoutParams(mFrameRoot, frame));
  mFrameRoot.SetCornerRadius(maximized ? Dali::Vector4::ZERO : style.GetFrameCornerRadius());
  mFrameBackground.SetCornerRadius(maximized ? Dali::Vector4::ZERO : style.GetFrameCornerRadius());
  mFrameBackground.SetLayoutParams(MakeBoundsLayoutParams(mFrameBackground, frameLocalBounds));
  mContentRoot.SetLayoutParams(MakeBoundsLayoutParams(mContentRoot, WindowFrameLayoutEngine::SurfaceToFrameLocal(layout.GetContent(), frame)));
  mDecorationRoot.SetLayoutParams(MakeBoundsLayoutParams(mDecorationRoot, frameLocalBounds));
  if(mDecoration.GetTopSlot()) mDecoration.GetTopSlot().SetLayoutParams(MakeBoundsLayoutParams(mDecoration.GetTopSlot(), WindowFrameLayoutEngine::SurfaceToFrameLocal(layout.GetTop(), frame)));
  if(mDecoration.GetBottomSlot()) mDecoration.GetBottomSlot().SetLayoutParams(MakeBoundsLayoutParams(mDecoration.GetBottomSlot(), WindowFrameLayoutEngine::SurfaceToFrameLocal(layout.GetBottom(), frame)));
  if(mDecoration.GetLeftSlot()) mDecoration.GetLeftSlot().SetLayoutParams(MakeBoundsLayoutParams(mDecoration.GetLeftSlot(), WindowFrameLayoutEngine::SurfaceToFrameLocal(layout.GetLeft(), frame)));
  if(mDecoration.GetRightSlot()) mDecoration.GetRightSlot().SetLayoutParams(MakeBoundsLayoutParams(mDecoration.GetRightSlot(), WindowFrameLayoutEngine::SurfaceToFrameLocal(layout.GetRight(), frame)));
  if(mDecoration.GetBottomLeftResizeHandle()) mDecoration.GetBottomLeftResizeHandle().SetLayoutParams(MakeBoundsLayoutParams(mDecoration.GetBottomLeftResizeHandle(), WindowFrameLayoutEngine::SurfaceToFrameLocal(layout.GetBottomLeftResizeHandle(), frame)));
  if(mDecoration.GetBottomRightResizeHandle()) mDecoration.GetBottomRightResizeHandle().SetLayoutParams(MakeBoundsLayoutParams(mDecoration.GetBottomRightResizeHandle(), WindowFrameLayoutEngine::SurfaceToFrameLocal(layout.GetBottomRightResizeHandle(), frame)));
}

void WindowFrameViewTree::SetDecorationVisible(bool visible)
{
  if(mDecorationRoot)
  {
    mDecorationRoot.SetVisible(visible);
    mDecorationRoot.SetSensitive(visible);
  }
}

Dali::Ui::AbsoluteLayout WindowFrameViewTree::GetSurfaceRoot() const
{
  return mSurfaceRoot;
}

Dali::Ui::View WindowFrameViewTree::GetContentRoot() const
{
  return mContentRoot;
}

Dali::Ui::AbsoluteLayout WindowFrameViewTree::GetDecorationRoot() const
{
  return mDecorationRoot;
}

WindowFrameDecoration& WindowFrameViewTree::GetDecoration()
{
  return mDecoration;
}

const WindowFrameDecoration& WindowFrameViewTree::GetDecoration() const
{
  return mDecoration;
}

void WindowFrameViewTree::AddDecorationViews()
{
  if(!mDecorationRoot)
  {
    return;
  }
  if(mDecoration.GetTopSlot()) mDecorationRoot.Add(mDecoration.GetTopSlot());
  if(mDecoration.GetBottomSlot()) mDecorationRoot.Add(mDecoration.GetBottomSlot());
  if(mDecoration.GetLeftSlot()) mDecorationRoot.Add(mDecoration.GetLeftSlot());
  if(mDecoration.GetRightSlot()) mDecorationRoot.Add(mDecoration.GetRightSlot());
  if(mDecoration.GetBottomLeftResizeHandle()) mDecorationRoot.Add(mDecoration.GetBottomLeftResizeHandle());
  if(mDecoration.GetBottomRightResizeHandle()) mDecorationRoot.Add(mDecoration.GetBottomRightResizeHandle());
}

void WindowFrameViewTree::RemoveDecorationViews()
{
  if(mDecoration.GetTopSlot()) mDecoration.GetTopSlot().Unparent();
  if(mDecoration.GetBottomSlot()) mDecoration.GetBottomSlot().Unparent();
  if(mDecoration.GetLeftSlot()) mDecoration.GetLeftSlot().Unparent();
  if(mDecoration.GetRightSlot()) mDecoration.GetRightSlot().Unparent();
  if(mDecoration.GetBottomLeftResizeHandle()) mDecoration.GetBottomLeftResizeHandle().Unparent();
  if(mDecoration.GetBottomRightResizeHandle()) mDecoration.GetBottomRightResizeHandle().Unparent();
}

} //namespace DALI_NAMESPACE::Ui::Internal
