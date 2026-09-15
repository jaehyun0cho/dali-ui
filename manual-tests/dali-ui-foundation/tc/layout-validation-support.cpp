/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-support.h"
#include <dali-ui-foundation/public-api/configuration/ui-scale-manager.h>
#include <dali-ui-foundation/public-api/layouts/layout-controller.h>
#include <dali/public-api/adaptor-framework/ui-context.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace LayoutValidation
{
namespace
{
constexpr std::size_t PAGE_ROWS = 6;
template<std::size_t N> void Copy(char (&out)[N], const char* value)
{
  std::snprintf(out, N, "%s", value ? value : "");
}
std::string Escape(const char* value)
{
  std::string result;
  for(const unsigned char ch : std::string(value))
  {
    if(ch == '\\' || ch == '"') { result += '\\'; result += static_cast<char>(ch); }
    else if(ch == '\n') result += "\\n";
    else if(ch == '\r') result += "\\r";
    else if(ch == '\t') result += "\\t";
    else if(ch < 32) { char hex[7]; std::snprintf(hex, sizeof(hex), "\\u%04x", ch); result += hex; }
    else result += static_cast<char>(ch);
  }
  return result;
}
Label TextLabel(const char* text, float size)
{
  Label label = Label::New(text);
  label.SetFontSize(size);
  label.SetTextColor(UiColor(0x202124u));
  label.SetRequestedWidth(MATCH_PARENT);
  label.SetRequestedHeight(WRAP_CONTENT);
  return label;
}
} // namespace

Run::Run(View host, Window window)
: mHost(host), mWindow(window), mRows(new Result[MAX_RESULT_ROWS])
{
  mOwned.reserve(1024);
  mTimers.reserve(32);
  mCleanup.reserve(64);
  mRequiredViews.reserve(1024);
}
Run::~Run() { Cleanup(); }
bool Run::Cleanup()
{
  if(mCleaned) return mCleanupPassed;
  mCleaned = true;
  mPending = false;
  ++mActionEpoch;
  auto guard = [this](const std::function<void()>& action) {
    try { action(); }
    catch(...)
    {
      mCleanupPassed = false;
      const std::string record = "LR_CLEANUP_ERROR {\"tc\":\"" + Escape(mCaseId.c_str()) + "\",\"pid\":" + std::to_string(getpid()) + ",\"run\":" + std::to_string(mRunId) + ",\"scenario\":\"" + Escape(mScenarioId.c_str()) + "\"}\n";
      std::fputs(record.c_str(), stdout); std::fflush(stdout);
      std::fputs(record.c_str(), stderr); std::fflush(stderr);
    }
  };
  guard([this] { mFenceConnections.DisconnectAll(); DisconnectAll(); });
  for(auto& timer : mTimers) guard([&timer] { if(timer) timer.Stop(); });
  for(auto it = mCleanup.rbegin(); it != mCleanup.rend(); ++it) guard(*it);
  mAfterLayout = {};
  mState.reset();
  for(auto& view : mOwned) guard([&view] { if(view && view.GetParent()) view.Unparent(); });
  mOwned.clear();
  return mCleanupPassed;
}
void Run::SetIdentity(const char* tc, uint64_t run, const char* scenario, const char* step, uint64_t sequence)
{
  mCaseId = tc; mRunId = run; mScenarioId = scenario; mStepId = step; mActionSequence = sequence;
}
void Run::BeginAction(std::size_t requiredChecks)
{
  mFenceConnections.DisconnectAll();
  mAfterLayout = {};
  mRequiredViews.clear();
  mSeen.fill(false);
  ++mActionEpoch;
  mActionStart = mCount;
  mRequired = requiredChecks;
  mAutomaticPresentation = true;
  mPending = false;
  mFinished = false;
  if(requiredChecks == 0) Fail("protocol.required_checks", "zero required checks");
}
void Run::Finish()
{
  if(mFinished) return;
  if(mCount - mActionStart != mRequired) Fail("protocol.required_checks", "executed assertion count differs from the declared count");
  if(mCount == mActionStart) Fail("protocol.empty_action", "no observations");
  mPending = false;
  mFinished = true;
  std::printf("LR_READY tc=%s pid=%d run=%llu scenario=%s step=%s action_seq=%llu checks=%zu failures=%zu\n",
              mCaseId.c_str(), getpid(), static_cast<unsigned long long>(mRunId), mScenarioId.c_str(), mStepId.c_str(), static_cast<unsigned long long>(mActionSequence), mCount - mActionStart, mFailures);
  std::fflush(stdout);
}
void Run::Pending() { mPending = true; }
bool Run::IsPending() const { return mPending; }
bool Run::IsFinished() const { return mFinished; }
bool Run::HasFailed() const { return mFailures != 0 || mOverflow; }
std::size_t Run::ActionStart() const { return mActionStart; }
std::size_t Run::RowCount() const { return mCount; }
std::size_t Run::FailureCount() const { return mFailures; }
const Result& Run::Row(std::size_t index) const
{
  if(index >= mCount) throw std::out_of_range("layout result index");
  return mRows[index];
}
bool Run::HasOverflow() const { return mOverflow; }
bool Run::NeedsExternalVerification() const { return mExternalVerification; }
void Run::SuppressAutomaticPresentation() { mAutomaticPresentation = false; }
bool Run::AllowsAutomaticPresentation() const { return mAutomaticPresentation; }
void Run::RequireExternalVerification(const char* reason)
{
  mExternalVerification = true;
  std::printf("LR_EXTERNAL_REQUIRED %s\n", reason);
  std::fflush(stdout);
}
Result* Run::Append(const char* id)
{
  if(mFinished || mCount >= MAX_RESULT_ROWS)
  {
    mOverflow = true;
    ++mFailures;
    return nullptr;
  }
  Result& row = mRows[mCount++];
  row = Result{};
  Copy(row.id, id);
  Copy(row.step, mStepId.c_str());
  row.actionSequence = mActionSequence;
  if(!id || std::strlen(id) >= sizeof(row.id))
  {
    mOverflow = true;
    ++mFailures;
  }
  return &row;
}
void Run::Near(const char* id, double actual, double expected, double tolerance)
{
  auto* row = Append(id);
  if(!row) return;
  row->kind = "numeric";
  std::snprintf(row->actual, sizeof(row->actual), "%.17g", actual);
  std::snprintf(row->expected, sizeof(row->expected), "%.17g", expected);
  row->error = std::abs(actual - expected);
  row->tolerance = tolerance;
  row->passed = std::isfinite(actual) && std::isfinite(expected) && std::isfinite(tolerance) && tolerance >= 0 && row->error <= tolerance;
  if(!row->passed) ++mFailures;
}
void Run::Equal(const char* id, int64_t actual, int64_t expected)
{
  auto* row = Append(id);
  if(!row) return;
  row->kind = "integer";
  std::snprintf(row->actual, sizeof(row->actual), "%lld", static_cast<long long>(actual));
  std::snprintf(row->expected, sizeof(row->expected), "%lld", static_cast<long long>(expected));
  row->passed = actual == expected;
  if(!row->passed) ++mFailures;
}
void Run::Truth(const char* id, bool actual) { Equal(id, actual ? 1 : 0, 1); }
void Run::Text(const char* id, const std::string& actual, const std::string& expected)
{
  auto* row = Append(id);
  if(!row) return;
  row->kind = "text";
  Copy(row->actual, actual.c_str());
  Copy(row->expected, expected.c_str());
  row->passed = actual == expected && actual.size() < sizeof(row->actual) && expected.size() < sizeof(row->expected);
  if(!row->passed) ++mFailures;
}
void Run::Fail(const char* id, const char* reason)
{
  auto* row = Append(id);
  if(!row) return;
  Copy(row->actual, reason);
  Copy(row->expected, "required condition satisfied");
  ++mFailures;
}
void Run::Rect(const char* id, const LayoutRect& actual, const LayoutRect& expected, double tolerance)
{
  char field[160];
  std::snprintf(field, sizeof(field), "%s.x", id); Near(field, actual.x, expected.x, tolerance);
  std::snprintf(field, sizeof(field), "%s.y", id); Near(field, actual.y, expected.y, tolerance);
  std::snprintf(field, sizeof(field), "%s.width", id); Near(field, actual.width, expected.width, tolerance);
  std::snprintf(field, sizeof(field), "%s.height", id); Near(field, actual.height, expected.height, tolerance);
}
void Run::Rect(const char* id, View actual, const LayoutRect& expected, double tolerance) { Rect(id, Bounds(actual), expected, tolerance); }
void Run::Size(const char* id, const MeasuredSize& actual, const MeasuredSize& expected, double tolerance)
{
  char field[160];
  std::snprintf(field, sizeof(field), "%s.width", id); Near(field, actual.width, expected.width, tolerance);
  std::snprintf(field, sizeof(field), "%s.height", id); Near(field, actual.height, expected.height, tolerance);
}
View Run::Host() const { return mHost; }
Window Run::GetWindow() const { return mWindow; }
void Run::Attach(View root) { Keep(root); mHost.Add(root); }
void Run::Keep(View view) { mOwned.push_back(view); }
void Run::OnCleanup(std::function<void()> action) { mCleanup.push_back(std::move(action)); }
void Run::AfterLayout(const std::vector<View>& views, std::function<void(Run&)> action)
{
  AfterLayout(mWindow, views, std::move(action));
}
void Run::AfterLayout(Window window, const std::vector<View>& views, std::function<void(Run&)> action)
{
  mFenceConnections.DisconnectAll();
  if(views.empty() || views.size() > mSnapshots.size())
  {
    Fail("observer.required_views", "required view set is empty or exceeds capacity");
    return;
  }
  mRequiredViews = views;
  mSeen.fill(false);
  mAfterLayout = std::move(action);
  mPending = true;
  const auto epoch = mActionEpoch;
  for(std::size_t i = 0; i < views.size(); ++i)
  {
    mRequiredViews[i].LayoutFinishedSignal().Connect(&mFenceConnections, [this, epoch, i](View, const LayoutRect& bounds)
    {
      if(!mPending || mFinished || epoch != mActionEpoch) return;
      mSnapshots[i] = bounds;
      mSeen[i] = true;
    });
  }
  LayoutController::Get(window).LayoutFinishedSignal().Connect(&mFenceConnections, [this](Window target) { OnWindowFinished(target); });
}
void Run::OnWindowFinished(Window)
{
  if(!mPending || mFinished || !mAfterLayout) return;
  for(std::size_t i = 0; i < mRequiredViews.size(); ++i)
  {
    if(!mSeen[i])
    {
      Fail("observer.required_snapshot", "Window settled without all required View snapshots");
      Finish();
      return;
    }
  }
  mPending = false;
  auto action = std::move(mAfterLayout);
  try { action(*this); }
  catch(const Dali::DaliException& error) { Fail("observer.exception", error.condition); }
  catch(const std::exception& error) { Fail("observer.exception", error.what()); }
  catch(...) { Fail("observer.exception", "unexpected non-standard exception"); }
  if(!mPending) Finish();
}
LayoutRect Run::Snapshot(View view) const
{
  for(std::size_t i = 0; i < mRequiredViews.size(); ++i)
    if(mRequiredViews[i] == view && mSeen[i]) return mSnapshots[i];
  const float invalid = std::numeric_limits<float>::quiet_NaN();
  return LayoutRect(invalid, invalid, invalid, invalid);
}
void Run::Delay(uint32_t milliseconds, std::function<void(Run&)> action)
{
  Timer timer = Timer::New(std::max(1u, milliseconds));
  const auto epoch = mActionEpoch;
  mPending = true;
  timer.TickSignal().Connect(this, [this, epoch, action = std::move(action)]() mutable
  {
    if(epoch != mActionEpoch || mFinished) return false;
    mPending = false;
    try { action(*this); }
    catch(const Dali::DaliException& error) { Fail("timer.exception", error.condition); }
  catch(const std::exception& error) { Fail("timer.exception", error.what()); }
    catch(...) { Fail("timer.exception", "unexpected non-standard exception"); }
    if(!mPending) Finish();
    return false;
  });
  mTimers.push_back(timer);
  timer.Start();
}
bool Run::RequireDiagnostics()
{
#ifdef DALI_UI_LAYOUT_TEST_DIAGNOSTICS
  return true;
#else
  Fail("profile.diagnostics", "rebuild library and TC with DALI_UI_LAYOUT_TEST_DIAGNOSTICS");
  return false;
#endif
}
void Run::Sample(const char* metric, uint32_t iteration, double nanoseconds, uint32_t operations)
{
#ifdef DALI_UI_LAYOUT_TEST_DIAGNOSTICS
  const char* profile = "diagnostic";
#elif defined(DEBUG_ENABLED)
  const char* profile = "debug";
#else
  const char* profile = "release";
#endif
  std::printf("LR_SAMPLE {\"tc\":\"%s\",\"pid\":%d,\"run\":%llu,\"scenario\":\"%s\",\"step\":\"%s\",\"action_seq\":%llu,\"profile\":\"%s\",\"metric\":\"%s\",\"iteration\":%u,\"nanoseconds\":%.17g,\"operations\":%u}\n",
              mCaseId.c_str(), getpid(), static_cast<unsigned long long>(mRunId), mScenarioId.c_str(), mStepId.c_str(), static_cast<unsigned long long>(mActionSequence), profile, Escape(metric).c_str(), iteration, nanoseconds, operations);
}

Case::Case(const char* id, const char* title) : mId(id), mTitle(title) {}
Case::~Case() { OnExit(); }
Dali::String Case::GetName() const { return Dali::String((mId + ". Layout: " + mTitle).c_str()); }
Dali::String Case::GetDescription() const { return Dali::String(("Layout regression assertions: " + mTitle).c_str()); }
void Case::OnEnter(View contentArea)
{
  mContent = contentArea;
  mPresentationRoot = contentArea;
  for(View parent = View::DownCast(mPresentationRoot.GetParent()); parent; parent = View::DownCast(mPresentationRoot.GetParent())) mPresentationRoot = parent;
  mSavedRootScalePolicy = mPresentationRoot.GetUiScalePolicy();
  mPresentationRoot.SetUiScalePolicy(UiScalePolicy::DISABLED);
  mWindow = UiContext::Get().GetDefaultWindow();
  auto scale = UiScaleManager::Get();
  mSavedScale = scale.GetScale();
  mSavedScalable = scale.IsScalable();
  mScenarios = BuildScenarios();
  auto row1 = StackLayout::New(StackOrientation::HORIZONTAL);
  row1.SetRequestedWidth(MATCH_PARENT); row1.SetRequestedHeight(36);
  row1.Add(Button("Reset run", [this] { Reset(); }));
  row1.Add(Button("Next scenario", [this] { Next(); }));
  row1.Add(Button("Apply next step", [this] { Apply(); }));
  contentArea.Add(row1);
  auto row2 = StackLayout::New(StackOrientation::HORIZONTAL);
  row2.SetRequestedWidth(MATCH_PARENT); row2.SetRequestedHeight(36);
  row2.Add(Button("Read result", [this] { Read(); }));
  row2.Add(Button("Previous result page", [this] { ChangePage(-1); }));
  row2.Add(Button("Next result page", [this] { ChangePage(1); }));
  contentArea.Add(row2);
  mStatus = TextLabel("", 12);
  mStatus.SetRequestedHeight(96);
  mStatus.SetProperty(Actor::Property::NAME, Dali::String((mId + ".status").c_str()));
  mStatus.SetAccessibilityName(Dali::String((mId + ".status").c_str()));
  contentArea.Add(mStatus);
  mResult = TextLabel("", 11);
  mResult.SetRequestedHeight(240);
  mResult.SetProperty(Actor::Property::NAME, Dali::String((mId + ".result").c_str()));
  mResult.SetAccessibilityName(Dali::String((mId + ".result").c_str()));
  contentArea.Add(mResult);
  mFixtureHost = View::New();
  mFixtureHost.SetUiScalePolicy(UiScalePolicy::ENABLED);
  mFixtureHost.SetRequestedWidth(MATCH_PARENT);
  mFixtureHost.SetLayoutParams(StackLayoutParams::New().SetWeight(1));
  contentArea.Add(mFixtureHost);
  Reset();
}
View Case::Button(const char* label, std::function<void()> action)
{
  auto button = StackLayout::New(StackOrientation::VERTICAL);
  button.SetRequestedHeight(34);
  button.SetLayoutParams(StackLayoutParams::New().SetWeight(1));
  button.SetMargin(Insets(2, 2, 1, 1));
  button.SetBackgroundColor(UiColor(0xDCEAF8u));
  button.SetProperty(Actor::Property::NAME, Dali::String((mId + "." + label).c_str()));
  button.SetAccessibilityName(Dali::String((mId + "." + label).c_str()));
  button.Add(TextLabel(label, 11));
  button.AsInteractive().ClickedSignal().Connect(this, [action = std::move(action)](View, InputEvent) { action(); return true; });
  return button;
}
void Case::OnExit()
{
  if(!mContent) return;
  DisconnectAll();
  EndRun("exit");
  auto scale = UiScaleManager::Get();
  scale.SetScalable(false);
  scale.SetScale(mSavedScale);
  scale.SetScalable(mSavedScalable);
  if(mPresentationRoot) mPresentationRoot.SetUiScalePolicy(mSavedRootScalePolicy);
  mPresentationRoot.Reset();
  mScenarios.clear();
  mContent.Reset(); mFixtureHost.Reset(); mStatus.Reset(); mResult.Reset(); mWindow.Reset();
}
void Case::CloseScenario()
{
  if(!mRun) return;
  if(!mRun->Cleanup()) { ++mTotalFailures; mCleanupPassed = false; }
  mRun.reset();
}
void Case::EndRun(const char* reason)
{
  if(!mRun) return;
  CollectFinished();
  CloseScenario();
  std::printf("LR_CASE_END {\"tc\":\"%s\",\"pid\":%d,\"run\":%llu,\"completed_scenarios\":%zu,\"completed_steps\":%zu,\"failures\":%zu,\"cleanup_passed\":%s,\"reason\":\"%s\"}\n",
              mId.c_str(), getpid(), static_cast<unsigned long long>(mRunId), mCompletedScenarios, mCompletedSteps, mTotalFailures, mCleanupPassed ? "true" : "false", reason);
  std::fflush(stdout);
}
void Case::Reset()
{
  EndRun("reset");
  mFixtureHost.RemoveAll();
  UiScaleManager::Get().SetScalable(false);
  UiScaleManager::Get().SetScale(1.0f);
  mScenario = mStep = mPage = mCompletedScenarios = mCompletedSteps = mTotalFailures = mTotalChecks = 0;
  mLastCollectedRows = mLastCollectedFailures = 0;
  mStepInFlight = mHasExternalVerification = false;
  mCleanupPassed = true;
  mRunId = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
  mRun = std::make_unique<Run>(mFixtureHost, mWindow);
  mRun->SetIdentity(mId.c_str(), mRunId, mScenarios.empty() ? "" : mScenarios[mScenario].id.c_str(), "", mCompletedSteps);
  std::printf("LR_CASE_CONTRACT {\"tc\":\"%s\",\"pid\":%d,\"run\":%llu,\"scenarios\":[",mId.c_str(),getpid(),static_cast<unsigned long long>(mRunId));
  for(std::size_t i=0;i<mScenarios.size();++i)
  {
    if(i)std::printf(",");
    std::printf("{\"id\":\"%s\",\"steps\":[",Escape(mScenarios[i].id.c_str()).c_str());
    for(std::size_t j=0;j<mScenarios[i].steps.size();++j)
    {
      const auto& step=mScenarios[i].steps[j];if(j)std::printf(",");
      std::printf("{\"id\":\"%s\",\"required_checks\":%zu}",Escape(step.id.c_str()).c_str(),step.requiredChecks);
    }
    std::printf("]}");
  }
  std::printf("]}\n");std::fflush(stdout);
  Publish(true);
}
void Case::Apply()
{
  CollectFinished();
  if(mScenarios.empty() || mScenario >= mScenarios.size() || mStepInFlight || mStep >= mScenarios[mScenario].steps.size()) { Publish(false); return; }
  const auto& step = mScenarios[mScenario].steps[mStep];
  mRun->SetIdentity(mId.c_str(), mRunId, mScenarios[mScenario].id.c_str(), step.id.c_str(), mCompletedSteps + 1);
  mRun->BeginAction(step.requiredChecks);
  mStepInFlight = true;
  mPage = mRun->ActionStart() / PAGE_ROWS;
  std::printf("LR_ACTION {\"tc\":\"%s\",\"pid\":%d,\"run\":%llu,\"scenario\":\"%s\",\"step\":\"%s\",\"action_seq\":%zu,\"required_checks\":%zu}\n",
              mId.c_str(), getpid(), static_cast<unsigned long long>(mRunId), mScenarios[mScenario].id.c_str(), step.id.c_str(), mCompletedSteps + 1, step.requiredChecks);
  std::fflush(stdout);
  try { step.action(*mRun); }
  catch(const Dali::DaliException& error) { mRun->Fail("action.exception", error.condition); }
  catch(const std::exception& error) { mRun->Fail("action.exception", error.what()); }
  catch(...) { mRun->Fail("action.exception", "unexpected non-standard exception"); }
  if(!mRun->IsPending()) mRun->Finish();
  // A pending action must not be disturbed by a result-label invalidation.
  if(!mRun->IsPending()) { CollectFinished(); if(mRun->AllowsAutomaticPresentation()) Publish(true); }
}
void Case::CollectFinished()
{
  if(!mRun || !mRun->IsFinished()) return;
  mTotalChecks += mRun->RowCount() - mLastCollectedRows;
  mTotalFailures += mRun->FailureCount() - mLastCollectedFailures;
  mLastCollectedRows = mRun->RowCount();
  mLastCollectedFailures = mRun->FailureCount();
  mHasExternalVerification |= mRun->NeedsExternalVerification();
  if(!mStepInFlight) return;
  mStepInFlight = false;
  ++mStep;
  ++mCompletedSteps;
  if(mStep == mScenarios[mScenario].steps.size()) ++mCompletedScenarios;
}
void Case::Next()
{
  CollectFinished();
  if(mStepInFlight || mScenarios.empty() || mStep != mScenarios[mScenario].steps.size() || mScenario + 1 >= mScenarios.size()) { Publish(false); return; }
  CloseScenario();
  mFixtureHost.RemoveAll();
  UiScaleManager::Get().SetScalable(false);
  UiScaleManager::Get().SetScale(1.0f);
  ++mScenario;
  mStep = mPage = mLastCollectedRows = mLastCollectedFailures = 0;
  mRun = std::make_unique<Run>(mFixtureHost, mWindow);
  mRun->SetIdentity(mId.c_str(), mRunId, mScenarios.empty() ? "" : mScenarios[mScenario].id.c_str(), "", mCompletedSteps);
  Publish(true);
}
void Case::Read() { CollectFinished(); Publish(true, true); }
void Case::ChangePage(int direction)
{
  CollectFinished();
  if(direction < 0 && mPage) --mPage;
  if(direction > 0 && (mPage + 1) * PAGE_ROWS < mRun->RowCount()) ++mPage;
  Publish(true);
}
void Case::Publish(bool dump, bool allActionRows)
{
  if(!mRun) return;
  const bool all = !mScenarios.empty() && mCompletedScenarios == mScenarios.size();
  const char* verdict = mTotalFailures || mRun->HasFailed() ? "FAIL" : mStepInFlight ? "PENDING" : all ? (mHasExternalVerification ? "EXTERNAL_REQUIRED" : "PASS") : "INCOMPLETE";
  const auto rows = mRun->RowCount();
  const auto pages = std::max(std::size_t{1}, (rows + PAGE_ROWS - 1) / PAGE_ROWS);
  mPage = std::min(mPage, pages - 1);
  const auto first = mPage * PAGE_ROWS;
  const auto last = std::min(rows, first + PAGE_ROWS);
  std::ostringstream status;
  status << mId << " run=" << mRunId << " " << verdict << "\nscenario=" << (mScenarios.empty() ? "NONE" : mScenarios[mScenario].id)
         << " step=" << mStep << "/" << (mScenarios.empty() ? 0 : mScenarios[mScenario].steps.size())
         << " completed=" << mCompletedScenarios << "/" << mScenarios.size()
         << "\nchecks=" << mTotalChecks << " failures=" << mTotalFailures << " page=" << mPage + 1 << "/" << pages
         << " rows=" << (rows ? first + 1 : 0) << ".." << last << "/" << rows;
  std::ostringstream display;
  for(std::size_t i = first; i < last; ++i)
  {
    const auto& row = mRun->Row(i);
    display << (i + 1) << " " << (row.passed ? "PASS " : "FAIL ") << row.id << "\na=" << row.actual << " e=" << row.expected << " tol=" << row.tolerance << "\n";
  }
  // Pending reads report progress without invalidating the observed layout tree.
  if(!mStepInFlight)
  {
    mStatus.SetText(Dali::String(status.str().c_str()));
    mStatus.SetAccessibilityValue(Dali::String(status.str().c_str()));
    mResult.SetText(Dali::String(display.str().c_str()));
    mResult.SetAccessibilityValue(Dali::String(display.str().c_str()));
  }
  if(!dump) return;
  std::printf("LR_RESULT {\"tc\":\"%s\",\"pid\":%d,\"run\":%llu,\"scenario\":\"%s\",\"completed_steps\":%zu,\"completed_scenarios\":%zu,\"required_scenarios\":%zu,\"checks\":%zu,\"failures\":%zu,\"verdict\":\"%s\",\"page\":%zu,\"pages\":%zu,\"row_first\":%zu,\"row_last\":%zu,\"rows\":%zu}\n",
              mId.c_str(), getpid(), static_cast<unsigned long long>(mRunId), mScenarios.empty() ? "NONE" : mScenarios[mScenario].id.c_str(), mCompletedSteps, mCompletedScenarios, mScenarios.size(), mTotalChecks, mTotalFailures, verdict, mPage + 1, pages, rows ? first + 1 : 0, last, rows);
  const std::size_t dumpFirst = allActionRows ? mRun->ActionStart() : first;
  const std::size_t dumpLast = allActionRows ? rows : last;
  for(std::size_t i = dumpFirst; i < dumpLast; ++i)
  {
    const auto& row = mRun->Row(i);
    std::printf("LR_CHECK {\"tc\":\"%s\",\"pid\":%d,\"run\":%llu,\"scenario\":\"%s\",\"row\":%zu,\"step\":\"%s\",\"action_seq\":%llu,\"kind\":\"%s\",\"id\":\"%s\",\"actual\":\"%s\",\"expected\":\"%s\",\"tolerance\":%.17g,\"passed\":%s}\n",
                mId.c_str(), getpid(), static_cast<unsigned long long>(mRunId), mScenarios.empty() ? "NONE" : mScenarios[mScenario].id.c_str(), i + 1, Escape(row.step).c_str(), static_cast<unsigned long long>(row.actionSequence), row.kind, Escape(row.id).c_str(), Escape(row.actual).c_str(), Escape(row.expected).c_str(), row.tolerance, row.passed ? "true" : "false");
  }
  std::fflush(stdout);
}

View Leaf(float width, float height, const char* name)
{
  View view = View::New();
  view.SetRequestedWidth(width); view.SetRequestedHeight(height);
  view.SetProperty(Actor::Property::NAME, Dali::String(name));
  return view;
}
LayoutRect Bounds(View view)
{
  return LayoutRect(view.GetProperty<float>(Actor::Property::POSITION_X), view.GetProperty<float>(Actor::Property::POSITION_Y), view.GetProperty<float>(Actor::Property::SIZE_WIDTH), view.GetProperty<float>(Actor::Property::SIZE_HEIGHT));
}
Scenario Single(const char* id, const char* description, std::size_t requiredChecks, std::function<void(Run&)> action)
{
  return {id, description, {{"run", requiredChecks, std::move(action)}}};
}
double TimeNanoseconds(uint32_t iterations, const std::function<void()>& operation)
{
  const auto start = std::chrono::steady_clock::now();
  for(uint32_t i = 0; i < iterations; ++i) operation();
  return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count());
}
std::string Id(const char* prefix, uint32_t index) { return std::string(prefix) + "." + std::to_string(index); }
} // namespace LayoutValidation
