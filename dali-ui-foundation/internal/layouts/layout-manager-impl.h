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
#include <typeinfo>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/layouts/layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/layout-types.h>

namespace Dali
{
namespace Ui
{

/**
 * @brief Polymorphic base for a LayoutManager's implementation storage.
 *
 * A single instance is owned by LayoutManager (base-owned mImpl) and deleted
 * through this virtual destructor, so each concrete manager can subclass it to
 * hold its own state without changing the manager's instance size.
 *
 * This is also where the ARRANGE PURITY declaration lives, for two reasons:
 *
 *  - LayoutManager's virtual API is ABI-frozen, so purity cannot be a virtual;
 *    and LayoutManager itself is only ever a pointer to this storage, so a field
 *    here changes no public instance size. Every Impl -- this base and every
 *    concrete manager's subclass of it -- is allocated and destroyed inside
 *    libdali2-ui-foundation (the LayoutManager(Impl*) constructor is DALI_INTERNAL
 *    and documented as in-library only, and the default constructor allocates the
 *    base Impl in layout-manager.cpp), and this header is not installed -- only
 *    public-api/, integration-api/ and extension-api/ are -- so the layout of this
 *    class is not part of any external ABI and no external code can even name it.
 *  - keeping the SETTER here, rather than on LayoutManager, keeps the public
 *    header free of both a new method and a <typeinfo> dependency.
 */
class LayoutManager::Impl
{
public:
  virtual ~Impl() = default;

  /**
   * @brief Declares whether Arrange() may be skipped when its inputs are unchanged.
   *
   * A manager declares PURE when its Arrange() is a pure function of the bounds it
   * is handed, the owner's effective layout direction and effective scale, and
   * layout-tracked state (requested sizes, margins, padding, measured sizes, layout
   * params, the child list). A manager that reads live actor geometry -- the way
   * ScrollViewLayoutManager reads the scrolled child's current position -- must NOT
   * declare, and IMPURE is the default so that saying nothing is the safe answer.
   *
   * @param[in] purity        The declaration
   * @param[in] declaringType The type_info of the class making the declaration
   *
   * @note The declaration is per EXACT TYPE and is deliberately NOT inherited.
   *       Concrete managers are public, non-final classes with public constructors
   *       and no factory, so a declaration made in a constructor would otherwise run
   *       for every third-party subclass and silently claim purity for an override
   *       nobody vetted. Recording the declaring type and comparing it against the
   *       manager's most-derived type at read time (LayoutManager::IsArrangeProducerPure)
   *       is what makes a constructor a legal place to declare: it reproduces the
   *       guarantee ViewImpl::SetArrangePurity gets from being called only out of a
   *       type's own New() factory.
   */
  void DeclareArrangePurity(ArrangePurity purity, const std::type_info& declaringType)
  {
    mArrangePureType = (purity == ArrangePurity::PURE) ? &declaringType : nullptr;
  }

  /**
   * @brief Returns whether Arrange() was declared PURE for @p dynamicType.
   *
   * @param[in] dynamicType The most-derived type of the manager owning this storage
   * @return True only when a PURE declaration was made for exactly that type
   */
  bool IsArrangePureForType(const std::type_info& dynamicType) const
  {
    return mArrangePureType != nullptr && *mArrangePureType == dynamicType;
  }

private:
  /// The type that declared Arrange() PURE, or nullptr for the IMPURE default. The
  /// pointee is a static type_info with static storage duration, so it never dangles.
  const std::type_info* mArrangePureType{nullptr};
};

} // namespace Ui
} // namespace Dali
