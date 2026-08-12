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

#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <dali.h>
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/extension-api/screen-reader-localization.h>
#include <dali-ui-foundation/public-api/configuration/ui-localization-manager.h>
#include <dali-ui-foundation/public-api/types/callback.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <dali-ui-test-suite-utils.h>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
// ---- Test constants ----

const char* const SCREEN_READER_DOMAIN = "screen-reader";
const char* const TEST_RESOURCE_ID     = "IDS_ACCS_TBOPT_BUTTON";
const char* const TEST_TRANSLATION     = "ScreenReader Translated";

// ---- StringView comparison helper ----

bool EqualsStringView(StringView view, const char* text)
{
  const size_t len = strlen(text);
  return view.Size() == len &&
         view.Data() != nullptr &&
         strncmp(view.Data(), text, len) == 0;
}

// ---- Override functions ----

// Answers only for the screen reader domain, so a passing test proves the
// helper forwarded that domain explicitly rather than relying on the default.
bool OverrideScreenReaderDomain(StringView resourceId, StringView domain, Dali::String& outString)
{
  if(EqualsStringView(domain, SCREEN_READER_DOMAIN))
  {
    outString = Dali::String(TEST_TRANSLATION);
    return true;
  }
  return false;
}

// ---- Binding apply state ----

int gApplyCallCount = 0;

void TestApplyFunc(BaseHandle target, const Dali::String& localized)
{
  gApplyCallCount++;
}

// ---- Cleanup helper ----

// The domain registration latch inside the helper is process-global, so no
// test may assume it performs the first registration. Only manager state that
// the helper actually reads is reset here.
void CleanupManager(UiLocalizationManager& manager, BaseHandle target = BaseHandle())
{
  if(target)
  {
    manager.ClearBindings(target);
  }

  manager.ClearLocalizedStringOverride();
  manager.SetBypassEnabled(false);
  manager.SetDefaultDomain("");
}

} // namespace

void utc_dali_screenreaderlocalization_startup(void)
{
  test_return_value = TET_UNDEF;
  gApplyCallCount   = 0;
}

void utc_dali_screenreaderlocalization_cleanup(void)
{
  test_return_value = TET_PASS;
}

// === Domain forwarding ===

int UtcDaliScreenReaderLocalizationGetLocalizedStringDomainP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager(manager);

  manager.SetLocalizedStringOverride(OverrideScreenReaderDomain);

  Dali::String result = Extension::ScreenReaderLocalization::GetLocalizedString(TEST_RESOURCE_ID);
  DALI_TEST_EQUALS(result, Dali::String(TEST_TRANSLATION), TEST_LOCATION);

  CleanupManager(manager);

  END_TEST;
}

int UtcDaliScreenReaderLocalizationTryGetLocalizedStringP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager(manager);

  manager.SetLocalizedStringOverride(OverrideScreenReaderDomain);

  Dali::String outString;
  bool         found = Extension::ScreenReaderLocalization::TryGetLocalizedString(TEST_RESOURCE_ID, outString);

  DALI_TEST_CHECK(found);
  DALI_TEST_EQUALS(outString, Dali::String(TEST_TRANSLATION), TEST_LOCATION);

  CleanupManager(manager);

  END_TEST;
}

// === Fallback ===

int UtcDaliScreenReaderLocalizationFallbackN(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager(manager);

  // No override and no catalog message -> reported as untranslated
  Dali::String outString;
  bool         found = Extension::ScreenReaderLocalization::TryGetLocalizedString(TEST_RESOURCE_ID, outString);

  DALI_TEST_CHECK(!found);
  DALI_TEST_EQUALS(outString, Dali::String(TEST_RESOURCE_ID), TEST_LOCATION);

  Dali::String result = Extension::ScreenReaderLocalization::GetLocalizedString(TEST_RESOURCE_ID);
  DALI_TEST_EQUALS(result, Dali::String(TEST_RESOURCE_ID), TEST_LOCATION);

  CleanupManager(manager);

  END_TEST;
}

// === Empty resource id ===

int UtcDaliScreenReaderLocalizationEmptyResourceIdN(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager(manager);

  Dali::String outString("stale value");
  bool         found = Extension::ScreenReaderLocalization::TryGetLocalizedString("", outString);

  DALI_TEST_CHECK(!found);
  DALI_TEST_CHECK(outString.Empty());

  Dali::String result = Extension::ScreenReaderLocalization::GetLocalizedString("");
  DALI_TEST_CHECK(result.Empty());

  CleanupManager(manager);

  END_TEST;
}

// === Default domain is left alone ===

int UtcDaliScreenReaderLocalizationDoesNotChangeDefaultDomainP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager(manager);

  DALI_TEST_EQUALS(manager.GetDefaultDomain(), Dali::String(""), TEST_LOCATION);

  Extension::ScreenReaderLocalization::GetLocalizedString(TEST_RESOURCE_ID);

  Dali::String outString;
  Extension::ScreenReaderLocalization::TryGetLocalizedString(TEST_RESOURCE_ID, outString);

  DALI_TEST_EQUALS(manager.GetDefaultDomain(), Dali::String(""), TEST_LOCATION);

  CleanupManager(manager);

  END_TEST;
}

// === Repeated lookups are stable ===

int UtcDaliScreenReaderLocalizationRepeatedCallP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager(manager);

  manager.SetLocalizedStringOverride(OverrideScreenReaderDomain);

  Dali::String first;
  bool         foundFirst = Extension::ScreenReaderLocalization::TryGetLocalizedString(TEST_RESOURCE_ID, first);

  Dali::String second;
  bool         foundSecond = Extension::ScreenReaderLocalization::TryGetLocalizedString(TEST_RESOURCE_ID, second);

  // The override is installed, so the lookup must actually resolve; otherwise
  // the equality checks below would pass on two identical failures.
  DALI_TEST_CHECK(foundFirst);
  DALI_TEST_EQUALS(foundFirst, foundSecond, TEST_LOCATION);
  DALI_TEST_EQUALS(first, second, TEST_LOCATION);

  CleanupManager(manager);

  END_TEST;
}

// === Domain is registered at most once ===

int UtcDaliScreenReaderLocalizationRegisterOnceP(void)
{
  UiTestApplication application;

  UiLocalizationManager manager = UiLocalizationManager::Get();
  CleanupManager(manager);

  View view = View::New();
  manager.SetLocalizedStringOverride(OverrideScreenReaderDomain);

  // Establish a binding whose callback counts every refresh
  gApplyCallCount = 0;
  manager.SetBindingResource(view, "Text", "IDS_TEST", LocalizedStringCallback::New(TestApplyFunc));

  // The first lookup may perform the one-time domain registration, and
  // RegisterDomain() refreshes every binding when it succeeds.
  Extension::ScreenReaderLocalization::GetLocalizedString(TEST_RESOURCE_ID);
  const int countAfterFirstLookup = gApplyCallCount;

  // The second lookup must not register again, so it must not refresh bindings.
  Extension::ScreenReaderLocalization::GetLocalizedString(TEST_RESOURCE_ID);
  DALI_TEST_EQUALS(gApplyCallCount, countAfterFirstLookup, TEST_LOCATION);

  CleanupManager(manager, view);

  END_TEST;
}
