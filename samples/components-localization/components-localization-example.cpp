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

// Reads strings from the message catalog shipped with dali-ui-components.
//
// The sample owns no translations of its own: the catalog is installed with the
// components library, and Components::Localization binds it on the first lookup.
//
// "IDS_ACCS_POP_PAGE_P1SD_OF_P2SD_M_DESCRIPTIVE_FORM_TTS" takes two integer
// arguments and its translations place them in different orders, so the same
// call reads "Page 1 of 5." in English and "5페이지 중 1페이지." in Korean.
//
// Key bindings:
//   1 - Set locale to en_US
//   2 - Set locale to ko_KR
//   ESC/BACK - Quit

#include <dali-ui-components/public-api/components-ui-config.h>
#include <dali-ui-components/public-api/localization/components-localization.h>
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout.h>
#include <dali-ui-foundation/public-api/views/text-controls/label.h>

#include <clocale>
#include <cstdio>
#include <string>

using namespace Dali;
using namespace Dali::Ui;

namespace ComponentsLocalization = Dali::Ui::Components::Localization;

namespace
{
#if defined(_WIN32)
constexpr int MESSAGE_LOCALE_CATEGORY = LC_ALL;
#else
constexpr int MESSAGE_LOCALE_CATEGORY = LC_MESSAGES;
#endif

// Takes the current page and the page count, e.g. "Page 1 of 5."
constexpr const char* IDS_PAGE_OF = "IDS_ACCS_POP_PAGE_P1SD_OF_P2SD_M_DESCRIPTIVE_FORM_TTS";

constexpr int32_t CURRENT_PAGE = 1;
constexpr int32_t PAGE_COUNT   = 5;

constexpr float STACK_SPACING = 10.0f;
constexpr float STACK_PADDING = 20.0f;

constexpr float TITLE_FONT_SIZE  = 20.0f;
constexpr float RESULT_FONT_SIZE = 26.0f;
constexpr float STATUS_FONT_SIZE = 16.0f;
constexpr float HELP_FONT_SIZE   = 14.0f;

constexpr uint32_t COLOR_WHITE      = 0xFFFFFF;
constexpr uint32_t COLOR_DARK_TEXT  = 0x222222;
constexpr uint32_t COLOR_MID_GRAY   = 0x808080;
constexpr uint32_t COLOR_LIGHT_GRAY = 0xF2F2F2;
constexpr uint32_t COLOR_LIGHT_BLUE = 0xEAF4FF;

} // namespace

class ComponentsLocalizationExample : public ConnectionTracker
{
public:
  explicit ComponentsLocalizationExample(Application& application)
  : mApplication(application),
    mCurrentLocale("en_US.UTF-8")
  {
    mApplication.InitSignal().Connect(this, &ComponentsLocalizationExample::OnInit);
  }

private:
  void OnInit(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(UiColor(COLOR_WHITE));

    window.Add(CreateContents());

    SetLocale(mCurrentLocale);

    window.KeyEventSignal().Connect(this, &ComponentsLocalizationExample::OnKeyEvent);
  }

  View CreateContents()
  {
    StackLayout contents = StackLayout::New(StackOrientation::VERTICAL);
    contents.SetSpacing(STACK_SPACING);
    contents.SetRequestedWidth(MATCH_PARENT);
    contents.SetRequestedHeight(MATCH_PARENT);
    contents.SetPadding(Extents(static_cast<int16_t>(STACK_PADDING),
                                static_cast<int16_t>(STACK_PADDING),
                                static_cast<int16_t>(STACK_PADDING),
                                static_cast<int16_t>(STACK_PADDING)));

    mResultLabel = CreateResultLabel();
    mRawLabel    = CreateRawLabel();
    mStatusLabel = CreateStatusLabel();

    contents.Add(CreateHeaderLabel());
    contents.Add(CreateHelpLabel());
    contents.Add(CreateSeparator());
    contents.Add(CreateSectionLabel("GetLocalizedString(resourceId, 1, 5):"));
    contents.Add(mResultLabel);
    contents.Add(CreateSectionLabel("Raw catalog text:"));
    contents.Add(mRawLabel);
    contents.Add(CreateSeparator());
    contents.Add(mStatusLabel);
    return contents;
  }

  Label CreateHeaderLabel()
  {
    Label label = Label::New("Components Message Catalog");
    label.SetFontSize(TITLE_FONT_SIZE);
    label.SetTextColor(UiColor(COLOR_DARK_TEXT));
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    label.SetLayoutDirectionMode(Text::LayoutDirectionMode::CONTENTS);
    label.SetPadding(Extents(10, 10, 10, 10));
    label.SetBackgroundColor(UiColor(COLOR_LIGHT_GRAY));
    return label;
  }

  Label CreateHelpLabel()
  {
    Label label = Label::New("Keys: 1=en_US, 2=ko_KR, ESC/BACK=quit");
    label.SetFontSize(HELP_FONT_SIZE);
    label.SetTextColor(UiColor(COLOR_DARK_TEXT));
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    label.SetLayoutDirectionMode(Text::LayoutDirectionMode::CONTENTS);
    label.SetMultiLine(true);
    label.SetPadding(Extents(10, 10, 10, 10));
    label.SetBackgroundColor(UiColor(0xEFEFEF));
    return label;
  }

  View CreateSeparator()
  {
    View separator = View::New();
    separator.SetBackgroundColor(UiColor(COLOR_MID_GRAY));
    separator.SetRequestedWidth(MATCH_PARENT);
    separator.SetRequestedHeight(2.0f);
    return separator;
  }

  Label CreateSectionLabel(const char* text)
  {
    Label label = Label::New(text);
    label.SetFontSize(STATUS_FONT_SIZE);
    label.SetTextColor(UiColor(COLOR_MID_GRAY));
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    label.SetLayoutDirectionMode(Text::LayoutDirectionMode::CONTENTS);
    return label;
  }

  Label CreateResultLabel()
  {
    Label label = Label::New();
    label.SetFontSize(RESULT_FONT_SIZE);
    label.SetTextColor(UiColor(COLOR_DARK_TEXT));
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    label.SetLayoutDirectionMode(Text::LayoutDirectionMode::CONTENTS);
    label.SetMultiLine(true);
    label.SetPadding(Extents(10, 10, 10, 10));
    label.SetBackgroundColor(UiColor(COLOR_LIGHT_BLUE));
    return label;
  }

  Label CreateRawLabel()
  {
    Label label = Label::New();
    label.SetFontSize(STATUS_FONT_SIZE);
    label.SetTextColor(UiColor(COLOR_DARK_TEXT));
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    label.SetLayoutDirectionMode(Text::LayoutDirectionMode::CONTENTS);
    label.SetMultiLine(true);
    label.SetPadding(Extents(10, 10, 10, 10));
    label.SetBackgroundColor(UiColor(0xFAFAFA));
    return label;
  }

  Label CreateStatusLabel()
  {
    Label label = Label::New();
    label.SetFontSize(STATUS_FONT_SIZE);
    label.SetTextColor(UiColor(COLOR_DARK_TEXT));
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    label.SetLayoutDirectionMode(Text::LayoutDirectionMode::CONTENTS);
    label.SetMultiLine(true);
    label.SetPadding(Extents(10, 10, 10, 10));
    label.SetBackgroundColor(UiColor(0xFAFAFA));
    return label;
  }

  void UpdateLabels()
  {
    // The formatted lookup. In en_US this reads "Page 1 of 5."; in ko_KR the
    // translation reverses the arguments and reads "5페이지 중 1페이지."
    Dali::String formatted = ComponentsLocalization::GetLocalizedString(IDS_PAGE_OF, CURRENT_PAGE, PAGE_COUNT);
    mResultLabel.SetText(formatted);

    // The same entry without arguments, i.e. the catalog text as stored.
    Dali::String raw = ComponentsLocalization::GetLocalizedString(IDS_PAGE_OF);
    mRawLabel.SetText(raw);

    std::printf("[%s] %s -> %s\n", mCurrentLocale.c_str(), raw.CStr(), formatted.CStr());

    const char* lcMessages = setlocale(MESSAGE_LOCALE_CATEGORY, nullptr);
    if(!lcMessages)
    {
      lcMessages = "(null)";
    }

    std::string status;
    status += "Locale: " + mCurrentLocale + "\n";
    status += "Domain: " + std::string(ComponentsLocalization::GetDomain().CStr()) + "\n";
    status += "LC_MESSAGES: " + std::string(lcMessages);
    mStatusLabel.SetText(Dali::String(status.c_str()));
  }

  /**
   * @brief Sets the locale for Ubuntu desktop sample testing.
   *
   * This is a desktop-only helper. On target devices, verify localization by
   * changing the system language in Settings and letting the platform locale
   * changed signal refresh the bindings.
   */
  void SetLocale(const std::string& locale)
  {
    mCurrentLocale = locale;

    if(setlocale(MESSAGE_LOCALE_CATEGORY, locale.c_str()) == nullptr)
    {
      std::printf("setlocale(\"%s\") failed; the catalog cannot be resolved for this locale\n", locale.c_str());
    }

    UpdateLabels();
  }

  void OnKeyEvent(Window window, KeyEvent event)
  {
    if(event.GetState() != KeyEvent::UP)
    {
      return;
    }

    if(IsKey(event, Dali::DALI_KEY_ESCAPE) || IsKey(event, Dali::DALI_KEY_BACK))
    {
      mApplication.Quit();
      return;
    }

    if(event.GetKeyName() == "1")
    {
      SetLocale("en_US.UTF-8");
    }
    else if(event.GetKeyName() == "2")
    {
      SetLocale("ko_KR.UTF-8");
    }
  }

private:
  Application& mApplication;
  Label        mResultLabel;
  Label        mRawLabel;
  Label        mStatusLabel;
  std::string  mCurrentLocale;
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);
  Components::UiConfig::New().Apply();

  ComponentsLocalizationExample example(application);
  application.MainLoop();

  return 0;
}
