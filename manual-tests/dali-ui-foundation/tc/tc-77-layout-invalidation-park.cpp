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
 */

// Exercises a finite in-pass text mutation chain. Each successful pass emits
// LayoutFinished while retained invalidation receives an automatic, paced retry.
// No application timer or event pump assists the controller. Resource processing
// can also trigger events; TC95 separately isolates a quiet, resource-free producer.
// ArrangePolicy::ALWAYS keeps this deliberate producer active despite equal bounds.

#include "manual-test-case.h"

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/layouts/layout-controller.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout.h>
#include <dali-ui-foundation/public-api/views/text-controls/label.h>
#include <dali/public-api/adaptor-framework/ui-context.h>

#include <iostream>
#include <string>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
constexpr int ARRANGE_LIMIT = 30; ///< Mutation stops here so quiet completion is observable too.

int   gArrangeCount  = 0;
int   gFinishedCount = 0;
bool  gMutating      = true;
Label gCounterLabel; ///< The label whose text is rewritten from inside the arrange pass.

Label MakeLabel(const Dali::String& text, float fontSize, uint32_t color)
{
  Label label = Label::New(text);
  label.SetRequestedWidth(MATCH_PARENT);
  label.SetRequestedHeight(WRAP_CONTENT);
  label.SetFontSize(fontSize);
  label.SetTextColor(UiColor(color));
  return label;
}

// The in-pass producer. Runs inside the box's arrange; the SetText() below
// invalidates the label's measure while the pass is on the stack, so the
// resulting layout work is retained for an automatically scheduled continuation.
LayoutRect BoxArrange(View /*view*/, const LayoutRect& bounds)
{
  ++gArrangeCount;
  if(gMutating && gCounterLabel)
  {
    const std::string text = "arrange #" + std::to_string(gArrangeCount) +
                             " - automatic continuation";
    gCounterLabel.SetText(Dali::String(text.c_str()));
    std::cout << "TC77_ARRANGE count=" << gArrangeCount
              << " mutating=1" << std::endl;
    if(gArrangeCount >= ARRANGE_LIMIT)
    {
      gMutating = false;
      std::cout << "producer stopped mutating; the final retained request will run automatically" << std::endl;
    }
  }
  return bounds;
}
} // namespace

class TcLayoutInvalidationPark : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "77. Layout Invalidation: Automatic Continuation";
  }

  Dali::String GetDescription() const override
  {
    return "Verify finite in-pass text changes progress automatically";
  }

  void OnEnter(View contentArea) override
  {
    gArrangeCount  = 0;
    gFinishedCount = 0;
    gMutating      = true;

    Window window = UiContext::Get().GetDefaultWindow();

    StackLayout root = StackLayout::New(StackOrientation::VERTICAL);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetSpacing(12.0f);
    root.SetPadding(Insets(24.0f, 24.0f, 24.0f, 24.0f));

    root.Add(MakeLabel("Automatic in-pass continuation", 24.0f, 0x202124u));
    root.Add(MakeLabel("Every arrange rewrites the label below from OnArrange.", 14.0f, 0x5F6368u));
    root.Add(MakeLabel("Watch: counts advance automatically, then stop.", 14.0f, 0x5F6368u));

    gCounterLabel = MakeLabel("arrange #0 - waiting for the first pass", 18.0f, 0x0B57D0u);
    root.Add(gCounterLabel);

    // The producer view. Its ArrangeCallback replaces its OnArrange, which is
    // fine for a childless box; the label it mutates is a SIBLING, so the
    // mutation is an in-pass invalidation of another view.
    View box = View::New();
    box.SetRequestedWidth(MATCH_PARENT);
    box.SetRequestedHeight(80.0f);
    box.SetBackgroundColor(UiColor(0xE8F0FEu));
    box.SetArrangeCallback(ArrangeCallback::New(&BoxArrange), ArrangePolicy::ALWAYS);
    root.Add(box);

    contentArea.Add(root);

    LayoutController::Get(window).LayoutFinishedSignal().Connect(this, &TcLayoutInvalidationPark::OnLayoutFinished);
  }

  void OnLayoutFinished(Window /*window*/)
  {
    // Observation only: logging must not invalidate layout or request another pass.
    std::cout << "TC77_PASS finished=" << ++gFinishedCount
              << " arrange=" << gArrangeCount
              << " mutating=" << (gMutating ? 1 : 0) << std::endl;
  }

  void OnExit() override
  {
    DisconnectAll();
    gCounterLabel.Reset();
    gArrangeCount  = 0;
    gFinishedCount = 0;
    gMutating      = true;
  }
};

REGISTER_MANUAL_TEST(TcLayoutInvalidationPark)
