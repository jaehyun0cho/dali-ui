/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-fixtures.h"

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

class TcLr15 : public Case
{
public:
  TcLr15()
  : Case("LR15", "Flex baseline")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    // DejaVu Sans design metrics (res/layout-validation/font-metrics.json): ascender 1901 / 2048 em.
    constexpr float ASCENDER_PER_EM = 1901.0f / 2048.0f;
    for(bool overrideSelf : {false, true})
      out.push_back(Single(overrideSelf ? "F14.align-self" : "F14.align-items", "Rendered baseline equality of unequal font sizes", 19, [overrideSelf](Run& r) {
        auto f = FlexLayout::New();
        Fixed(f, 320, 100);
        f.SetAlignItems(overrideSelf ? FlexAlign::FLEX_START : FlexAlign::BASELINE);
        Label a = Label::New(), b = Label::New();
        a.SetText("Hx");
        b.SetText("Hx");
        a.SetFontFamily("DejaVu Sans");
        b.SetFontFamily("DejaVu Sans");
        a.SetFontSize(18);
        b.SetFontSize(36);
        a.SetMultiLine(false);
        b.SetMultiLine(false);
        a.SetRequestedWidth(WRAP_CONTENT);
        a.SetRequestedHeight(WRAP_CONTENT);
        b.SetRequestedWidth(WRAP_CONTENT);
        b.SetRequestedHeight(WRAP_CONTENT);
        if(overrideSelf)
        {
          a.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::BASELINE));
          b.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::BASELINE));
        }
        f.Add(a);
        f.Add(b);
        View stage = Mount(r, f, 320, 100);
        r.AfterRender({stage, f, a, b}, [=](Run& done) {
          auto ta = Diagnostic::GetTextSnapshot(a), tb = Diagnostic::GetTextSnapshot(b);
          done.Truth("observer-valid", ta.valid && tb.valid);
          done.Truth("text-ready", ta.ready && tb.ready);
          done.Truth("first-line", ta.firstLineAvailable && tb.firstLineAvailable);
          done.Equal("a-lines", ta.lineCount, 1);
          done.Equal("b-lines", tb.lineCount, 1);
          done.Truth("glyphs", ta.glyphCount >= 2 && tb.glyphCount >= 2);
          done.Truth("font-resolved", ta.firstGlyphFontId != 0 && tb.firstGlyphFontId != 0);
          done.Truth("same-glyph", ta.firstGlyphIndex == tb.firstGlyphIndex && ta.firstGlyphIndex != 0);
          done.Truth("larger-advance", tb.firstGlyphAdvance > ta.firstGlyphAdvance);
          done.Near("a.line-spacing", ta.firstLineSpacing, 0, 0);
          done.Near("b.line-spacing", tb.firstLineSpacing, 0, 0);
          done.Near("a.ascender", ta.firstAscender, ASCENDER_PER_EM * 18.0f, 1.0);
          done.Near("b.ascender", tb.firstAscender, ASCENDER_PER_EM * 36.0f, 1.0);
          done.Near("a.local-baseline", ta.firstBaseline, ta.renderedOffsetY + ta.firstAscender, 0.01);
          done.Near("b.local-baseline", tb.firstBaseline, tb.renderedOffsetY + tb.firstAscender, 0.01);
          done.Truth("different-local-baselines", std::fabs(tb.firstBaseline - ta.firstBaseline) > 1);
          const LayoutRect ra = Run::RenderedBounds(a), rb = Run::RenderedBounds(b);
          done.Near("shared-rendered-baseline", ra.y + ta.firstBaseline, rb.y + tb.firstBaseline, 0.01);
          done.Near("first-x", ra.x, 0);
          done.Near("second-x", rb.x, ra.width);
        });
      }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr15)
