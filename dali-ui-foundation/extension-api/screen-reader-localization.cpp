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

// CLASS HEADER
#include <dali-ui-foundation/extension-api/screen-reader-localization.h>

// EXTERNAL INCLUDES
#include <dali/devel-api/adaptor-framework/environment-variable.h>
#include <dali/integration-api/string-utils.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/configuration/ui-localization-manager.h>

namespace
{
#define TOKEN_STRING(x) #x

/// The gettext domain of the platform screen reader message catalog.
constexpr const char* SCREEN_READER_DOMAIN = "screen-reader";

std::string GetScreenReaderLocaleDirectory()
{
  /**
   * @note DALI_UI_SCREEN_READER_LOCALE_DIR is a macro that can be defined either with a file
   *       system path or zero. If it's defined as zero then the value is retrieved from an
   *       environment variable named DALI_UI_SCREEN_READER_LOCALE_DIR.
   */
  const char* localeDirectory = (nullptr == DALI_UI_SCREEN_READER_LOCALE_DIR)
                                  ? Dali::EnvironmentVariable::GetEnvironmentVariable(TOKEN_STRING(DALI_UI_SCREEN_READER_LOCALE_DIR))
                                  : DALI_UI_SCREEN_READER_LOCALE_DIR;

  return (localeDirectory != nullptr) ? std::string(localeDirectory) : std::string();
}

/**
 * @brief Binds the screen reader catalog domain once per process.
 *
 * The latch is only set on success. A failed registration is retried on the
 * next call, because RegisterDomain() refreshes every existing localization
 * binding when it succeeds and must therefore run exactly once.
 *
 * @param[in] manager The localization manager singleton
 * @return true if the domain is bound
 */
bool EnsureDomainRegistered(Dali::Ui::UiLocalizationManager& manager)
{
  static bool gRegistered = false;

  if(gRegistered)
  {
    return true;
  }

  const std::string localeDirectory = GetScreenReaderLocaleDirectory();
  if(localeDirectory.empty())
  {
    return false;
  }

  if(!manager.RegisterDomain(SCREEN_READER_DOMAIN, Dali::Integration::ToDaliStringView(localeDirectory)))
  {
    return false;
  }

  gRegistered = true;
  return true;
}

} // unnamed namespace

namespace Dali
{
namespace Ui
{
namespace Extension
{
namespace ScreenReaderLocalization
{
bool TryGetLocalizedString(StringView resourceId, Dali::String& outString)
{
  if(resourceId.Empty() || resourceId.Data() == nullptr)
  {
    outString.Clear();
    return false;
  }

  UiLocalizationManager manager = UiLocalizationManager::Get();
  if(!manager)
  {
    outString = resourceId;
    return false;
  }

  // Bind the catalog if it is not bound yet. A failure here is not fatal: the
  // lookup below still consults the override hook, which is applied before the
  // domain is resolved, and an unbound domain falls back to the resource id.
  EnsureDomainRegistered(manager);

  Dali::String localized = manager.GetLocalizedString(resourceId, SCREEN_READER_DOMAIN);

  // dgettext() echoes the msgid back when the catalog has no message for it.
  if(localized == resourceId)
  {
    outString = resourceId;
    return false;
  }

  outString = std::move(localized);
  return true;
}

Dali::String GetLocalizedString(StringView resourceId)
{
  Dali::String localized;
  TryGetLocalizedString(resourceId, localized);
  return localized;
}

} // namespace ScreenReaderLocalization
} // namespace Extension
} // namespace Ui
} // namespace Dali
