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

#include <dali-ui-components/dali-ui-components.h>
#include <dali-ui-foundation/public-api/configuration/ui-localization-manager.h>
#include <dali-ui-test-suite-utils.h>

#include <cstring>

using namespace Dali;
using namespace Dali::Ui;

namespace ComponentsLocalization = Dali::Ui::Components::Localization;

namespace
{
// The catalog installed on the build machine is not available to the test
// binary, so every case routes lookups through the override hook instead.
// That keeps the expected strings inside this file and independent of the
// locale of the machine running the tests.

const char* const COMPONENTS_DOMAIN = "dali-ui-components";

const char* const RESOURCE_TWO_POSITIONAL    = "IDS_TEST_TWO_POSITIONAL";
const char* const RESOURCE_TWO_REORDERED     = "IDS_TEST_TWO_REORDERED";
const char* const RESOURCE_ONE_SEQUENTIAL    = "IDS_TEST_ONE_SEQUENTIAL";
const char* const RESOURCE_STRING_CONVERSION = "IDS_TEST_STRING_CONVERSION";
const char* const RESOURCE_PLAIN             = "IDS_TEST_PLAIN";
const char* const RESOURCE_UNKNOWN           = "IDS_TEST_UNKNOWN";

// Formats that reference the second argument without the first, or an index
// beyond the supplied arguments.
const char* const RESOURCE_GAP_ONLY      = "IDS_TEST_GAP_ONLY";
const char* const RESOURCE_GAP_REPEATED  = "IDS_TEST_GAP_REPEATED";
const char* const RESOURCE_GAP_PREFIXED  = "IDS_TEST_GAP_PREFIXED";
const char* const RESOURCE_GAP_HIGH      = "IDS_TEST_GAP_HIGH";
const char* const RESOURCE_TRAILING_DROP = "IDS_TEST_TRAILING_DROP";

// Formats taking string arguments, in every combination the catalog uses.
const char* const RESOURCE_ONE_STRING       = "IDS_TEST_ONE_STRING";
const char* const RESOURCE_SEQ_STRING       = "IDS_TEST_SEQ_STRING";
const char* const RESOURCE_TWO_STRINGS      = "IDS_TEST_TWO_STRINGS";
const char* const RESOURCE_TWO_STRINGS_SWAP = "IDS_TEST_TWO_STRINGS_SWAP";
const char* const RESOURCE_STRING_THEN_INT  = "IDS_TEST_STRING_THEN_INT";
const char* const RESOURCE_GAP_STRING       = "IDS_TEST_GAP_STRING";

bool EqualsStringView(StringView view, const char* text)
{
  const size_t length = strlen(text);
  return view.Size() == length &&
         view.Data() != nullptr &&
         strncmp(view.Data(), text, length) == 0;
}

// Serves the catalog strings the cases below expect. Anything else falls
// through to the normal lookup.
bool OverrideComponentsCatalog(StringView resourceId, StringView domain, Dali::String& outString)
{
  if(!EqualsStringView(domain, COMPONENTS_DOMAIN))
  {
    return false;
  }

  if(EqualsStringView(resourceId, RESOURCE_TWO_POSITIONAL))
  {
    outString = Dali::String("Page %1$d of %2$d.");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_TWO_REORDERED))
  {
    outString = Dali::String("%2$d / %1$d");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_ONE_SEQUENTIAL))
  {
    outString = Dali::String("%d items");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_STRING_CONVERSION))
  {
    outString = Dali::String("%1$s");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_PLAIN))
  {
    outString = Dali::String("Button");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_GAP_ONLY))
  {
    outString = Dali::String("%2$d");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_GAP_REPEATED))
  {
    outString = Dali::String("%2$d %2$d");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_GAP_PREFIXED))
  {
    outString = Dali::String("b%2$d");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_GAP_HIGH))
  {
    outString = Dali::String("%3$d %1$d");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_TRAILING_DROP))
  {
    outString = Dali::String("%1$d");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_ONE_STRING))
  {
    outString = Dali::String("%1$s percent");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_SEQ_STRING))
  {
    outString = Dali::String("%s deleted");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_TWO_STRINGS))
  {
    outString = Dali::String("%1$s of %2$s");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_TWO_STRINGS_SWAP))
  {
    outString = Dali::String("%2$s / %1$s");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_STRING_THEN_INT))
  {
    outString = Dali::String("%1$s volume, %2$d percent");
    return true;
  }
  if(EqualsStringView(resourceId, RESOURCE_GAP_STRING))
  {
    outString = Dali::String("%2$s");
    return true;
  }

  return false;
}

// Reports the domain each lookup was routed with.
Dali::String gObservedDomain;

bool OverrideRecordingDomain(StringView resourceId, StringView domain, Dali::String& outString)
{
  gObservedDomain = Dali::String(domain);
  outString       = Dali::String("Routed");
  return true;
}

// The components catalog registers itself lazily and the registration latch is
// process-global, so a case must never assume it is the first registrant.
// Restore the manager to its default state instead.
void CleanupManager()
{
  UiLocalizationManager manager = UiLocalizationManager::Get();
  manager.ClearLocalizedStringOverride();
  manager.SetBypassEnabled(false);
}

} // namespace

void utc_dali_components_localization_startup(void)
{
  test_return_value = TET_UNDEF;
  gObservedDomain   = Dali::String();
}

void utc_dali_components_localization_cleanup(void)
{
  test_return_value = TET_PASS;
}

// === GetDomain ===

int UtcDaliComponentsLocalizationGetDomainP(void)
{
  UiTestApplication application;

  DALI_TEST_EQUALS(ComponentsLocalization::GetDomain(), Dali::String(COMPONENTS_DOMAIN), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

// === Explicit domain routing ===

int UtcDaliComponentsLocalizationExplicitDomainP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();

  // A different default domain must not be picked up by the components lookup.
  manager.SetDefaultDomain("some-application-domain");
  manager.SetLocalizedStringOverride(OverrideRecordingDomain);

  Dali::String result = ComponentsLocalization::GetLocalizedString(RESOURCE_PLAIN);

  DALI_TEST_EQUALS(result, Dali::String("Routed"), TEST_LOCATION);
  DALI_TEST_EQUALS(gObservedDomain, Dali::String(COMPONENTS_DOMAIN), TEST_LOCATION);

  // The application default domain must be left untouched.
  DALI_TEST_EQUALS(manager.GetDefaultDomain(), Dali::String("some-application-domain"), TEST_LOCATION);

  manager.SetDefaultDomain("");
  CleanupManager();

  END_TEST;
}

// === Plain lookup ===

int UtcDaliComponentsLocalizationGetLocalizedStringP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_PLAIN), Dali::String("Button"), TEST_LOCATION);

  // Not in the catalog -> the resource id comes back.
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_UNKNOWN), Dali::String(RESOURCE_UNKNOWN), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

// === Positional arguments ===

int UtcDaliComponentsLocalizationTwoPositionalArgumentsP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  Dali::String result = ComponentsLocalization::GetLocalizedString(RESOURCE_TWO_POSITIONAL, 1, 5);
  DALI_TEST_EQUALS(result, Dali::String("Page 1 of 5."), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

int UtcDaliComponentsLocalizationReorderedArgumentsP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  // A translation may read the arguments in a different order.
  Dali::String result = ComponentsLocalization::GetLocalizedString(RESOURCE_TWO_REORDERED, 1, 5);
  DALI_TEST_EQUALS(result, Dali::String("5 / 1"), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

// === Sequential argument ===

int UtcDaliComponentsLocalizationOneSequentialArgumentP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  Dali::String result = ComponentsLocalization::GetLocalizedString(RESOURCE_ONE_SEQUENTIAL, 1);
  DALI_TEST_EQUALS(result, Dali::String("1 items"), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

// === String arguments ===

int UtcDaliComponentsLocalizationOneStringArgumentP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  Dali::String result = ComponentsLocalization::GetLocalizedString(RESOURCE_ONE_STRING, "vol");
  DALI_TEST_EQUALS(result, Dali::String("vol percent"), TEST_LOCATION);

  // A sequential "%s" works the same way.
  result = ComponentsLocalization::GetLocalizedString(RESOURCE_SEQ_STRING, "Wi-Fi");
  DALI_TEST_EQUALS(result, Dali::String("Wi-Fi deleted"), TEST_LOCATION);

  // A StringView value need not be null-terminated: only the viewed range is used.
  result = ComponentsLocalization::GetLocalizedString(RESOURCE_ONE_STRING, StringView("volume", 3u));
  DALI_TEST_EQUALS(result, Dali::String("vol percent"), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

int UtcDaliComponentsLocalizationTwoStringArgumentsP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  Dali::String result = ComponentsLocalization::GetLocalizedString(RESOURCE_TWO_STRINGS, "track", "album");
  DALI_TEST_EQUALS(result, Dali::String("track of album"), TEST_LOCATION);

  // A translation may read the arguments in a different order.
  result = ComponentsLocalization::GetLocalizedString(RESOURCE_TWO_STRINGS_SWAP, "first", "second");
  DALI_TEST_EQUALS(result, Dali::String("second / first"), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

int UtcDaliComponentsLocalizationStringAndIntegerArgumentP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  Dali::String result = ComponentsLocalization::GetLocalizedString(RESOURCE_STRING_THEN_INT, "Media", 42);
  DALI_TEST_EQUALS(result, Dali::String("Media volume, 42 percent"), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

int UtcDaliComponentsLocalizationTypeMismatchN(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  // Every conversion must match the type of the argument it consumes. On any
  // mismatch the raw text comes back unformatted, in either direction.
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_TWO_POSITIONAL, "a", "b"), Dali::String("Page %1$d of %2$d."), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_ONE_STRING, 5), Dali::String("%1$s percent"), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_STRING_THEN_INT, 1, 5), Dali::String("%1$s volume, %2$d percent"), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_STRING_THEN_INT, "a", "b"), Dali::String("%1$s volume, %2$d percent"), TEST_LOCATION);

  // The positional-gap rule applies to string conversions as well.
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_GAP_STRING, "a", "b"), Dali::String("%2$s"), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

// === Rejected format strings ===

int UtcDaliComponentsLocalizationRejectedFormatN(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  // "%1$s" is a string conversion. It must never be handed to snprintf with an
  // integer argument, so the raw text comes back unformatted.
  Dali::String result = ComponentsLocalization::GetLocalizedString(RESOURCE_STRING_CONVERSION, 1);
  DALI_TEST_EQUALS(result, Dali::String("%1$s"), TEST_LOCATION);

  result = ComponentsLocalization::GetLocalizedString(RESOURCE_STRING_CONVERSION, 1, 5);
  DALI_TEST_EQUALS(result, Dali::String("%1$s"), TEST_LOCATION);

  // A text with no conversion at all is returned unchanged.
  result = ComponentsLocalization::GetLocalizedString(RESOURCE_PLAIN, 1, 5);
  DALI_TEST_EQUALS(result, Dali::String("Button"), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

int UtcDaliComponentsLocalizationPositionalGapN(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  // A translation must not skip a positional index. How an unreferenced
  // positional argument behaves differs between libc implementations, so these
  // come back unformatted.
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_GAP_ONLY, 1, 5), Dali::String("%2$d"), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_GAP_REPEATED, 1, 5), Dali::String("%2$d %2$d"), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_GAP_PREFIXED, 1, 5), Dali::String("b%2$d"), TEST_LOCATION);

  // An index beyond the supplied arguments is rejected as well.
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_GAP_HIGH, 1, 5), Dali::String("%3$d %1$d"), TEST_LOCATION);

  // Dropping a trailing argument stays valid: the extra value is simply unused.
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_TRAILING_DROP, 1, 5), Dali::String("1"), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_TRAILING_DROP, 7), Dali::String("7"), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

// === Empty resource id ===

int UtcDaliComponentsLocalizationEmptyResourceIdN(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(""), Dali::String(), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString("", 1), Dali::String(), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString("", 1, 5), Dali::String(), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString("", "a"), Dali::String(), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString("", "a", "b"), Dali::String(), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString("", "a", 5), Dali::String(), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}

// === Bypass ===

int UtcDaliComponentsLocalizationBypassP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager();
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);

  manager.SetBypassEnabled(true);

  // Bypass wins over the override, so the resource id comes back as-is and the
  // formatting overloads leave it alone.
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_PLAIN), Dali::String(RESOURCE_PLAIN), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_TWO_POSITIONAL, 1, 5), Dali::String(RESOURCE_TWO_POSITIONAL), TEST_LOCATION);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_ONE_SEQUENTIAL, 1), Dali::String(RESOURCE_ONE_SEQUENTIAL), TEST_LOCATION);

  CleanupManager();

  // Bypass off again -> translated results.
  manager.SetLocalizedStringOverride(OverrideComponentsCatalog);
  DALI_TEST_EQUALS(ComponentsLocalization::GetLocalizedString(RESOURCE_TWO_POSITIONAL, 1, 5), Dali::String("Page 1 of 5."), TEST_LOCATION);

  CleanupManager();

  END_TEST;
}
