/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#pragma once

#include "manual-test-case.h"
#include <dali/public-api/adaptor-framework/timer.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace LayoutValidation
{
using namespace Dali;
using namespace Dali::Ui;

constexpr double GEOMETRY_TOLERANCE = 0.001;
constexpr std::size_t MAX_RESULT_ROWS = 32768;

struct Result
{
  const char* kind{"failure"};
  char id[160]{};
  char step[96]{};
  uint64_t actionSequence{0};
  char actual[192]{};
  char expected[192]{};
  double error{0};
  double tolerance{0};
  bool passed{false};
};

class Run : public ConnectionTracker
{
public:
  Run(View host, Window window);
  ~Run() override;
  Run(const Run&) = delete;
  Run& operator=(const Run&) = delete;

  void SetIdentity(const char* tc, uint64_t run, const char* scenario, const char* step, uint64_t actionSequence);
  void BeginAction(std::size_t requiredChecks);
  void Finish();
  void Pending();
  bool IsPending() const;
  bool IsFinished() const;
  bool HasFailed() const;
  std::size_t ActionStart() const;
  std::size_t RowCount() const;
  std::size_t FailureCount() const;
  const Result& Row(std::size_t index) const;
  bool HasOverflow() const;
  bool NeedsExternalVerification() const;
  void RequireExternalVerification(const char* reason);
  void SuppressAutomaticPresentation();
  bool AllowsAutomaticPresentation() const;

  void Near(const char* id, double actual, double expected, double tolerance = GEOMETRY_TOLERANCE);
  void Equal(const char* id, int64_t actual, int64_t expected);
  void Truth(const char* id, bool actual);
  void Text(const char* id, const std::string& actual, const std::string& expected);
  void Fail(const char* id, const char* reason);
  void Rect(const char* id, const LayoutRect& actual, const LayoutRect& expected, double tolerance = GEOMETRY_TOLERANCE);
  void Rect(const char* id, View actual, const LayoutRect& expected, double tolerance = GEOMETRY_TOLERANCE);
  void Size(const char* id, const MeasuredSize& actual, const MeasuredSize& expected, double tolerance = GEOMETRY_TOLERANCE);

  View Host() const;
  Window GetWindow() const;
  void Attach(View root);
  void Keep(View view);
  void OnCleanup(std::function<void()> action);
  bool Cleanup();
  void AfterLayout(const std::vector<View>& requiredViews, std::function<void(Run&)> action);
  void AfterLayout(Window window, const std::vector<View>& requiredViews, std::function<void(Run&)> action);
  LayoutRect Snapshot(View view) const;
  void Delay(uint32_t milliseconds, std::function<void(Run&)> action);
  bool RequireDiagnostics();
  void Sample(const char* metric, uint32_t iteration, double nanoseconds, uint32_t operations);

  template<typename T> void SetState(std::shared_ptr<T> value) { mState = std::move(value); }
  template<typename T> std::shared_ptr<T> State() const { return std::static_pointer_cast<T>(mState); }

private:
  Result* Append(const char* id);
  void OnWindowFinished(Window window);
  std::string mCaseId, mScenarioId, mStepId;
  uint64_t mRunId{0}, mActionSequence{0};
  View mHost;
  Window mWindow;
  std::unique_ptr<Result[]> mRows;
  std::size_t mCount{0};
  std::size_t mFailures{0};
  std::size_t mActionStart{0};
  std::size_t mRequired{0};
  bool mOverflow{false};
  bool mPending{false};
  bool mFinished{false};
  bool mExternalVerification{false};
  bool mAutomaticPresentation{true};
  bool mCleaned{false};
  bool mCleanupPassed{true};
  uint64_t mActionEpoch{0};
  std::shared_ptr<void> mState;
  std::vector<View> mOwned;
  std::vector<Timer> mTimers;
  std::vector<std::function<void()>> mCleanup;
  ConnectionTracker mFenceConnections;
  std::vector<View> mRequiredViews;
  std::array<LayoutRect, 1024> mSnapshots{};
  std::array<bool, 1024> mSeen{};
  std::function<void(Run&)> mAfterLayout;
};

struct Step
{
  std::string id;
  std::size_t requiredChecks;
  std::function<void(Run&)> action;
};

struct Scenario
{
  std::string id;
  std::string description;
  std::vector<Step> steps;
};

class Case : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Case(const char* id, const char* title);
  ~Case() override;
  Dali::String GetName() const override;
  Dali::String GetDescription() const override;
  void OnEnter(View contentArea) override;
  void OnExit() override;
  virtual std::vector<Scenario> BuildScenarios() = 0;

private:
  View Button(const char* label, std::function<void()> action);
  void Reset();
  void Apply();
  void Next();
  void Read();
  void Publish(bool dump, bool allActionRows = false);
  void CollectFinished();
  void ChangePage(int direction);
  void CloseScenario();
  void EndRun(const char* reason);
  std::string mId;
  std::string mTitle;
  View mContent;
  View mFixtureHost;
  View mPresentationRoot;
  UiScalePolicy mSavedRootScalePolicy{UiScalePolicy::INHERIT};
  Label mStatus;
  Label mResult;
  Window mWindow;
  std::vector<Scenario> mScenarios;
  std::unique_ptr<Run> mRun;
  std::size_t mScenario{0};
  std::size_t mStep{0};
  std::size_t mPage{0};
  std::size_t mCompletedScenarios{0};
  std::size_t mCompletedSteps{0};
  std::size_t mTotalFailures{0};
  std::size_t mTotalChecks{0};
  std::size_t mLastCollectedRows{0};
  std::size_t mLastCollectedFailures{0};
  bool mStepInFlight{false};
  bool mHasExternalVerification{false};
  bool mCleanupPassed{true};
  uint64_t mRunId{0};
  float mSavedScale{1};
  bool mSavedScalable{false};
};

View Leaf(float width, float height, const char* name = "leaf");
LayoutRect Bounds(View view);
Scenario Single(const char* id, const char* description, std::size_t requiredChecks, std::function<void(Run&)> action);
double TimeNanoseconds(uint32_t iterations, const std::function<void()>& operation);
std::string Id(const char* prefix, uint32_t index);
} // namespace LayoutValidation
