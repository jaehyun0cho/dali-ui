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
#include <dali/public-api/common/dali-string-view.h>
#include <dali/public-api/common/dali-string.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>

namespace Dali
{
namespace Ui
{
namespace Extension
{
/**
 * @brief Reads localized strings from the platform screen reader message catalog.
 *
 * The screen reader message catalog is owned and installed by the platform
 * screen reader package. Dali UI never installs, writes, or extends that
 * catalog; it only reads from it. Because the catalog is external, a lookup
 * returns a translation only on a target where that package is installed and
 * provides a message for the current locale.
 *
 * The catalog domain is bound once per process, on the first lookup. The
 * binding uses an explicit domain for every lookup, so the application's
 * default domain set through UiLocalizationManager::SetDefaultDomain() is
 * never read and never changed by these functions.
 *
 * @code
 *   Dali::String label;
 *   if(Extension::ScreenReaderLocalization::TryGetLocalizedString("IDS_ACCS_TBOPT_BUTTON", label))
 *   {
 *     // label holds the translated text
 *   }
 *
 *   // Or take the fallback silently
 *   Dali::String text = Extension::ScreenReaderLocalization::GetLocalizedString("IDS_ACCS_TBOPT_BUTTON");
 * @endcode
 */
namespace ScreenReaderLocalization
{
/**
 * @brief Looks up a screen reader string and reports whether it was translated.
 *
 * @a outString is always assigned, whatever the result:
 *   - a translation was found -> @a outString is the translation, returns true
 *   - no translation was found -> @a outString is @a resourceId, returns false
 *   - @a resourceId is empty -> @a outString is empty, returns false
 *
 * @note A translation whose text is identical to its msgid cannot be told
 *       apart from a missing translation, and is reported as untranslated.
 * @note Plural forms are not supported. Only singular msgid lookup is performed.
 *
 * @pre This function must be called from the UI thread.
 * @param[in] resourceId The localization resource id / msgid
 * @param[out] outString The translated string, or the fallback described above
 * @return true if a translation was found, false otherwise
 */
DALI_UI_API bool TryGetLocalizedString(StringView resourceId, Dali::String& outString);

/**
 * @brief Looks up a screen reader string, falling back to the resource id.
 *
 * Equivalent to TryGetLocalizedString() with the returned flag discarded.
 *
 * @note A translation whose text is identical to its msgid cannot be told
 *       apart from a missing translation, but both cases return the same text.
 * @note Plural forms are not supported. Only singular msgid lookup is performed.
 *
 * @pre This function must be called from the UI thread.
 * @param[in] resourceId The localization resource id / msgid
 * @return The translated string, or @a resourceId when no translation was found
 */
DALI_UI_API Dali::String GetLocalizedString(StringView resourceId);

} // namespace ScreenReaderLocalization
} // namespace Extension
} // namespace Ui
} // namespace Dali
