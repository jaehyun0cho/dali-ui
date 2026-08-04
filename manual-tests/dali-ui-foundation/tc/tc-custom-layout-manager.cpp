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

#include "manual-test-case.h"

using namespace Dali;
using namespace Dali::Ui;

namespace
{
/**
 * Diagonal arrangement implemented by subclassing LayoutManager directly and
 * attaching it to a plain View via View::AttachLayoutManager.
 *
 * Three children (50x50, 100x100, 200x200) are placed diagonally so that
 * each child's top-left corner touches the bottom-right corner of the
 * previous child.
 *
 *  +--+
 *  |50|
 *  +--+----+
 *     |100 |
 *     |    |
 *     +----+--------+
 *          |  200   |
 *          |        |
 *          |        |
 *          +--------+
 */
class DiagonalLayoutManager : public LayoutManager
{
public:
  DiagonalLayoutManager() = default;

  MeasuredSize Measure(ViewImpl* view, float widthConstraint, float heightConstraint) override
  {
    float          totalWidth  = 0.0f;
    float          totalHeight = 0.0f;
    const uint32_t count       = GetChildViewCount(view);

    for(uint32_t i = 0; i < count; ++i)
    {
      View      child     = GetChildViewAt(view, i);
      ViewImpl& childImpl = GetImpl(child);

      if(IsStandalone(&childImpl))
      {
        continue;
      }

      MeasuredSize sz = childImpl.Measure(widthConstraint - totalWidth,
                                          heightConstraint - totalHeight);
      totalWidth += sz.width;
      totalHeight += sz.height;
    }

    return {totalWidth, totalHeight};
  }

  void Arrange(ViewImpl* view, const LayoutRect& bounds) override
  {
    float          x     = bounds.x;
    float          y     = bounds.y;
    const uint32_t count = GetChildViewCount(view);

    for(uint32_t i = 0; i < count; ++i)
    {
      View      child     = GetChildViewAt(view, i);
      ViewImpl& childImpl = GetImpl(child);

      if(IsStandalone(&childImpl))
      {
        continue;
      }

      MeasuredSize sz = childImpl.GetMeasuredSize();
      childImpl.Arrange({x, y, sz.width, sz.height});
      x += sz.width;
      y += sz.height;
    }

  }
};
} // namespace

/**
 * @brief Verifies View::AttachLayoutManager with a LayoutManager subclass.
 *
 * A plain View delegates Measure/Arrange to DiagonalLayoutManager for every
 * layout pass, producing the same diagonal arrangement that
 * "CustomLayout: Diagonal" obtains from measure / arrange callbacks.
 *
 * Ported from samples/custom-layout-manager/custom-layout-manager-example.cpp.
 */
class TcCustomLayoutManager : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "CustomLayoutManager: Diagonal (LayoutManager Subclass)";
  }

  Dali::String GetDescription() const override
  {
    return "View::AttachLayoutManager with a LayoutManager subclass";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Attach the custom LayoutManager directly to a plain View. The View now
    // delegates Measure/Arrange to DiagonalLayoutManager for every layout pass.
    // A fresh View and a fresh manager are created on every entry because
    // AttachLayoutManager takes ownership of the manager.
    View root = View::New();
    root.AttachLayoutManager(Dali::MakeUnique<DiagonalLayoutManager>());

    View child1 = View::New();
    child1.SetRequestedWidth(50.0f);
    child1.SetRequestedHeight(50.0f);
    child1.SetBackgroundColor(Vector4(0.9f, 0.2f, 0.2f, 1.0f));

    View child2 = View::New();
    child2.SetRequestedWidth(100.0f);
    child2.SetRequestedHeight(100.0f);
    child2.SetBackgroundColor(Vector4(0.2f, 0.7f, 0.2f, 1.0f));

    View child3 = View::New();
    child3.SetRequestedWidth(200.0f);
    child3.SetRequestedHeight(200.0f);
    child3.SetBackgroundColor(Vector4(0.2f, 0.3f, 0.9f, 1.0f));

    root.Add(child1);
    root.Add(child2);
    root.Add(child3);

    mBackdrop.Add(root);
    contentArea.Add(mBackdrop);
  }

  void OnExit() override
  {
    mBackdrop.Reset();
    ResetState();
  }

private:
  void ResetState()
  {
    // No mutable scalar state; every handle is reassigned in OnEnter.
  }

  View mBackdrop;
};

REGISTER_MANUAL_TEST(TcCustomLayoutManager)
