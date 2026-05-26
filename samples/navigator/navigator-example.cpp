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

#include <dali-ui-components/dali-ui-components.h>
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <functional>
#include <sstream>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
UiColor PageBackground(uint32_t index)
{
  switch(index % 4u)
  {
    case 0u:
      return UiColor(0xEEF4FF);
    case 1u:
      return UiColor(0xEAF8EF);
    case 2u:
      return UiColor(0xFFF4DF);
    default:
      return UiColor(0xF4EAFE);
  }
}

UiColor PageAccent(uint32_t index)
{
  switch(index % 4u)
  {
    case 0u:
      return UiColor(0x2563EB);
    case 1u:
      return UiColor(0x16803C);
    case 2u:
      return UiColor(0xB45309);
    default:
      return UiColor(0x7C3AED);
  }
}
} // namespace

class NavigatorExample : public ConnectionTracker
{
public:
  explicit NavigatorExample(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &NavigatorExample::Create);
  }

  void Create(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::WHITE);

    mNavigator = Navigator::New()
                   .SetRequestedWidth(MATCH_PARENT)
                   .SetRequestedHeight(MATCH_PARENT);

    Layout root = Layout::New()
                    .SetRequestedWidth(MATCH_PARENT)
                    .SetRequestedHeight(MATCH_PARENT);

    root.Add(mNavigator);
    root.Add(CreateToolbar());
    window.Add(root);

    mNavigator.Push(CreatePage("Root", 0u), false);
    UpdateStatus();
  }

private:
  View CreateToolbar()
  {
    Layout toolbar = Layout::New()
                       .SetLayoutMode(LayoutMode::STANDALONE)
                       .SetRequestedWidth(MATCH_PARENT)
                       .SetRequestedHeight(96_spx)
                       .SetRequestedPositionY(0_spx)
                       .SetBackgroundColor(UiColor(0x111827));

    toolbar.Add(CreateButton("Push", 16_spx, 24_spx, UiColor(0x2563EB), [this](View, InputEvent) -> bool {
      ++mPageIndex;
      mNavigator.Push(CreatePage(MakePageTitle(), mPageIndex), true);
      UpdateStatus();
      return true;
    }));

    toolbar.Add(CreateButton("Pop", 136_spx, 24_spx, UiColor(0x374151), [this](View, InputEvent) -> bool {
      mNavigator.Pop(true);
      UpdateStatus();
      return true;
    }));

    toolbar.Add(CreateButton("Modal", 256_spx, 24_spx, UiColor(0x7C3AED), [this](View, InputEvent) -> bool {
      mNavigator.PushModal(CreateModal(), true);
      UpdateStatus();
      return true;
    }));

    toolbar.Add(CreateButton("Back", 376_spx, 24_spx, UiColor(0xB45309), [this](View, InputEvent) -> bool {
      mNavigator.NavigateBack();
      UpdateStatus();
      return true;
    }));

    mStatusLabel = Label::New("")
                     .SetRequestedWidth(280_spx)
                     .SetRequestedHeight(48_spx)
                     .SetRequestedPositionX(510_spx)
                     .SetRequestedPositionY(24_spx)
                     .SetFontSize(18_spx)
                     .SetTextColor(UiColor(0xE5E7EB));
    toolbar.Add(mStatusLabel);

    return toolbar;
  }

  InteractiveView CreateButton(const char* text, float x, float y, UiColor color, std::function<bool(View, InputEvent)> callback)
  {
    InteractiveView button = InteractiveView::New()
                               .SetBackgroundColor(color)
                               .SetRequestedWidth(104_spx)
                               .SetRequestedHeight(48_spx)
                               .SetRequestedPositionX(x)
                               .SetRequestedPositionY(y)
                               .ConnectClickedSignal(this, std::move(callback));

    button.Add(Label::New(text)
                 .SetRequestedWidth(MATCH_PARENT)
                 .SetRequestedHeight(MATCH_PARENT)
                 .SetFontSize(17_spx)
                 .SetTextColor(UiColor(0xFFFFFF)));
    return button;
  }

  View CreatePage(const std::string& title, uint32_t index)
  {
    Layout page = Layout::New()
                    .SetRequestedWidth(MATCH_PARENT)
                    .SetRequestedHeight(MATCH_PARENT)
                    .SetBackgroundColor(PageBackground(index));

    page.Add(View::New()
               .SetRequestedWidth(18_spx)
               .SetRequestedHeight(MATCH_PARENT)
               .SetBackgroundColor(PageAccent(index)));

    page.Add(Label::New(Dali::String(title.c_str()))
               .SetRequestedWidth(520_spx)
               .SetRequestedHeight(82_spx)
               .SetRequestedPositionX(48_spx)
               .SetRequestedPositionY(146_spx)
               .SetFontSize(42_spx)
               .SetTextColor(PageAccent(index)));

    Layout panel = Layout::New()
                     .SetRequestedWidth(560_spx)
                     .SetRequestedHeight(190_spx)
                     .SetRequestedPositionX(48_spx)
                     .SetRequestedPositionY(250_spx)
                     .SetBackgroundColor(UiColor(0xFFFFFF));

    panel.Add(Label::New("Navigator content area")
               .SetRequestedWidth(500_spx)
               .SetRequestedHeight(42_spx)
               .SetRequestedPositionX(28_spx)
               .SetRequestedPositionY(28_spx)
               .SetFontSize(24_spx)
               .SetTextColor(UiColor(0x111827)));

    panel.Add(Label::New("Push adds a page, Modal opens above it, Back prefers the modal stack.")
               .SetRequestedWidth(500_spx)
               .SetRequestedHeight(70_spx)
               .SetRequestedPositionX(28_spx)
               .SetRequestedPositionY(84_spx)
               .SetFontSize(17_spx)
               .SetTextColor(UiColor(0x4B5563)));

    page.Add(panel);

    page.Add(Label::New("Use the toolbar to exercise Navigator stacks")
               .SetRequestedWidth(560_spx)
               .SetRequestedHeight(44_spx)
               .SetRequestedPositionX(48_spx)
               .SetRequestedPositionY(470_spx)
               .SetFontSize(18_spx)
               .SetTextColor(UiColor(0x374151)));

    return page;
  }

  View CreateModal()
  {
    Layout modal = Layout::New()
                    .SetRequestedWidth(MATCH_PARENT)
                    .SetRequestedHeight(MATCH_PARENT)
                    .SetBackgroundColor(UiColor(0x99000000));

    Layout dialog = Layout::New()
                      .SetLayoutMode(LayoutMode::STANDALONE)
                      .SetRequestedWidth(360_spx)
                      .SetRequestedHeight(220_spx)
                      .SetRequestedPositionX(220_spx)
                      .SetRequestedPositionY(190_spx)
                      .SetBackgroundColor(UiColor(0xFFFFFF));

    dialog.Add(Label::New("Modal View")
                 .SetRequestedWidth(300_spx)
                 .SetRequestedHeight(60_spx)
                 .SetRequestedPositionX(28_spx)
                 .SetRequestedPositionY(32_spx)
                 .SetFontSize(30_spx)
                 .SetTextColor(UiColor(0x111827)));

    dialog.Add(Label::New("This view lives on the modal stack.")
                 .SetRequestedWidth(300_spx)
                 .SetRequestedHeight(44_spx)
                 .SetRequestedPositionX(28_spx)
                 .SetRequestedPositionY(92_spx)
                 .SetFontSize(17_spx)
                 .SetTextColor(UiColor(0x4B5563)));

    dialog.Add(CreateButton("Close", 28_spx, 150_spx, UiColor(0x111827), [this](View, InputEvent) -> bool {
      mNavigator.PopModal(true);
      UpdateStatus();
      return true;
    }));

    modal.Add(dialog);
    return modal;
  }

  std::string MakePageTitle() const
  {
    std::ostringstream stream;
    stream << "Page " << mPageIndex;
    return stream.str();
  }

  void UpdateStatus()
  {
    std::ostringstream stream;
    stream << "nav " << mNavigator.GetNavigationStackCount()
           << " / modal " << mNavigator.GetModalStackCount();
    const std::string status = stream.str();
    mStatusLabel.SetText(Dali::String(status.c_str()));
  }

private:
  Application& mApplication;
  Navigator    mNavigator;
  Label        mStatusLabel;
  uint32_t     mPageIndex{0u};
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);
  UiConfig::New().Apply();
  NavigatorExample example(application);
  application.MainLoop();
  return 0;
}
