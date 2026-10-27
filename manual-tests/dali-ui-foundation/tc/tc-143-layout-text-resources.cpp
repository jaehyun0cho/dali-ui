/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-fixtures.h"

#include <dali-ui-foundation/integration-api/layout-test-diagnostics.h>
#include <dali-ui-foundation/public-api/views/image/image-view.h>
#include <dali/devel-api/actors/actor-devel.h>
#include <dali/devel-api/text-abstraction/font-client.h>
#include <cmath>
#include <filesystem>

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;
namespace
{
namespace Diagnostic = Dali::Ui::Integration::LayoutTestDiagnostics;
struct TextFixture
{
  Label                                                                         label;
  bool                                                                          armed{false}, windowReady{false}, queued{false};
  uint32_t                                                                      relayouts{0};
  Diagnostic::TextSnapshot                                                      snapshot{};
  LayoutRect                                                                    target{};
  std::function<void(Run&, const Diagnostic::TextSnapshot&, const LayoutRect&)> validate;
};
// The fixed 100x120 frame has no padding, so the content box is 100x120. The Label lays the text
// out in that box, then gives the text visual a render box of the text's own height,
// min(layout height, 120), and places it in the frame at the vertical alignment offset
// round((120 - box) * factor). The visual relayouts the same controller at the render box, so the
// model's control size ends as that box. With the default line spacing 0 the layout height is
// ceil(lines * (ascender - descender)), and both metrics are pinned to the font file below.
void CheckText(Run& r, const Diagnostic::TextSnapshot& t, const LayoutRect& target, uint32_t lines, uint32_t glyphs, float fontSize, float alignment)
{
  r.Truth("text.model.valid", t.valid);
  r.Truth("text.model.ready", t.ready);
  r.Equal("text.lines", t.lineCount, lines);
  r.Equal("text.glyphs", t.glyphCount, glyphs);
  r.Rect("text.target", target, LayoutRect(0, 0, 100, 120));
  r.Near("text.control.width", t.controlWidth, 100);
  const float renderBox = std::min(std::ceil(static_cast<float>(lines) * (t.firstAscender - t.firstDescender)), 120.0f);
  r.Near("text.control.height", t.controlHeight, renderBox);
  r.Near("text.render.offset.y", t.renderedOffsetY, std::round((120.0f - renderBox) * alignment));
  r.Truth("text.font.resolved", t.firstGlyphFontId != 0);
  Dali::TextAbstraction::FontDescription description;
  Dali::TextAbstraction::FontClient::Get().GetDescription(t.firstGlyphFontId, description);
  r.Text("text.font.path", std::filesystem::weakly_canonical(description.path).string(), std::filesystem::weakly_canonical(TEST_RESOURCE_DIR "/layout-validation/DejaVuSans.ttf").string());
  r.Equal("text.glyph.A", t.firstGlyphIndex, 36);
  r.Near("text.advance.A", t.firstGlyphAdvance, 1401.0 / 2048.0 * fontSize, 1.0);
  r.Near("text.ascender", t.firstAscender, 1901.0 / 2048.0 * fontSize, 1.0);
  r.Near("text.descender", t.firstDescender, -483.0 / 2048.0 * fontSize, 1.0);
  r.Truth("text.baseline.in.control", std::isfinite(t.firstBaseline) && t.firstBaseline > 0 && t.firstBaseline < 120);
}
void QueueTextResult(Run& r, const std::shared_ptr<TextFixture>& s)
{
  if(!s->armed || s->queued || !s->windowReady || s->relayouts == 0 || !s->snapshot.ready) return;
  s->armed  = false;
  s->queued = true;
  // The signal callback copies the completed model. Font metadata lookup and checks
  // use that frozen copy after the core relayout stack has unwound.
  r.Delay(1, [s](Run& done)
  {
    done.Truth("text.relayout.after-action", s->relayouts > 0);
    s->validate(done, s->snapshot, s->target);
  });
}
std::shared_ptr<TextFixture> MakeTextFixture(Run& r, Label label)
{
  auto s   = std::make_shared<TextFixture>();
  s->label = label;
  DevelActor::OnRelayoutSignal(label).Connect(&r, [&r, s](Actor)
  {
    if(!s->armed) return;
    ++s->relayouts;
    s->snapshot = Diagnostic::GetTextSnapshot(s->label);
    s->target   = LayoutValidation::Bounds(s->label);
    QueueTextResult(r, s);
  });
  r.OnCleanup([s]()
  {s->armed=false;s->validate={}; });
  return s;
}
void ArmText(Run& r, const std::shared_ptr<TextFixture>& s, bool first,
             std::function<void(Run&, const Diagnostic::TextSnapshot&, const LayoutRect&)> validate)
{
  s->armed       = true;
  s->queued      = false;
  s->relayouts   = 0;
  s->snapshot    = {};
  s->windowReady = !first;
  s->validate    = std::move(validate);
  r.Pending();
  if(first)
  {
    r.AfterLayout({s->label}, [s](Run& done)
    {
      s->windowReady = true;
      QueueTextResult(done, s);
      if(!s->queued) done.Pending();
    });
  }
}
void ArmLatin(Run& r, const std::shared_ptr<TextFixture>& s, bool first, uint32_t lines, uint32_t glyphs, float size, float alignment = 0.0f)
{
  ArmText(r, s, first, [lines, glyphs, size, alignment](Run& done, const Diagnostic::TextSnapshot& model, const LayoutRect& target)
  { CheckText(done, model, target, lines, glyphs, size, alignment); });
}

struct AsyncImage
{
  ImageView              image;
  bool                   armed{false}, resourceReady{false}, childSeen{false}, queued{false};
  uint32_t               readyCount{0}, windowEpisodes{0};
  int                    width{37}, height{19};
  Visual::ResourceStatus status{};
  LayoutRect             pendingSnapshot{}, frozenSnapshot{};
};
void QueueImageResult(Run& r, const std::shared_ptr<AsyncImage>& s)
{
  if(!s->armed || s->queued || !s->resourceReady || s->windowEpisodes == 0) return;
  s->queued = true;
  // No layout mutation: allow the callback stack and any naturally queued resource
  // layout to settle, retaining newer complete Window snapshots until this read.
  r.Delay(100, [s](Run& done)
  {
    s->armed = false;
    done.Equal("image.ready.count", s->readyCount, 1);
    done.Equal("image.ready.status", static_cast<int>(s->status), static_cast<int>(Visual::ResourceStatus::READY));
    done.Truth("image.request.window-observed", s->windowEpisodes > 0);
    done.Rect("image.async.target", s->frozenSnapshot, LayoutRect(0, 0, s->width, s->height));
    done.Rendered("image.current.target", s->image, LayoutRect(0, 0, s->width, s->height));
    done.Truth("image.async.on.scene", s->image.GetProperty<bool>(Actor::Property::CONNECTED_TO_SCENE));
  });
}
std::shared_ptr<AsyncImage> MakeAsyncImage(Run& r)
{
  auto s   = std::make_shared<AsyncImage>();
  s->image = ImageView::New();
  s->image.SetRequestedWidth(WRAP_CONTENT);
  s->image.SetRequestedHeight(WRAP_CONTENT);
  s->image.ResourceReadySignal().Connect(&r, [&r, s](View)
  {
    if(!s->armed) return;
    ++s->readyCount;
    s->resourceReady = true;
    s->status        = s->image.GetLoadingStatus();
    QueueImageResult(r, s);
  });
  s->image.LayoutFinishedSignal().Connect(&r, [s](View, const LayoutRect& bounds)
  {
    if(!s->armed) return;
    s->pendingSnapshot = bounds;
    s->childSeen       = true;
  });
  LayoutController::Get(r.GetWindow()).LayoutFinishedSignal().Connect(&r, [&r, s](Window)
  {
    if(!s->armed) return;
    if(s->childSeen)
    {
      s->frozenSnapshot = s->pendingSnapshot;
      ++s->windowEpisodes;
    }
    // Never combine one child's observation with a later, unrelated episode.
    s->childSeen = false;
    QueueImageResult(r, s);
  });
  r.OnCleanup([s]()
  { s->armed = false; });
  return s;
}
void ChangeImage(Run& r, const std::shared_ptr<AsyncImage>& s, int width, int height, bool attach)
{
  s->width         = width;
  s->height        = height;
  s->armed         = true;
  s->resourceReady = false;
  s->childSeen     = false;
  s->queued        = false;
  s->readyCount = s->windowEpisodes = 0;
  s->pendingSnapshot                = {};
  s->frozenSnapshot                 = {};
  r.Pending();
  s->image.SetResourceUrl(Dali::String((std::string(TEST_RESOURCE_DIR) + "/layout-validation/image-" + std::to_string(width) + "x" + std::to_string(height) + ".png").c_str()));
  if(attach) r.Attach(s->image);
}
struct IntrinsicText
{
  Label label;
  View  stage;
};
struct SyncImage
{
  ImageView image;
  View      stage;
};
struct LaidOutText
{
  Label label;
  View  stage, stack, seed;
};
// A multi-line DejaVu Sans 24 Label with CHARACTER wrap: glyph 'A' advances 1401/2048 x 24 =
// 16.418 px, so floor(w / 16.418) characters fit a line of width w and k lines are 29k +- k high.
Label WrappingLabel(const char* text)
{
  Label label = Label::New(text);
  label.SetFontFamily("DejaVu Sans");
  label.SetFontSize(24);
  label.SetMultiLine(true);
  label.SetLineWrapMode(Text::LineWrapMode::CHARACTER);
  return label;
}
class Tc143 : public Case
{
public:
  Tc143()
  : Case("TC143", "Text and image resources")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> result;
    result.push_back({"text-model", "Locked font and fresh core relayout model after fixed-size text changes", {{"single-A", 19, [](Run& r)
    {
      auto label = Label::New("A");
      label.SetFontFamily("DejaVu Sans");
      label.SetFontSize(24);
      label.SetMultiLine(true);
      label.SetRequestedWidth(100);
      label.SetRequestedHeight(120);
      auto s = MakeTextFixture(r, label);
      r.SetState(s);
      ArmLatin(r, s, true, 1, 1, 24);
      r.Attach(label);
    }},
                                                                                                                {"two-lines", 19, [](Run& r)
    {
      auto s = r.State<TextFixture>();
      ArmLatin(r, s, false, 2, 3, 24);
      s->label.SetText("A\nA");
    }},
                                                                                                                {"font-size", 19, [](Run& r)
    {
      auto s = r.State<TextFixture>();
      ArmLatin(r, s, false, 2, 3, 36);
      s->label.SetFontSize(36);
    }},
                                                                                                                {"return-A", 19, [](Run& r)
    {
      auto s = r.State<TextFixture>();
      ArmLatin(r, s, false, 1, 1, 24);
      s->label.SetText("A");
      s->label.SetFontSize(24);
    }},
                                                                                                                {"align-center", 19, [](Run& r)
    {
      auto s = r.State<TextFixture>();
      ArmLatin(r, s, false, 1, 1, 24, 0.5f);
      s->label.SetVerticalTextAlignment(Text::Alignment::CENTER);
    }},
                                                                                                                {"align-end", 19, [](Run& r)
    {
      auto s = r.State<TextFixture>();
      ArmLatin(r, s, false, 1, 1, 24, 1.0f);
      s->label.SetVerticalTextAlignment(Text::Alignment::END);
    }}}});
    result.push_back(Steps("text-intrinsic", "Natural metrics, constrained wrapping and intrinsic invalidation rendered on screen", {
      {"natural", 6, [](Run& r) {
         auto s    = std::make_shared<IntrinsicText>();
         s->label  = Label::New("A");
         s->label.SetFontFamily("DejaVu Sans");
         s->label.SetFontSize(24);
         s->label.SetMultiLine(true);
         s->label.SetRequestedWidth(WRAP_CONTENT);
         s->label.SetRequestedHeight(WRAP_CONTENT);
         s->stage = Mount(r, s->label, 300, 300);
         r.SetState(s);
         r.AfterRender({s->stage, s->label}, [=](Run& r) {
           const auto a = s->label.GetMeasuredSize();
           r.Near("intrinsic.A.width", a.width, 1401.0 / 2048.0 * 24, 1);
           r.Near("intrinsic.A.height", a.height, 29, 1);
           r.Rendered("intrinsic.A.rendered", s->label, LayoutRect(0, 0, 1401.0f / 2048.0f * 24, 29), 1);
         });
       }},
      {"narrow", 6, [](Run& r) {
         auto s = r.State<IntrinsicText>();
         s->label.SetText("AAAAAA");
         s->label.SetLineWrapMode(Text::LineWrapMode::CHARACTER);
         s->label.SetRequestedWidth(MATCH_PARENT);
         s->stage.SetRequestedWidth(25);
         r.AfterRender({s->stage, s->label}, [=](Run& r) {
           const auto narrow = s->label.GetMeasuredSize();
           r.Near("intrinsic.match.slot", narrow.width, 25);
           r.Near("intrinsic.six.lines", narrow.height, 174, 6);
           r.Rendered("intrinsic.six.rendered", s->label, LayoutRect(0, 0, 25, 174), 6);
         });
       }},
      {"wide", 6, [](Run& r) {
         auto s = r.State<IntrinsicText>();
         s->stage.SetRequestedWidth(40);
         r.AfterRender({s->stage, s->label}, [=](Run& r) {
           const auto wide = s->label.GetMeasuredSize();
           r.Near("intrinsic.match.wide.slot", wide.width, 40);
           r.Near("intrinsic.three.lines", wide.height, 87, 3);
           r.Rendered("intrinsic.three.rendered", s->label, LayoutRect(0, 0, 40, 87), 3);
         });
       }},
      {"fixed-width", 6, [](Run& r) {
         auto s = r.State<IntrinsicText>();
         s->label.SetRequestedWidth(25);
         s->stage.SetRequestedWidth(300);
         r.AfterRender({s->stage, s->label}, [=](Run& r) {
           const auto fixed = s->label.GetMeasuredSize();
           r.Near("intrinsic.fixed.width", fixed.width, 25);
           r.Near("intrinsic.fixed.height", fixed.height, 174, 6);
           r.Rendered("intrinsic.fixed.rendered", s->label, LayoutRect(0, 0, 25, 174), 6);
         });
       }},
      {"changed-text", 6, [](Run& r) {
         auto s = r.State<IntrinsicText>();
         s->label.SetText("A");
         r.AfterRender({s->stage, s->label}, [=](Run& r) {
           const auto changed = s->label.GetMeasuredSize();
           r.Near("intrinsic.changed.width", changed.width, 25);
           r.Near("intrinsic.changed.height", changed.height, 29, 1);
           r.Rendered("intrinsic.changed.rendered", s->label, LayoutRect(0, 0, 25, 29), 1);
         });
       }},
      {"font-size", 6, [](Run& r) {
         auto s = r.State<IntrinsicText>();
         s->label.SetRequestedWidth(WRAP_CONTENT);
         s->label.SetFontSize(36);
         r.AfterRender({s->stage, s->label}, [=](Run& r) {
           const auto large = s->label.GetMeasuredSize();
           // Width: glyph advance plus bearing and integer rounding grow with the size; ±2 at 36px.
           r.Near("intrinsic.font.width", large.width, 1401.0 / 2048.0 * 36, 2);
           r.Near("intrinsic.font.height", large.height, 43, 1);
           r.Rendered("intrinsic.font.rendered", s->label, LayoutRect(0, 0, 1401.0f / 2048.0f * 36, 43), 2);
         });
       }},
      {"empty", 6, [](Run& r) {
         auto s = r.State<IntrinsicText>();
         s->label.SetText("");
         r.AfterRender({s->stage, s->label}, [=](Run& r) {
           r.Size("intrinsic.empty", s->label.GetMeasuredSize(), MeasuredSize(0, 0));
           r.Rendered("intrinsic.empty.rendered", s->label, LayoutRect(0, 0, 0, 0));
         });
       }},
    }));
    result.push_back(Single("fixed-korean-font", "Pinned Korean cmap and advances after core relayout", 10, [](Run& r)
    {
      auto label = Label::New("가");
      label.SetFontFamily("NanumGothic");
      label.SetFontSize(24);
      label.SetRequestedWidth(100);
      label.SetRequestedHeight(60);
      auto s = MakeTextFixture(r, label);
      r.SetState(s);
      ArmText(r, s, true, [](Run& done, const Diagnostic::TextSnapshot& t, const LayoutRect& target)
      {
        done.Truth("korean.ready", t.valid && t.ready);
        done.Equal("korean.glyphs", t.glyphCount, 1);
        done.Equal("korean.glyph.index", t.firstGlyphIndex, 1086);
        Dali::TextAbstraction::FontDescription description;
        Dali::TextAbstraction::FontClient::Get().GetDescription(t.firstGlyphFontId, description);
        done.Text("korean.font.path", std::filesystem::weakly_canonical(description.path).string(), std::filesystem::weakly_canonical(TEST_RESOURCE_DIR "/layout-validation/NanumGothic.ttf").string());
        done.Near("korean.advance", t.firstGlyphAdvance, 940.0 / 1000.0 * 24, 1);
        done.Rect("korean.target", target, LayoutRect(0, 0, 100, 60));
      });
      r.Attach(label);
    }));
    for(const auto& dimensions : {std::pair<int, int>{37, 19}, {73, 41}})
    {
      const std::string path = std::string(TEST_RESOURCE_DIR) + "/layout-validation/image-" + std::to_string(dimensions.first) + "x" + std::to_string(dimensions.second) + ".png";
      const float       w = dimensions.first, h = dimensions.second;
      result.push_back(Steps("image-" + std::to_string(dimensions.first), "Natural dimensions and aspect-preserving one-axis requests rendered on screen", {
        {"wrap", 8, [path, w, h](Run& r) {
           auto s   = std::make_shared<SyncImage>();
           s->image = ImageView::New();
           s->image.SetSynchronousLoading(true);
           s->image.SetLoadPolicy(Image::LoadPolicy::IMMEDIATE);
           s->image.SetResourceUrl(Dali::String(path.c_str()));
           s->image.SetRequestedWidth(WRAP_CONTENT);
           s->image.SetRequestedHeight(WRAP_CONTENT);
           const auto natural = s->image.GetNaturalSize();
           r.Near("image.natural.width", natural.x, w);
           r.Near("image.natural.height", natural.y, h);
           s->stage = Mount(r, s->image, 300, 300);
           r.SetState(s);
           r.AfterRender({s->stage, s->image}, [=](Run& r) {
             r.Size("image.wrap", s->image.GetMeasuredSize(), MeasuredSize(w, h));
             r.Rendered("image.wrap.rendered", s->image, LayoutRect(0, 0, w, h));
           });
         }},
        {"fixed-width", 6, [w, h](Run& r) {
           auto s = r.State<SyncImage>();
           s->image.SetRequestedWidth(w * 2);
           r.AfterRender({s->stage, s->image}, [=](Run& r) {
             r.Size("image.fixed.width", s->image.GetMeasuredSize(), MeasuredSize(w * 2, h * 2));
             r.Rendered("image.fixed.width.rendered", s->image, LayoutRect(0, 0, w * 2, h * 2));
           });
         }},
        {"fixed-height", 6, [w, h](Run& r) {
           auto s = r.State<SyncImage>();
           s->image.SetRequestedWidth(WRAP_CONTENT);
           s->image.SetRequestedHeight(h * 3);
           r.AfterRender({s->stage, s->image}, [=](Run& r) {
             r.Size("image.fixed.height", s->image.GetMeasuredSize(), MeasuredSize(w * 3, h * 3));
             r.Rendered("image.fixed.height.rendered", s->image, LayoutRect(0, 0, w * 3, h * 3));
           });
         }},
        {"both-fixed", 7, [](Run& r) {
           auto s = r.State<SyncImage>();
           s->image.SetRequestedWidth(80);
           s->image.SetRequestedHeight(30);
           r.AfterRender({s->stage, s->image}, [=](Run& r) {
             r.Size("image.both.fixed", s->image.GetMeasuredSize(), MeasuredSize(80, 30));
             r.Rendered("image.both.fixed.rendered", s->image, LayoutRect(0, 0, 80, 30));
             r.Equal("image.status", static_cast<int>(s->image.GetLoadingStatus()), static_cast<int>(Visual::ResourceStatus::READY));
           });
         }},
      }));
    }
    result.push_back({"image-async", "Observe this request in either completion order and compare fresh current geometry", {{"load-37", 12, [](Run& r)
    {
      auto state = MakeAsyncImage(r);
      r.SetState(state);
      ChangeImage(r, state, 37, 19, true);
    }},
                                                                                                                            {"replace-73", 12, [](Run& r)
    { ChangeImage(r, r.State<AsyncImage>(), 73, 41, false); }},
                                                                                                                            {"return-37", 12, [](Run& r)
    { ChangeImage(r, r.State<AsyncImage>(), 37, 19, false); }}}});
    result.push_back(Steps("image-failure-recovery", "A failed resource cannot corrupt the fixed rendered slot; a valid replacement recovers the intrinsic size", {
      {"invalid", 7, [](Run& r) {
         auto s   = std::make_shared<SyncImage>();
         s->image = ImageView::New();
         s->image.SetSynchronousLoading(true);
         s->image.SetLoadPolicy(Image::LoadPolicy::IMMEDIATE);
         s->image.SetRequestedWidth(80);
         s->image.SetRequestedHeight(30);
         s->image.SetMargin(Insets(3, 0, 5, 0));
         s->image.SetResourceUrl(TEST_RESOURCE_DIR "/layout-validation/invalid-image.bin");
         s->stage = Mount(r, s->image, 300, 300);
         r.SetState(s);
         r.AfterRender({s->stage, s->image}, [=](Run& r) {
           r.Size("invalid.fixed", s->image.GetMeasuredSize(), MeasuredSize(80, 30));
           r.Equal("invalid.status", static_cast<int>(s->image.GetLoadingStatus()), static_cast<int>(Visual::ResourceStatus::FAILED));
           r.Rendered("invalid.slot", s->image, LayoutRect(3, 5, 80, 30));
         });
       }},
      {"recovered", 6, [](Run& r) {
         auto s = r.State<SyncImage>();
         s->image.SetResourceUrl(TEST_RESOURCE_DIR "/layout-validation/image-73x41.png");
         s->image.SetRequestedWidth(WRAP_CONTENT);
         s->image.SetRequestedHeight(WRAP_CONTENT);
         r.AfterRender({s->stage, s->image}, [=](Run& r) {
           r.Size("recovered.intrinsic", s->image.GetMeasuredSize(), MeasuredSize(73, 41));
           r.Rendered("recovered.rendered", s->image, LayoutRect(3, 5, 73, 41));
         });
       }},
    }));
    // The wrapping stack takes its width from the seed, then measures the MATCH_PARENT Label
    // exactly at that width, so the Label's height follows the laid-out width (6 A's: 2 per line
    // at 40 -> 3 lines, 4 per line at 75 -> 2 lines; 1 line at the 300 budget).
    result.push_back(Steps("text-two-step", "A MATCH_PARENT Label in a wrapping stack is measured at the width the stack resolves", {
      {"seed-40", 14, [](Run& r) {
         auto s   = std::make_shared<LaidOutText>();
         s->stack = StackLayout::New(StackOrientation::VERTICAL);
         s->seed  = Leaf(40, 0, "seed");
         s->label = WrappingLabel("AAAAAA");
         s->label.SetRequestedWidth(MATCH_PARENT);
         s->label.SetRequestedHeight(WRAP_CONTENT);
         s->stack.Add(s->seed);
         s->stack.Add(s->label);
         s->stage = Mount(r, s->stack, 300, 300);
         r.SetState(s);
         r.AfterRender({s->stage, s->stack, s->seed, s->label}, [=](Run& r) {
           r.Size("two-step.stack", s->stack.GetMeasuredSize(), MeasuredSize(40, 87), 3);
           r.Rendered("two-step.stack.rendered", s->stack, LayoutRect(0, 0, 40, 87), 3);
           r.Size("two-step.label", s->label.GetMeasuredSize(), MeasuredSize(40, 87), 3);
           r.Rendered("two-step.label.rendered", s->label, LayoutRect(0, 0, 40, 87), 3);
           r.Near("two-step.label.height-at-width", s->label.GetMeasuredSize().height, s->label.GetHeightForWidth(40));
           r.Truth("two-step.precondition", s->label.GetHeightForWidth(40) > s->label.GetHeightForWidth(300));
         });
       }},
      {"seed-75", 14, [](Run& r) {
         auto s = r.State<LaidOutText>();
         s->seed.SetRequestedWidth(75);
         r.AfterRender({s->stage, s->stack, s->seed, s->label}, [=](Run& r) {
           r.Size("two-step.stack", s->stack.GetMeasuredSize(), MeasuredSize(75, 58), 2);
           r.Rendered("two-step.stack.rendered", s->stack, LayoutRect(0, 0, 75, 58), 2);
           r.Size("two-step.label", s->label.GetMeasuredSize(), MeasuredSize(75, 58), 2);
           r.Rendered("two-step.label.rendered", s->label, LayoutRect(0, 0, 75, 58), 2);
           r.Near("two-step.label.height-at-width", s->label.GetMeasuredSize().height, s->label.GetHeightForWidth(75));
           r.Truth("two-step.precondition", s->label.GetHeightForWidth(75) > s->label.GetHeightForWidth(300));
         });
       }},
    }));
    // A FILL Label in a stack of determined width is measured EXACTLY at the stack's width (its
    // height at that width), clamped by its own minimum/maximum, and laid out at that clamped
    // width from the start edge of its slot. With a minimum above the maximum the maximum wins, and
    // the Label's height follows that assigned width (25: one 'A' per line), not the width its own
    // minimum-first clamp would pick (100: fewer lines, two on the reference machine).
    result.push_back(Steps("text-stretched", "A stretched Label reports the assigned width clamped by its own limits and its height at that width", {
      {"fill", 6, [](Run& r) {
         auto s   = std::make_shared<LaidOutText>();
         s->stack = StackLayout::New(StackOrientation::VERTICAL);
         Fixed(s->stack, 300, 200);
         s->label = WrappingLabel("A");
         s->label.SetRequestedWidth(WRAP_CONTENT);
         s->label.SetRequestedHeight(WRAP_CONTENT);
         s->label.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
         s->stack.Add(s->label);
         s->stage = Mount(r, s->stack, 300, 300);
         r.SetState(s);
         r.AfterRender({s->stage, s->stack, s->label}, [=](Run& r) {
           r.Near("stretched.measured.width", s->label.GetMeasuredSize().width, 300);
           r.Near("stretched.measured.height", s->label.GetMeasuredSize().height, 29, 1);
           r.Rendered("stretched.rendered", s->label, LayoutRect(0, 0, 300, 29), 1);
         });
       }},
      {"maximum", 6, [](Run& r) {
         auto s = r.State<LaidOutText>();
         s->label.SetText("AAAAAA");
         s->label.SetMaximumWidth(25);
         r.AfterRender({s->stage, s->stack, s->label}, [=](Run& r) {
           r.Near("stretched.maximum.width", s->label.GetMeasuredSize().width, 25);
           r.Near("stretched.maximum.height", s->label.GetMeasuredSize().height, 174, 6);
           r.Rendered("stretched.maximum.rendered", s->label, LayoutRect(0, 0, 25, 174), 6);
         });
       }},
      {"minimum", 6, [](Run& r) {
         auto s = r.State<LaidOutText>();
         s->label.SetMaximumWidth(std::numeric_limits<float>::max());
         s->label.SetMinimumWidth(300);
         s->stack.SetRequestedWidth(40);
         r.AfterRender({s->stage, s->stack, s->label}, [=](Run& r) {
           r.Near("stretched.minimum.width", s->label.GetMeasuredSize().width, 300);
           r.Near("stretched.minimum.height", s->label.GetMeasuredSize().height, 29, 1);
           r.Rendered("stretched.minimum.rendered", s->label, LayoutRect(0, 0, 300, 29), 1);
         });
       }},
      {"conflict", 8, [](Run& r) {
         auto s = r.State<LaidOutText>();
         s->label.SetMinimumWidth(100);
         s->label.SetMaximumWidth(25);
         s->stack.SetRequestedWidth(300);
         r.AfterRender({s->stage, s->stack, s->label}, [=](Run& r) {
           r.Near("stretched.conflict.width", s->label.GetMeasuredSize().width, 25);
           r.Near("stretched.conflict.height", s->label.GetMeasuredSize().height, 174, 6);
           r.Near("stretched.conflict.height-at-width", s->label.GetMeasuredSize().height, s->label.GetHeightForWidth(25));
           r.Rendered("stretched.conflict.rendered", s->label, LayoutRect(0, 0, 25, 174), 6);
           r.Truth("stretched.conflict.precondition", s->label.GetHeightForWidth(25) > s->label.GetHeightForWidth(100));
         });
       }},
    }));
    return result;
  }
};
} //namespace
REGISTER_MANUAL_TEST(Tc143)
