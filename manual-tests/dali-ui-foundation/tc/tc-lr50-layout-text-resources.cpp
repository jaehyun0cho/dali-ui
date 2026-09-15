/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "layout-validation-support.h"

#include <dali-ui-foundation/integration-api/layout-test-diagnostics.h>
#include <dali-ui-foundation/public-api/views/image/image-view.h>
#include <dali/devel-api/actors/actor-devel.h>
#include <dali/devel-api/text-abstraction/font-client.h>
#include <cmath>
#include <filesystem>

using namespace LayoutValidation;
namespace
{
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
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
void CheckText(Run& r, const Diagnostic::TextSnapshot& t, const LayoutRect& target, uint32_t lines, uint32_t glyphs, float fontSize)
{
  r.Truth("text.model.valid", t.valid);
  r.Truth("text.model.ready", t.ready);
  r.Equal("text.lines", t.lineCount, lines);
  r.Equal("text.glyphs", t.glyphCount, glyphs);
  r.Rect("text.target", target, LayoutRect(0, 0, 100, 120));
  r.Near("text.control.width", t.controlWidth, 100);
  r.Near("text.control.height", t.controlHeight, 120);
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
void ArmLatin(Run& r, const std::shared_ptr<TextFixture>& s, bool first, uint32_t lines, uint32_t glyphs, float size)
{
  ArmText(r, s, first, [lines, glyphs, size](Run& done, const Diagnostic::TextSnapshot& model, const LayoutRect& target)
  { CheckText(done, model, target, lines, glyphs, size); });
}
#endif

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
    done.Rect("image.current.target", s->image, LayoutRect(0, 0, s->width, s->height));
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
class TcLr50 : public Case
{
public:
  TcLr50()
  : Case("LR50", "Text and image resources")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> result;
    result.push_back({"text-model", "Locked font and fresh core relayout model after fixed-size text changes", {{"single-A", 18, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
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
#endif
    }},
                                                                                                                {"two-lines", 18, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto s = r.State<TextFixture>();
      ArmLatin(r, s, false, 2, 3, 24);
      s->label.SetText("A\nA");
#endif
    }},
                                                                                                                {"font-size", 18, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto s = r.State<TextFixture>();
      ArmLatin(r, s, false, 2, 3, 36);
      s->label.SetFontSize(36);
#endif
    }},
                                                                                                                {"return-A", 18, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
      auto s = r.State<TextFixture>();
      ArmLatin(r, s, false, 1, 1, 24);
      s->label.SetText("A");
      s->label.SetFontSize(24);
#endif
    }}}});
    result.push_back(Single("text-intrinsic", "Natural metrics, constrained wrapping and intrinsic invalidation", 14, [](Run& r)
    {
      auto label = Label::New("A");
      label.SetFontFamily("DejaVu Sans");
      label.SetFontSize(24);
      label.SetMultiLine(true);
      label.SetRequestedWidth(WRAP_CONTENT);
      label.SetRequestedHeight(WRAP_CONTENT);
      auto a = label.Measure(500, 500);
      r.Near("intrinsic.A.width", a.width, 1401.0 / 2048.0 * 24, 1);
      r.Near("intrinsic.A.height", a.height, 29, 1);
      label.SetText("AAAAAA");
      label.SetLineWrapMode(Text::LineWrapMode::CHARACTER);
      label.SetRequestedWidth(MATCH_PARENT);
      auto narrow = label.Measure(25, 500);
      r.Near("intrinsic.match.minimum", narrow.width, 0);
      r.Near("intrinsic.six.lines", narrow.height, 174, 6);
      auto wide = label.Measure(40, 500);
      r.Near("intrinsic.match.wide.minimum", wide.width, 0);
      r.Near("intrinsic.three.lines", wide.height, 87, 3);
      label.SetRequestedWidth(25);
      auto fixed = label.Measure(500, 500);
      r.Near("intrinsic.fixed.width", fixed.width, 25);
      r.Near("intrinsic.fixed.height", fixed.height, 174, 6);
      label.SetText("A");
      auto changed = label.Measure(500, 500);
      r.Near("intrinsic.changed.width", changed.width, 25);
      r.Near("intrinsic.changed.height", changed.height, 29, 1);
      label.SetRequestedWidth(WRAP_CONTENT);
      label.SetFontSize(36);
      auto large = label.Measure(500, 500);
      r.Near("intrinsic.font.width", large.width, 1401.0 / 2048.0 * 36, 1);
      r.Near("intrinsic.font.height", large.height, 43, 1);
      label.SetText("");
      auto empty = label.Measure(500, 500);
      r.Size("intrinsic.empty", empty, MeasuredSize(0, 0));
    }));
    result.push_back(Single("fixed-korean-font", "Pinned Korean cmap and advances after core relayout", 10, [](Run& r)
    {
      if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
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
#endif
    }));
    for(const auto& dimensions : {std::pair<int, int>{37, 19}, {73, 41}})
    {
      const std::string path = std::string(TEST_RESOURCE_DIR) + "/layout-validation/image-" + std::to_string(dimensions.first) + "x" + std::to_string(dimensions.second) + ".png";
      result.push_back(Single(("image-" + std::to_string(dimensions.first)).c_str(), "Natural dimensions and aspect-preserving one-axis requests", 11, [path, dimensions](Run& r)
      {
        auto image = ImageView::New();
        image.SetSynchronousLoading(true);
        image.SetLoadPolicy(Image::LoadPolicy::IMMEDIATE);
        image.SetResourceUrl(Dali::String(path.c_str()));
        image.SetRequestedWidth(WRAP_CONTENT);
        image.SetRequestedHeight(WRAP_CONTENT);
        const auto natural = image.GetNaturalSize();
        r.Near("image.natural.width", natural.x, dimensions.first);
        r.Near("image.natural.height", natural.y, dimensions.second);
        r.Size("image.wrap", image.Measure(500, 500), MeasuredSize(dimensions.first, dimensions.second));
        image.SetRequestedWidth(dimensions.first * 2);
        r.Size("image.fixed.width", image.Measure(500, 500), MeasuredSize(dimensions.first * 2, dimensions.second * 2));
        image.SetRequestedWidth(WRAP_CONTENT);
        image.SetRequestedHeight(dimensions.second * 3);
        r.Size("image.fixed.height", image.Measure(500, 500), MeasuredSize(dimensions.first * 3, dimensions.second * 3));
        image.SetRequestedWidth(80);
        image.SetRequestedHeight(30);
        r.Size("image.both.fixed", image.Measure(500, 500), MeasuredSize(80, 30));
        r.Equal("image.status", static_cast<int>(image.GetLoadingStatus()), static_cast<int>(Visual::ResourceStatus::READY));
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
    result.push_back(Single("image-failure-recovery", "Failed resource cannot corrupt fixed layout; valid replacement recovers intrinsic size", 9, [](Run& r)
    {
      auto image = ImageView::New();
      image.SetSynchronousLoading(true);
      image.SetLoadPolicy(Image::LoadPolicy::IMMEDIATE);
      image.SetRequestedWidth(80);
      image.SetRequestedHeight(30);
      image.SetResourceUrl(TEST_RESOURCE_DIR "/layout-validation/invalid-image.bin");
      r.Size("invalid.fixed", image.Measure(500, 500), MeasuredSize(80, 30));
      r.Equal("invalid.status", static_cast<int>(image.GetLoadingStatus()), static_cast<int>(Visual::ResourceStatus::FAILED));
      r.Rect("invalid.slot", image.Arrange(LayoutRect(3, 5, 80, 30)), LayoutRect(3, 5, 80, 30));
      image.SetResourceUrl(TEST_RESOURCE_DIR "/layout-validation/image-73x41.png");
      image.SetRequestedWidth(WRAP_CONTENT);
      image.SetRequestedHeight(WRAP_CONTENT);
      r.Size("recovered.intrinsic", image.Measure(500, 500), MeasuredSize(73, 41));
    }));
    return result;
  }
};
} //namespace
REGISTER_MANUAL_TEST(TcLr50)
