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

#include <cstdint>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>

namespace Dali
{
namespace Ui
{
namespace Components
{
/**
 * @brief Reads strings from the message catalog shipped with dali-ui-components.
 *
 * The catalog collects the common UI and accessibility strings that components
 * need, so a component can present translated text without the application
 * providing its own translations.
 *
 * The catalog domain is bound once per process, on the first lookup, and every
 * lookup passes that domain explicitly. The application's default domain is
 * neither read nor changed.
 *
 * @code
 *   #include <dali-ui-components/public-api/localization/components-localization.h>
 *
 *   namespace Localization = Dali::Ui::Components::Localization;
 *
 *   // Plain lookup
 *   Dali::String text = Localization::GetLocalizedString("IDS_ACCS_TBOPT_BUTTON");
 *
 *   // Lookup with integer arguments, e.g. "Page 1 of 5."
 *   Dali::String page = Localization::GetLocalizedString(
 *     "IDS_ACCS_POP_PAGE_P1SD_OF_P2SD_M_DESCRIPTIVE_FORM_TTS", 1, 5);
 *
 *   // Lookup with a string and an integer argument, e.g. "Media volume, 42 percent"
 *   Dali::String volume = Localization::GetLocalizedString(
 *     "IDS_GCTS_OPT_P1SS_VOLUME_P2SD_PERCENT_TTS", "Media", 42);
 * @endcode
 */
namespace Localization
{
/**
 * @brief Gets the gettext domain of the components message catalog.
 *
 * The returned name is a constant and is always available.
 *
 * Calling this also attempts the one-time binding of the catalog. That binding
 * needs Dali::Ui::UiLocalizationManager, so it is skipped while the manager
 * does not exist yet and retried on the next call. A returned domain is
 * therefore not proof that the catalog is bound.
 *
 * @return The components message catalog domain
 */
DALI_UI_COMPONENTS_API Dali::String GetDomain();

/**
 * @brief Gets a localized string from the components message catalog.
 *
 * Fallback behavior:
 *   - empty resourceId -> empty string
 *   - no translation found -> resourceId
 *
 * @param[in] resourceId The localization resource id / msgid
 * @return The localized string, or the fallback described above
 */
DALI_UI_COMPONENTS_API Dali::String GetLocalizedString(StringView resourceId);

/**
 * @brief Gets a localized string that takes one integer argument.
 *
 * The translated text is used as a format string and may place the argument
 * with either a sequential ("%d") or a positional ("%1$d") specifier.
 *
 * Fallback behavior:
 *   - empty resourceId -> empty string
 *   - no translation found -> resourceId
 *   - translation is not a format matching the arguments -> translation as-is,
 *     unformatted
 *
 * @param[in] resourceId The localization resource id / msgid
 * @param[in] value1 The value for the first format argument
 * @return The formatted localized string, or the fallback described above
 */
DALI_UI_COMPONENTS_API Dali::String GetLocalizedString(StringView resourceId, int32_t value1);

/**
 * @brief Gets a localized string that takes two integer arguments.
 *
 * The translated text is used as a format string and may place the arguments
 * with either sequential ("%d") or positional ("%1$d") specifiers. Positional
 * specifiers let a translation reorder the arguments, e.g. "Page %1$d of %2$d."
 * against a translation that reads the second value first.
 *
 * A translation may drop trailing arguments, e.g. use only "%1$d", but it may
 * not skip one: a hole such as "%2$d" without "%1$d" is treated as an invalid
 * format, because how an unreferenced positional argument behaves differs
 * between libc implementations.
 *
 * Fallback behavior:
 *   - empty resourceId -> empty string
 *   - no translation found -> resourceId
 *   - translation is not a format matching the arguments -> translation as-is,
 *     unformatted
 *
 * @param[in] resourceId The localization resource id / msgid
 * @param[in] value1 The value for the first format argument
 * @param[in] value2 The value for the second format argument
 * @return The formatted localized string, or the fallback described above
 */
DALI_UI_COMPONENTS_API Dali::String GetLocalizedString(StringView resourceId, int32_t value1, int32_t value2);

/**
 * @brief Gets a localized string that takes one string argument.
 *
 * The translated text is used as a format string and may place the argument
 * with either a sequential ("%s") or a positional ("%1$s") specifier.
 *
 * The value does not need to be null-terminated; it is copied internally.
 *
 * Fallback behavior:
 *   - empty resourceId -> empty string
 *   - no translation found -> resourceId
 *   - translation is not a format matching the arguments -> translation as-is,
 *     unformatted
 *
 * @param[in] resourceId The localization resource id / msgid
 * @param[in] value1 The value for the first format argument
 * @return The formatted localized string, or the fallback described above
 */
DALI_UI_COMPONENTS_API Dali::String GetLocalizedString(StringView resourceId, StringView value1);

/**
 * @brief Gets a localized string that takes two string arguments.
 *
 * Positional specifiers ("%1$s", "%2$s") let a translation reorder the
 * arguments, and the same trailing-drop and no-hole rules apply as for the
 * integer overloads. The values do not need to be null-terminated.
 *
 * Fallback behavior: as for the one-string overload.
 *
 * @param[in] resourceId The localization resource id / msgid
 * @param[in] value1 The value for the first format argument
 * @param[in] value2 The value for the second format argument
 * @return The formatted localized string, or the fallback described above
 */
DALI_UI_COMPONENTS_API Dali::String GetLocalizedString(StringView resourceId, StringView value1, StringView value2);

/**
 * @brief Gets a localized string that takes a string and an integer argument.
 *
 * The first format argument is the string ("%1$s") and the second the integer
 * ("%2$d"), e.g. "%1$s volume, %2$d percent". The string value does not need
 * to be null-terminated.
 *
 * Fallback behavior: as for the one-string overload.
 *
 * @param[in] resourceId The localization resource id / msgid
 * @param[in] value1 The value for the first format argument
 * @param[in] value2 The value for the second format argument
 * @return The formatted localized string, or the fallback described above
 */
DALI_UI_COMPONENTS_API Dali::String GetLocalizedString(StringView resourceId, StringView value1, int32_t value2);

} // namespace Localization
} // namespace Components
} // namespace Ui
} // namespace Dali
