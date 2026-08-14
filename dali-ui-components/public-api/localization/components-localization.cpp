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
#include <dali-ui-components/public-api/localization/components-localization.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/configuration/ui-localization-manager.h>

// EXTERNAL INCLUDES
#include <dali/devel-api/adaptor-framework/environment-variable.h>

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

// <libintl.h> must stay the last include of this translation unit.
//
// On Windows it defines snprintf as libintl_snprintf, the only implementation
// there that understands the positional "%1$d" specifiers used by translated
// format strings. Included before <cstdio>, that macro would also poison the
// std::snprintf declaration and break the build, so <cstdio> comes first and
// this include comes last.
#if !defined(_WIN32) || defined(DALI_UI_HAS_GETTEXT)
#include <libintl.h>
#endif

namespace
{
#define TOKEN_STRING(x) #x

constexpr const char* COMPONENTS_LOCALE_DOMAIN = DALI_UI_COMPONENTS_LOCALE_DOMAIN;

/// Size of the stack buffer used before falling back to a heap buffer.
constexpr size_t FORMAT_STACK_BUFFER_SIZE = 256u;

/**
 * @brief Gets the root directory of the components message catalog.
 *
 * @note DALI_UI_COMPONENTS_LOCALE_DIR is a macro that can be defined either with a
 *       file system path or zero. If it's defined as zero then the value is retrieved
 *       from an environment variable named DALI_UI_COMPONENTS_LOCALE_DIR.
 *
 * @return The locale root directory, or an empty string when it is unknown
 */
std::string GetLocaleDirectory()
{
  const char* directory = (nullptr == DALI_UI_COMPONENTS_LOCALE_DIR)
                            ? Dali::EnvironmentVariable::GetEnvironmentVariable(TOKEN_STRING(DALI_UI_COMPONENTS_LOCALE_DIR))
                            : DALI_UI_COMPONENTS_LOCALE_DIR;

  return (directory != nullptr) ? std::string(directory) : std::string();
}

/**
 * @brief Binds the components message catalog domain once per process.
 *
 * The latch is only set once the binding succeeds. A lookup made before the
 * localization manager exists therefore does not disable the catalog for the
 * rest of the process: the next call tries again. After success the binding is
 * not repeated, because RegisterDomain() refreshes every existing localization
 * binding. The mutex makes the check-then-register step atomic, so concurrent
 * first lookups cannot race on the latch or bind the domain twice.
 *
 * @return true if the domain is bound
 */
bool EnsureRegistered()
{
  static std::mutex gMutex;
  static bool       gRegistered = false;

  std::lock_guard<std::mutex> lock(gMutex);

  if(gRegistered)
  {
    return true;
  }

  const std::string localeDirectory = GetLocaleDirectory();
  if(localeDirectory.empty())
  {
    return false;
  }

  Dali::Ui::UiLocalizationManager manager = Dali::Ui::UiLocalizationManager::Get();
  if(!manager)
  {
    return false;
  }

  if(!manager.RegisterDomain(COMPONENTS_LOCALE_DOMAIN, localeDirectory.c_str()))
  {
    return false;
  }

  gRegistered = true;
  return true;
}

/**
 * @brief Returns whether a catalog string is a format string matching the arguments.
 *
 * Translated text is untrusted data, so it must never reach snprintf() unchecked.
 * argumentTypes describes the arguments the caller will pass, one character per
 * argument in order: 'd' for an integer, 's' for a string. Only these sequences
 * are accepted, and every conversion must match the type of the argument it
 * consumes:
 *   - "%%"                                     : a literal percent
 *   - "%<index>$d", "%<index>$i", "%<index>$s" : a positional argument,
 *     index in [1, argument count]
 *   - "%d", "%i", "%s"                         : a sequential argument, at most
 *     argument-count of them
 *
 * Anything else is rejected, including write-back ("%n") conversions, width and
 * precision fields, a mix of positional and sequential conversions, and any
 * conversion whose type does not match the supplied argument.
 *
 * Positional indices must also be contiguous from 1 up to the highest one used.
 * A translation may drop trailing arguments, e.g. use only "%1$d" of two, but it
 * may not leave a hole such as "%2$d" on its own: how an unreferenced positional
 * argument is treated differs between libc implementations, and silently
 * dropping a value the caller supplied is wrong for a translation regardless.
 *
 * @param[in] format The candidate format string
 * @param[in] argumentTypes The argument types, e.g. "d", "dd", "s", "ss", "sd"
 * @return true if the format string is safe to pass to snprintf()
 */
bool IsMatchingFormat(const char* format, const char* argumentTypes)
{
  const size_t typeCount = strlen(argumentTypes);

  // One bit of positionalMask tracks each index, so the count must fit it.
  if(typeCount > 31u)
  {
    return false;
  }
  const int32_t argumentCount = static_cast<int32_t>(typeCount);

  bool     positionalUsed  = false;
  bool     sequentialUsed  = false;
  int32_t  sequentialCount = 0;
  uint32_t positionalMask  = 0u;

  for(const char* cursor = format; *cursor != '\0'; ++cursor)
  {
    if(*cursor != '%')
    {
      continue;
    }

    ++cursor;

    if(*cursor == '%')
    {
      // A literal percent. The loop increment steps past the second character.
      continue;
    }

    if(*cursor >= '1' && *cursor <= '9')
    {
      int32_t index = 0;
      while(*cursor >= '0' && *cursor <= '9')
      {
        index = index * 10 + (*cursor - '0');
        if(index > argumentCount)
        {
          // Out of range. Also keeps a long digit run from overflowing.
          return false;
        }
        ++cursor;
      }

      if(*cursor != '$')
      {
        return false;
      }
      ++cursor;

      const char expectedType = argumentTypes[index - 1];
      const bool matches      = (expectedType == 'd') ? (*cursor == 'd' || *cursor == 'i')
                                                      : (expectedType == 's' && *cursor == 's');
      if(!matches)
      {
        return false;
      }

      positionalUsed = true;
      positionalMask |= (1u << (index - 1));
    }
    else if(*cursor == 'd' || *cursor == 'i' || *cursor == 's')
    {
      ++sequentialCount;
      if(sequentialCount > argumentCount)
      {
        return false;
      }

      const char expectedType = argumentTypes[sequentialCount - 1];
      const bool matches      = (expectedType == 'd') ? (*cursor == 'd' || *cursor == 'i')
                                                      : (expectedType == 's' && *cursor == 's');
      if(!matches)
      {
        return false;
      }

      sequentialUsed = true;
    }
    else
    {
      // Rejects every other conversion, and a '%' at the end of the string.
      return false;
    }

    if(positionalUsed && sequentialUsed)
    {
      return false;
    }
  }

  // Reject a hole in the positional indices, e.g. "%2$d" without "%1$d".
  // (mask & (mask + 1)) is zero only when the set bits form an unbroken run
  // starting at bit 0, i.e. indices 1..N with nothing skipped.
  if((positionalMask & (positionalMask + 1u)) != 0u)
  {
    return false;
  }

  return true;
}

/**
 * @brief Formats a validated format string with the given arguments.
 *
 * @param[in] format The format string, already accepted by IsMatchingFormat()
 * @param[in] arguments The argument values matching the validated types
 * @return The formatted string, or the unformatted format string on failure
 */
template<typename... ARGUMENTS>
Dali::String FormatArguments(const std::string& format, ARGUMENTS... arguments)
{
  char stackBuffer[FORMAT_STACK_BUFFER_SIZE];

  // snprintf must stay unqualified. On Windows <libintl.h> maps it to
  // libintl_snprintf, which is the implementation that supports positional
  // specifiers there; std::snprintf would not.
  const int required = snprintf(stackBuffer, FORMAT_STACK_BUFFER_SIZE, format.c_str(), arguments...);

  if(required < 0)
  {
    return Dali::String(format.c_str());
  }

  if(static_cast<size_t>(required) < FORMAT_STACK_BUFFER_SIZE)
  {
    return Dali::String(stackBuffer);
  }

  std::vector<char> heapBuffer(static_cast<size_t>(required) + 1u);

  const int written = snprintf(heapBuffer.data(), heapBuffer.size(), format.c_str(), arguments...);

  if(written < 0)
  {
    return Dali::String(format.c_str());
  }

  return Dali::String(heapBuffer.data());
}

/**
 * @brief Copies a string view into a null-terminated string.
 *
 * StringView is not guaranteed to be null-terminated, but the "%s" conversion
 * consumes a null-terminated C string.
 *
 * @param[in] view The view to copy
 * @return The copied string
 */
std::string ToTerminatedString(Dali::StringView view)
{
  return (view.Data() != nullptr) ? std::string(view.Data(), view.Size()) : std::string();
}

/**
 * @brief Looks a resource id up in the components message catalog.
 *
 * The domain is always passed explicitly, so the application default domain is
 * neither read nor changed.
 *
 * @param[in] resourceId The localization resource id / msgid
 * @return The localized string
 */
Dali::String LookUp(Dali::StringView resourceId)
{
  EnsureRegistered();

  Dali::Ui::UiLocalizationManager manager = Dali::Ui::UiLocalizationManager::Get();
  if(!manager)
  {
    // No manager yet: fall back to the documented untranslated result.
    return Dali::String(resourceId);
  }

  return manager.GetLocalizedString(resourceId, COMPONENTS_LOCALE_DOMAIN);
}

/**
 * @brief Looks a resource id up and formats it with the given arguments.
 *
 * @param[in] resourceId The localization resource id / msgid
 * @param[in] argumentTypes The argument types, one character each: 'd' or 's'
 * @param[in] arguments The argument values matching argumentTypes
 * @return The formatted localized string
 */
template<typename... ARGUMENTS>
Dali::String LookUpAndFormat(Dali::StringView resourceId, const char* argumentTypes, ARGUMENTS... arguments)
{
  Dali::String localized = LookUp(resourceId);
  if(localized.Empty())
  {
    return localized;
  }

  const std::string format(localized.CStr());
  if(!IsMatchingFormat(format.c_str(), argumentTypes))
  {
    // The catalog text is untrusted. Return it unformatted instead of feeding
    // an unexpected format string to snprintf().
    return localized;
  }

  return FormatArguments(format, arguments...);
}

} // unnamed namespace

namespace Dali
{
namespace Ui
{
namespace Components
{
namespace Localization
{

Dali::String GetDomain()
{
  EnsureRegistered();

  return Dali::String(COMPONENTS_LOCALE_DOMAIN);
}

Dali::String GetLocalizedString(StringView resourceId)
{
  return LookUp(resourceId);
}

Dali::String GetLocalizedString(StringView resourceId, int32_t value1)
{
  return LookUpAndFormat(resourceId, "d", value1);
}

Dali::String GetLocalizedString(StringView resourceId, int32_t value1, int32_t value2)
{
  return LookUpAndFormat(resourceId, "dd", value1, value2);
}

Dali::String GetLocalizedString(StringView resourceId, StringView value1)
{
  const std::string firstValue = ToTerminatedString(value1);
  return LookUpAndFormat(resourceId, "s", firstValue.c_str());
}

Dali::String GetLocalizedString(StringView resourceId, StringView value1, StringView value2)
{
  const std::string firstValue  = ToTerminatedString(value1);
  const std::string secondValue = ToTerminatedString(value2);
  return LookUpAndFormat(resourceId, "ss", firstValue.c_str(), secondValue.c_str());
}

Dali::String GetLocalizedString(StringView resourceId, StringView value1, int32_t value2)
{
  const std::string firstValue = ToTerminatedString(value1);
  return LookUpAndFormat(resourceId, "sd", firstValue.c_str(), value2);
}

} // namespace Localization
} // namespace Components
} // namespace Ui
} // namespace Dali
