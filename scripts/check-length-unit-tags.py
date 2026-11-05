#!/usr/bin/env python3
#
# Copyright (c) 2026 Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Check the length unit tags of the dali-ui API documentation.

Every documented pixel length in the API headers, outside the generated *.autogen.h headers,
must carry exactly one @UNIT_LOGICAL or @UNIT_PHYSICAL tag, written as the first token of the
description it qualifies. See rules/length-units.md and docs/length-units.md.

The script finds candidates by C++ type and name, so a hit can be a false positive: add it to
ALLOWLIST below with a reason instead of adding a tag.

With --all the script also checks that each file of DEFINITION_FILES contains the two tag
definitions of DEFINITIONS word for word, in a header each right after its tag token.

Usage:
  python3 scripts/check-length-unit-tags.py --all
  python3 scripts/check-length-unit-tags.py --diff origin/devel
  python3 scripts/check-length-unit-tags.py --self-test

Exit codes: 0 no violation, 1 at least one violation, 2 usage error, git failure or unreadable file.
"""

import argparse
import bisect
import re
import subprocess
import sys
from pathlib import Path

# The src-dirs of the mkdoxy project in mkdocs.yml. Keep this list in sync with mkdocs.yml.
API_DIRS = (
    "dali-ui-foundation/public-api",
    "dali-ui-foundation/extension-api",
    "dali-ui-foundation/integration-api",
    "dali-ui-components/public-api",
    "dali-ui-components/integration-api",
)

# Items that carry no tag: false positives of the type and name search, and lengths deliberately left untagged for
# now (the reason says why). Entries are (header path relative to the repository root, item key exactly as reported,
# reason); a key covers every overload that has the same parameter, return value or member name.
# fmt: off
ALLOWLIST = [
    ("dali-ui-components/integration-api/chart/bar-series-impl.h", "Integration::BarSeries::GetBarGroupWidth -> return",
     "Ratio of the available space, not a length"),
    ("dali-ui-components/integration-api/chart/bar-series-impl.h", "Integration::BarSeries::mFillThreshold",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-annotation-impl.h", "Integration::ChartAnnotation::mPosition",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-axis-impl.h", "Integration::ChartAxis::GetDataPadding -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-axis-impl.h", "Integration::ChartAxis::GetMinStep -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-axis-impl.h", "Integration::ChartAxis::SetMinStep(step)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-bar-geometry.h", "Integration::ChartBarGeometry::dataX",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-bar-geometry.h", "Integration::ChartBarGeometry::dataY",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-color-palette.h", "Integration::ChartColorPalette::DEFAULT_SIZE",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-hit-tester.h", "Integration::HitResult::dataX",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-hit-tester.h", "Integration::HitResult::dataY",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-model.h", "Integration::StyleConfig::LayoutStyle::legendPosition",
     "Enum position (ChartView::LegendPosition), not a length"),
    ("dali-ui-components/integration-api/chart/chart-model.h", "Integration::StyleConfig::LayoutStyle::titlePosition",
     "Enum position (ChartView::TitlePosition), not a length"),
    ("dali-ui-components/integration-api/chart/chart-scale-engine.h", "Integration::ScaleEngine::ComputeNiceTicks(minStep)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-scale-engine.h", "Integration::ScaleEngine::IsValid(x)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-scale-engine.h", "Integration::ScaleEngine::IsValid(y)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-scale-engine.h", "Integration::ScaleEngine::ToCanvas(dataX)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-scale-engine.h", "Integration::ScaleEngine::ToCanvas(dataY)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-scale-engine.h", "Integration::ScaleEngine::ToCanvasX(dataX)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-scale-engine.h", "Integration::ScaleEngine::ToCanvasY(dataY)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-scale-engine.h", "Integration::ScaleEngine::ToDataX -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-scale-engine.h", "Integration::ScaleEngine::ToDataY -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-series-impl.h", "Integration::ChartSeries::AppendValue(x)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-series-impl.h", "Integration::ChartSeries::AppendValue(y)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/integration-api/chart/chart-view-impl.h", "Integration::ChartViewImpl::GetGaugeArcWidth -> return",
     "Ratio of the outer radius, not a length"),
    ("dali-ui-components/integration-api/chart/chart-view-impl.h", "Integration::ChartViewImpl::SetViewport(range)",
     "Visible data range of the chart (minimum and maximum x and y in data coordinates), not a pixel length"),
    ("dali-ui-components/integration-api/chart/pie-series-impl.h", "Integration::PieSeries::GetSliceGap -> return",
     "Angle in degrees, not a length"),
    ("dali-ui-components/integration-api/dialog/dialog-impl.h", "Integration::DialogImpl::GetSpacing -> return",
     "dialog.h and dialog-impl.h are not clang-format clean at the base; tagging them would pull a code reformat into this comment-only commit"),
    ("dali-ui-components/integration-api/dialog/dialog-impl.h", "Integration::DialogImpl::SetSpacing(spacing)",
     "dialog.h and dialog-impl.h are not clang-format clean at the base; tagging them would pull a code reformat into this comment-only commit"),
    ("dali-ui-components/public-api/chart/bar-series.h", "BarSeries::GetBarGroupWidth -> return",
     "Ratio of the available space, not a length"),
    ("dali-ui-components/public-api/chart/bar-series.h", "BarSeries::SetFillThreshold(value)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-annotation.h", "ChartAnnotation::SetPosition(position)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-axis.h", "ChartAxis::GetDataPadding -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-axis.h", "ChartAxis::GetMinimumStep -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-axis.h", "ChartAxis::SetMinimumStep(step)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-section.h", "ChartSection::GetMaximumX -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-section.h", "ChartSection::GetMaximumY -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-section.h", "ChartSection::GetMinimumX -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-section.h", "ChartSection::GetMinimumY -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-section.h", "ChartSection::SetMaximumX(value)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-section.h", "ChartSection::SetMaximumY(value)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-section.h", "ChartSection::SetMinimumX(value)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-section.h", "ChartSection::SetMinimumY(value)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-series.h", "ChartSeries::AppendValue(x)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-series.h", "ChartSeries::AppendValue(y)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-view-properties.h", "ChartViewPropertyIndex::AXIS_LABEL_SIZE",
     "Undocumented property index; ChartView::Property::AXIS_LABEL_SIZE documents the value (decision A-Q6)"),
    ("dali-ui-components/public-api/chart/chart-view-properties.h", "ChartViewPropertyIndex::LEGEND_POSITION",
     "Enum position (ChartView::LegendPosition), not a length"),
    ("dali-ui-components/public-api/chart/chart-view-properties.h", "ChartViewPropertyIndex::LINE_WIDTH",
     "Undocumented property index; ChartView::Property::LINE_WIDTH documents the value (decision A-Q6)"),
    ("dali-ui-components/public-api/chart/chart-view-properties.h", "ChartViewPropertyIndex::MARKER_RADIUS",
     "Undocumented property index; ChartView::Property::MARKER_RADIUS documents the value (decision A-Q6)"),
    ("dali-ui-components/public-api/chart/chart-view-properties.h", "ChartViewPropertyIndex::TITLE_SIZE",
     "Undocumented property index; ChartView::Property::TITLE_SIZE documents the value (decision A-Q6)"),
    ("dali-ui-components/public-api/chart/chart-view-properties.h", "ChartViewPropertyIndex::Y_AXIS_AUTO_RANGE",
     "Boolean flag (yAxisAutoRange, BOOLEAN in chart-view-impl.cpp:85), not a length"),
    ("dali-ui-components/public-api/chart/chart-view.h", "ChartPointEventArgs::ChartPointEventArgs(dataX)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-view.h", "ChartPointEventArgs::ChartPointEventArgs(dataY)",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-view.h", "ChartPointEventArgs::GetDataX -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-view.h", "ChartPointEventArgs::GetDataY -> return",
     "Chart data coordinate, data range, data step or count, not a pixel length"),
    ("dali-ui-components/public-api/chart/chart-view.h", "ChartView::GetGaugeArcWidth -> return",
     "Ratio of the outer radius, not a length"),
    ("dali-ui-components/public-api/chart/chart-view.h", "ChartView::Property::LEGEND_POSITION",
     "Enum position (ChartView::LegendPosition), not a length"),
    ("dali-ui-components/public-api/chart/chart-view.h", "ChartView::Property::Y_AXIS_AUTO_RANGE",
     "Boolean flag (yAxisAutoRange, BOOLEAN in chart-view-impl.cpp:85), not a length"),
    ("dali-ui-components/public-api/chart/chart-view.h", "ChartView::SetViewport(range)",
     "Visible data range of the chart (minimum and maximum x and y in data coordinates), not a pixel length"),
    ("dali-ui-components/public-api/chart/pie-series.h", "PieSeries::GetSliceGap -> return",
     "Angle in degrees, not a length"),
    ("dali-ui-components/public-api/dialog/dialog.h", "Dialog::GetSpacing -> return",
     "dialog.h and dialog-impl.h are not clang-format clean at the base; tagging them would pull a code reformat into this comment-only commit"),
    ("dali-ui-components/public-api/dialog/dialog.h", "Dialog::SetSpacing(spacing)",
     "dialog.h and dialog-impl.h are not clang-format clean at the base; tagging them would pull a code reformat into this comment-only commit"),
    ("dali-ui-components/public-api/styles/dialog-style.h", "DialogStyle::Builder::SetBorderlineOffset(value)",
     "Fraction of the borderline width in [-1, 1], not a length"),
    ("dali-ui-components/public-api/styles/dialog-style.h", "DialogStyle::GetBorderlineOffset -> return",
     "Fraction of the borderline width in [-1, 1], not a length"),
    ("dali-ui-components/public-api/styles/toast-style.h", "ToastStyle::Builder::SetBorderlineOffset(value)",
     "Fraction of the borderline width in [-1, 1], not a length"),
    ("dali-ui-components/public-api/styles/toast-style.h", "ToastStyle::GetBorderlineOffset -> return",
     "Fraction of the borderline width in [-1, 1], not a length"),
    ("dali-ui-foundation/integration-api/animated-image-view-impl.h", "Integration::AnimatedImageViewImpl::Property::BATCH_SIZE",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::BATCH_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/animated-image-view-impl.h", "Integration::AnimatedImageViewImpl::Property::CACHE_SIZE",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::CACHE_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/animated-image-view-impl.h", "Integration::AnimatedImageViewImpl::Property::DESIRED_HEIGHT",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::DESIRED_HEIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/animated-image-view-impl.h", "Integration::AnimatedImageViewImpl::Property::DESIRED_WIDTH",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::DESIRED_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/animated-image-view-impl.h", "Integration::AnimatedImageViewImpl::Property::IMAGE_LOAD_WITH_VIEW_SIZE",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::IMAGE_LOAD_WITH_VIEW_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/animated-image-view-impl.h", "Integration::AnimatedImageViewImpl::Property::PIXEL_AREA",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::PIXEL_AREA, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/builder/json-parser.h", "Integration::JsonParser::GetErrorPosition -> return",
     "Character position or indentation in JSON text, not a length"),
    ("dali-ui-foundation/integration-api/builder/json-parser.h", "Integration::JsonParser::Write(indent)",
     "Character position or indentation in JSON text, not a length"),
    ("dali-ui-foundation/integration-api/image-view-impl.h", "Integration::ImageViewImpl::Property::DESIRED_HEIGHT",
     "Undocumented mirror of ImageViewPropertyIndex::DESIRED_HEIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/image-view-impl.h", "Integration::ImageViewImpl::Property::DESIRED_WIDTH",
     "Undocumented mirror of ImageViewPropertyIndex::DESIRED_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/image-view-impl.h", "Integration::ImageViewImpl::Property::IMAGE_LOAD_WITH_VIEW_SIZE",
     "Undocumented mirror of ImageViewPropertyIndex::IMAGE_LOAD_WITH_VIEW_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/image-view-impl.h", "Integration::ImageViewImpl::Property::N_PATCH_BORDER",
     "Undocumented mirror of ImageViewPropertyIndex::N_PATCH_BORDER, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/image-view-impl.h", "Integration::ImageViewImpl::Property::PIXEL_AREA",
     "Undocumented mirror of ImageViewPropertyIndex::PIXEL_AREA, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/lottie-animation-view-impl.h", "Integration::LottieAnimationViewImpl::Property::DESIRED_HEIGHT",
     "Undocumented mirror of LottieAnimationViewPropertyIndex::DESIRED_HEIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/lottie-animation-view-impl.h", "Integration::LottieAnimationViewImpl::Property::DESIRED_WIDTH",
     "Undocumented mirror of LottieAnimationViewPropertyIndex::DESIRED_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/lottie-animation-view-impl.h", "Integration::LottieAnimationViewImpl::Property::PIXEL_AREA",
     "Undocumented mirror of LottieAnimationViewPropertyIndex::PIXEL_AREA, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/integration-api/shader-effects/dissolve-effect.h", "Integration::DissolveEffectSetCentralLine(displacement)",
     "Direction of the central line in normalized texture coordinates, not a length"),
    ("dali-ui-foundation/integration-api/shader-effects/dissolve-effect.h", "Integration::DissolveEffectSetCentralLine(position)",
     "Normalized position in the unit square"),
    ("dali-ui-foundation/integration-api/text/input-style.h", "Integration::Text::InputStyle::lineSpacing",
     "Stored and compared only; no layout or rendering code reads it, so it has no unit (plan-B section 8)"),
    ("dali-ui-foundation/integration-api/utility/npatch-helper.h", "Integration::NPatchHelper::CreateBorderGeometry(gridSize)",
     "Image texel coordinate or byte offset of the source image, not a screen length"),
    ("dali-ui-foundation/integration-api/utility/npatch-helper.h", "Integration::NPatchHelper::CreateGridGeometry(gridSize)",
     "Image texel coordinate or byte offset of the source image, not a screen length"),
    ("dali-ui-foundation/integration-api/utility/npatch-utilities.h", "Integration::NPatchUtility::GetRedOffsetAndMask(byteOffset)",
     "Image texel coordinate or byte offset of the source image, not a screen length"),
    ("dali-ui-foundation/integration-api/utility/npatch-utilities.h", "Integration::NPatchUtility::GetValidStrechPointFromBorder -> return",
     "Image texel coordinate or byte offset of the source image, not a screen length"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::BORDERLINE_OFFSET",
     "Ratio in [-1, 1] of the borderline width, not a length"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::BORDERLINE_WIDTH",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::CORNER_RADIUS",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::MARGIN",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::MAXIMUM_HEIGHT",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::MAXIMUM_WIDTH",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::MINIMUM_HEIGHT",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::MINIMUM_WIDTH",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::PADDING",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::REQUESTED_HEIGHT",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/view-integ.h", "Integration::View::Property::Type::REQUESTED_WIDTH",
     "Integration mirror of View::Property indices; the unit is documented on View::Property"),
    ("dali-ui-foundation/integration-api/visual-factory/visual-factory.h", "Integration::VisualFactory::CreateGridGeometry(gridSize)",
     "Grid cell count, not a length"),
    ("dali-ui-foundation/integration-api/visuals/gradient-visual-properties-integ.h", "Integration::GradientVisual::Property::START_OFFSET",
     "Fraction of the color ramp, not a length"),
    ("dali-ui-foundation/integration-api/visuals/gradient-visual-properties-integ.h", "Integration::GradientVisual::Property::STOP_OFFSET",
     "Fraction of the color ramp, not a length"),
    ("dali-ui-foundation/integration-api/visuals/image-visual-properties-integ.h", "Integration::ImageVisual::Property::BATCH_SIZE",
     "Frame count, not a length"),
    ("dali-ui-foundation/integration-api/visuals/image-visual-properties-integ.h", "Integration::ImageVisual::Property::CACHE_SIZE",
     "Frame count, not a length"),
    ("dali-ui-foundation/integration-api/visuals/image-visual-properties-integ.h", "Integration::ImageVisual::Property::IMAGE_LOAD_WITH_VIEW_SIZE",
     "Boolean flag, not a length"),
    ("dali-ui-foundation/integration-api/visuals/image-visual-properties-integ.h", "Integration::ImageVisual::Property::PIXEL_AREA",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/integration-api/visuals/image-visual-properties-integ.h", "Integration::ImageVisual::Property::PLAY_RANGE",
     "Frame range of an animated image, not a length"),
    ("dali-ui-foundation/integration-api/visuals/primitive-visual-properties-integ.h", "Integration::PrimitiveVisual::Property::SCALE_BOTTOM_RADIUS",
     "Unitless shape proportion of the primitive"),
    ("dali-ui-foundation/integration-api/visuals/primitive-visual-properties-integ.h", "Integration::PrimitiveVisual::Property::SCALE_HEIGHT",
     "Unitless shape proportion of the primitive"),
    ("dali-ui-foundation/integration-api/visuals/primitive-visual-properties-integ.h", "Integration::PrimitiveVisual::Property::SCALE_RADIUS",
     "Unitless shape proportion of the primitive"),
    ("dali-ui-foundation/integration-api/visuals/primitive-visual-properties-integ.h", "Integration::PrimitiveVisual::Property::SCALE_TOP_RADIUS",
     "Unitless shape proportion of the primitive"),
    ("dali-ui-foundation/integration-api/visuals/text-visual-properties-integ.h", "Integration::TextVisual::Property::FONT_WIDTH",
     "Font stretch class (Text::FontWidth), not a length"),
    ("dali-ui-foundation/integration-api/visuals/visual-properties-integ.h", "Integration::Visual::Property::Type::BORDERLINE_OFFSET",
     "Fraction of the borderline width in [-1, 1], not a length"),
    ("dali-ui-foundation/integration-api/visuals/visual-properties-integ.h", "Integration::Visual::Shader::Property::SUBDIVIDE_GRID_X",
     "Grid subdivision count, not a length"),
    ("dali-ui-foundation/integration-api/visuals/visual-properties-integ.h", "Integration::Visual::Shader::Property::SUBDIVIDE_GRID_Y",
     "Grid subdivision count, not a length"),
    ("dali-ui-foundation/integration-api/visuals/visual-properties-integ.h", "Integration::Visual::Transform::Property::Type::ORIGIN",
     "Alignment origin (Ui::VisualOrigin), not a length"),
    ("dali-ui-foundation/public-api/configuration/ui-color-manager.h", "ColorCallback",
     "Callback that receives a color (Vector4), not a length"),
    ("dali-ui-foundation/public-api/configuration/ui-config.h", "UiConfig::GetScaleForSystemFontSize -> return",
     "Font size scale factor for a system font size, not a length"),
    ("dali-ui-foundation/public-api/gradient/gradient-base.h", "Gradient::Base::GetStartOffset -> return",
     "Fraction of the color ramp, not a length"),
    ("dali-ui-foundation/public-api/gradient/gradient-base.h", "Gradient::Base::SetStartOffset(offset)",
     "Fraction of the color ramp, not a length"),
    ("dali-ui-foundation/public-api/gradient/gradient-stop-node.h", "Gradient::StopNode::GetOffset -> return",
     "Gradient stop offset, a fraction of the color ramp, not a length"),
    ("dali-ui-foundation/public-api/gradient/gradient-stop-node.h", "Gradient::StopNode::SetOffset(offset)",
     "Gradient stop offset, a fraction of the color ramp, not a length"),
    ("dali-ui-foundation/public-api/gradient/gradient-stop-node.h", "Gradient::StopNode::StopNode(offset)",
     "Gradient stop offset, a fraction of the color ramp, not a length"),
    ("dali-ui-foundation/public-api/layouts/layout-transition-types.h", "LayoutBoundsEffect::GetAnchorX -> return",
     "Normalized factor or anchor, not a length"),
    ("dali-ui-foundation/public-api/layouts/layout-transition-types.h", "LayoutBoundsEffect::GetAnchorY -> return",
     "Normalized factor or anchor, not a length"),
    ("dali-ui-foundation/public-api/layouts/layout-transition-types.h", "LayoutBoundsEffect::GetSizeFactorX -> return",
     "Normalized factor or anchor, not a length"),
    ("dali-ui-foundation/public-api/layouts/layout-transition-types.h", "LayoutBoundsEffect::GetSizeFactorY -> return",
     "Normalized factor or anchor, not a length"),
    ("dali-ui-foundation/public-api/layouts/layout-transition-types.h", "LayoutBoundsEffect::SetAnchor(x)",
     "Normalized factor or anchor, not a length"),
    ("dali-ui-foundation/public-api/layouts/layout-transition-types.h", "LayoutBoundsEffect::SetAnchor(y)",
     "Normalized factor or anchor, not a length"),
    ("dali-ui-foundation/public-api/layouts/layout-transition-types.h", "LayoutBoundsEffect::SetSizeFactor(x)",
     "Normalized factor or anchor, not a length"),
    ("dali-ui-foundation/public-api/layouts/layout-transition-types.h", "LayoutBoundsEffect::SetSizeFactor(y)",
     "Normalized factor or anchor, not a length"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::GetHeight -> return",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::GetPosition -> return",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::GetSize -> return",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::GetWidth -> return",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::GetX -> return",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::GetY -> return",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::LayoutRect(h)",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::LayoutRect(posX)",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::LayoutRect(posY)",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::LayoutRect(w)",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::SetHeight(h)",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::SetWidth(w)",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::SetX(posX)",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::SetY(posY)",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::height",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::width",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::x",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/layouts/layout-types.h", "LayoutRect::y",
     "LayoutRect members carry the unit of the API that hands the rect over; the class is used with both units"),
    ("dali-ui-foundation/public-api/render-effects/mask-effect.h", "MaskEffect::New(maskPosition)",
     "Texture UV offset, a fraction of the owner View's size, not pixels (plan-B section 8)"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::CURSOR_POSITION",
     "Character index, not a length"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::FONT_WIDTH",
     "Font stretch class (Text::FontWidth), not a length"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::MAXIMUM_LENGTH",
     "Character count, not a length"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::SELECTED_TEXT_END",
     "Character index, not a length"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::SELECTED_TEXT_START",
     "Character index, not a length"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::SELECTION_HANDLE_IMAGE_LEFT",
     "Image of a selection handle; LEFT and RIGHT name the handle, not a length"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::SELECTION_HANDLE_IMAGE_RIGHT",
     "Image of a selection handle; LEFT and RIGHT name the handle, not a length"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::SELECTION_HANDLE_PRESSED_IMAGE_LEFT",
     "Image of a selection handle; LEFT and RIGHT name the handle, not a length"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::SELECTION_HANDLE_PRESSED_IMAGE_RIGHT",
     "Image of a selection handle; LEFT and RIGHT name the handle, not a length"),
    ("dali-ui-foundation/public-api/text/input-editor-properties.h", "Text::InputEditorPropertyIndex::TYPING_FONT_WIDTH",
     "Font stretch class (Text::FontWidth), not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::CURSOR_POSITION",
     "Character index, not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::FONT_WIDTH",
     "Font stretch class (Text::FontWidth), not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::MAXIMUM_LENGTH",
     "Character count, not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::SELECTED_TEXT_END",
     "Character index, not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::SELECTED_TEXT_START",
     "Character index, not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::SELECTION_HANDLE_IMAGE_LEFT",
     "Image of a selection handle; LEFT and RIGHT name the handle, not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::SELECTION_HANDLE_IMAGE_RIGHT",
     "Image of a selection handle; LEFT and RIGHT name the handle, not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::SELECTION_HANDLE_PRESSED_IMAGE_LEFT",
     "Image of a selection handle; LEFT and RIGHT name the handle, not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::SELECTION_HANDLE_PRESSED_IMAGE_RIGHT",
     "Image of a selection handle; LEFT and RIGHT name the handle, not a length"),
    ("dali-ui-foundation/public-api/text/input-field-properties.h", "Text::InputFieldPropertyIndex::TYPING_FONT_WIDTH",
     "Font stretch class (Text::FontWidth), not a length"),
    ("dali-ui-foundation/public-api/text/label-properties.h", "Text::LabelPropertyIndex::FONT_WIDTH",
     "Font stretch class (Text::FontWidth), not a length"),
    ("dali-ui-foundation/public-api/views/canvas/canvas-view.h", "CanvasView::SetDropShadow(blurRadius)",
     "Canvas effect value passed to the canvas renderer unscaled; whether it is in view box units or buffer pixels is not determinable (plan-B CV-EFFECT)"),
    ("dali-ui-foundation/public-api/views/canvas/canvas-view.h", "CanvasView::SetDropShadow(offsetX)",
     "Canvas effect value passed to the canvas renderer unscaled; whether it is in view box units or buffer pixels is not determinable (plan-B CV-EFFECT)"),
    ("dali-ui-foundation/public-api/views/canvas/canvas-view.h", "CanvasView::SetDropShadow(offsetY)",
     "Canvas effect value passed to the canvas renderer unscaled; whether it is in view box units or buffer pixels is not determinable (plan-B CV-EFFECT)"),
    ("dali-ui-foundation/public-api/views/canvas/canvas-view.h", "CanvasView::SetGaussianBlur(blurRadius)",
     "Canvas effect value passed to the canvas renderer unscaled; whether it is in view box units or buffer pixels is not determinable (plan-B CV-EFFECT)"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view-properties.h", "AnimatedImageViewPropertyIndex::BATCH_SIZE",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view-properties.h", "AnimatedImageViewPropertyIndex::CACHE_SIZE",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view-properties.h", "AnimatedImageViewPropertyIndex::IMAGE_LOAD_WITH_VIEW_SIZE",
     "Boolean flag, not a length"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view-properties.h", "AnimatedImageViewPropertyIndex::PIXEL_AREA",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::GetBatchSize -> return",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::GetCacheSize -> return",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::GetPixelArea -> return",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::Property::BATCH_SIZE",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::BATCH_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::Property::CACHE_SIZE",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::CACHE_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::Property::DESIRED_HEIGHT",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::DESIRED_HEIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::Property::DESIRED_WIDTH",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::DESIRED_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::Property::IMAGE_LOAD_WITH_VIEW_SIZE",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::IMAGE_LOAD_WITH_VIEW_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::Property::PIXEL_AREA",
     "Undocumented mirror of AnimatedImageViewPropertyIndex::PIXEL_AREA, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::SetBatchSize(size)",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::SetCacheSize(size)",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/views/image/animated-image-view.h", "AnimatedImageView::SetPixelArea(pixelArea)",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/image/image-view-properties.h", "ImageViewPropertyIndex::IMAGE_LOAD_WITH_VIEW_SIZE",
     "Boolean flag, not a length"),
    ("dali-ui-foundation/public-api/views/image/image-view-properties.h", "ImageViewPropertyIndex::PIXEL_AREA",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/image/image-view.h", "ImageView::GetPixelArea -> return",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/image/image-view.h", "ImageView::Property::DESIRED_HEIGHT",
     "Undocumented mirror of ImageViewPropertyIndex::DESIRED_HEIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/image-view.h", "ImageView::Property::DESIRED_WIDTH",
     "Undocumented mirror of ImageViewPropertyIndex::DESIRED_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/image-view.h", "ImageView::Property::IMAGE_LOAD_WITH_VIEW_SIZE",
     "Undocumented mirror of ImageViewPropertyIndex::IMAGE_LOAD_WITH_VIEW_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/image-view.h", "ImageView::Property::N_PATCH_BORDER",
     "Undocumented mirror of ImageViewPropertyIndex::N_PATCH_BORDER, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/image-view.h", "ImageView::Property::PIXEL_AREA",
     "Undocumented mirror of ImageViewPropertyIndex::PIXEL_AREA, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/image-view.h", "ImageView::SetPixelArea(pixelArea)",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/image/lottie-animation-view-properties.h", "LottieAnimationViewPropertyIndex::PIXEL_AREA",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/image/lottie-animation-view.h", "LottieAnimationView::GetPixelArea -> return",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/image/lottie-animation-view.h", "LottieAnimationView::Property::DESIRED_HEIGHT",
     "Undocumented mirror of LottieAnimationViewPropertyIndex::DESIRED_HEIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/lottie-animation-view.h", "LottieAnimationView::Property::DESIRED_WIDTH",
     "Undocumented mirror of LottieAnimationViewPropertyIndex::DESIRED_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/lottie-animation-view.h", "LottieAnimationView::Property::PIXEL_AREA",
     "Undocumented mirror of LottieAnimationViewPropertyIndex::PIXEL_AREA, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/image/lottie-animation-view.h", "LottieAnimationView::SetPixelArea(pixelArea)",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/media/media-view.h", "MediaView::GetSourceCrop -> return",
     "Normalized source rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/media/media-view.h", "MediaView::SetSourceCrop(crop)",
     "Normalized source rectangle, not a length"),
    ("dali-ui-foundation/public-api/views/progress-view.h", "ProgressView::ValueChangedSignalType",
     "Progress value, not a length"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::GetMaximumLength -> return",
     "Character count, not a length"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::CURSOR_POSITION",
     "Undocumented mirror of Text::InputEditorPropertyIndex::CURSOR_POSITION, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::CURSOR_WIDTH",
     "Undocumented mirror of Text::InputEditorPropertyIndex::CURSOR_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::FONT_SIZE",
     "Undocumented mirror of Text::InputEditorPropertyIndex::FONT_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::FONT_WIDTH",
     "Undocumented mirror of Text::InputEditorPropertyIndex::FONT_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::LINE_HEIGHT",
     "Undocumented mirror of Text::InputEditorPropertyIndex::LINE_HEIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::MAXIMUM_LENGTH",
     "Undocumented mirror of Text::InputEditorPropertyIndex::MAXIMUM_LENGTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::SELECTED_TEXT_END",
     "Undocumented mirror of Text::InputEditorPropertyIndex::SELECTED_TEXT_END, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::SELECTED_TEXT_START",
     "Undocumented mirror of Text::InputEditorPropertyIndex::SELECTED_TEXT_START, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::SELECTION_HANDLE_IMAGE_LEFT",
     "Undocumented mirror of Text::InputEditorPropertyIndex::SELECTION_HANDLE_IMAGE_LEFT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::SELECTION_HANDLE_IMAGE_RIGHT",
     "Undocumented mirror of Text::InputEditorPropertyIndex::SELECTION_HANDLE_IMAGE_RIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::SELECTION_HANDLE_PRESSED_IMAGE_LEFT",
     "Undocumented mirror of Text::InputEditorPropertyIndex::SELECTION_HANDLE_PRESSED_IMAGE_LEFT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::SELECTION_HANDLE_PRESSED_IMAGE_RIGHT",
     "Undocumented mirror of Text::InputEditorPropertyIndex::SELECTION_HANDLE_PRESSED_IMAGE_RIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::TYPING_FONT_SIZE",
     "Undocumented mirror of Text::InputEditorPropertyIndex::TYPING_FONT_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::Property::TYPING_FONT_WIDTH",
     "Undocumented mirror of Text::InputEditorPropertyIndex::TYPING_FONT_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-editor.h", "InputEditor::SetMaximumLength(length)",
     "Character count, not a length"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::GetMaximumLength -> return",
     "Character count, not a length"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::CURSOR_POSITION",
     "Undocumented mirror of Text::InputFieldPropertyIndex::CURSOR_POSITION, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::CURSOR_WIDTH",
     "Undocumented mirror of Text::InputFieldPropertyIndex::CURSOR_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::FONT_SIZE",
     "Undocumented mirror of Text::InputFieldPropertyIndex::FONT_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::FONT_WIDTH",
     "Undocumented mirror of Text::InputFieldPropertyIndex::FONT_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::MAXIMUM_LENGTH",
     "Undocumented mirror of Text::InputFieldPropertyIndex::MAXIMUM_LENGTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::SELECTED_TEXT_END",
     "Undocumented mirror of Text::InputFieldPropertyIndex::SELECTED_TEXT_END, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::SELECTED_TEXT_START",
     "Undocumented mirror of Text::InputFieldPropertyIndex::SELECTED_TEXT_START, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::SELECTION_HANDLE_IMAGE_LEFT",
     "Undocumented mirror of Text::InputFieldPropertyIndex::SELECTION_HANDLE_IMAGE_LEFT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::SELECTION_HANDLE_IMAGE_RIGHT",
     "Undocumented mirror of Text::InputFieldPropertyIndex::SELECTION_HANDLE_IMAGE_RIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::SELECTION_HANDLE_PRESSED_IMAGE_LEFT",
     "Undocumented mirror of Text::InputFieldPropertyIndex::SELECTION_HANDLE_PRESSED_IMAGE_LEFT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::SELECTION_HANDLE_PRESSED_IMAGE_RIGHT",
     "Undocumented mirror of Text::InputFieldPropertyIndex::SELECTION_HANDLE_PRESSED_IMAGE_RIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::TYPING_FONT_SIZE",
     "Undocumented mirror of Text::InputFieldPropertyIndex::TYPING_FONT_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::Property::TYPING_FONT_WIDTH",
     "Undocumented mirror of Text::InputFieldPropertyIndex::TYPING_FONT_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/input-field.h", "InputField::SetMaximumLength(length)",
     "Character count, not a length"),
    ("dali-ui-foundation/public-api/views/text-controls/label.h", "Label::Property::FONT_SIZE",
     "Undocumented mirror of Text::LabelPropertyIndex::FONT_SIZE, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/label.h", "Label::Property::FONT_WIDTH",
     "Undocumented mirror of Text::LabelPropertyIndex::FONT_WIDTH, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/label.h", "Label::Property::LINE_HEIGHT",
     "Undocumented mirror of Text::LabelPropertyIndex::LINE_HEIGHT, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/label.h", "Label::Property::MARQUEE_GAP",
     "Undocumented mirror of Text::LabelPropertyIndex::MARQUEE_GAP, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/text-controls/label.h", "Label::Property::MARQUEE_SPEED",
     "Undocumented mirror of Text::LabelPropertyIndex::MARQUEE_SPEED, which documents the value (decision A-Q6)"),
    ("dali-ui-foundation/public-api/views/view.h", "View::GetBorderlineOffset -> return",
     "Ratio in [-1, 1] of the borderline width, not a length"),
    ("dali-ui-foundation/public-api/views/view.h", "View::Property::BORDERLINE_OFFSET",
     "Ratio in [-1, 1] of the borderline width, not a length"),
    ("dali-ui-foundation/public-api/views/view.h", "View::SetBorderlineOffset(offset)",
     "Ratio in [-1, 1] of the borderline width, not a length"),
    ("dali-ui-foundation/public-api/views/view.h", "View::SetCornerSquareness(bottomLeft)",
     "Corner squareness ratio, not a length"),
    ("dali-ui-foundation/public-api/views/view.h", "View::SetCornerSquareness(bottomRight)",
     "Corner squareness ratio, not a length"),
    ("dali-ui-foundation/public-api/views/view.h", "View::SetCornerSquareness(topLeft)",
     "Corner squareness ratio, not a length"),
    ("dali-ui-foundation/public-api/views/view.h", "View::SetCornerSquareness(topRight)",
     "Corner squareness ratio, not a length"),
    ("dali-ui-foundation/public-api/views/web/web-view.h", "WebView::FeedMouseWheel(step)",
     "Wheel step count, not a length"),
    ("dali-ui-foundation/public-api/visuals/animated-image-visual.h", "AnimatedImageVisual::GetBatchSize -> return",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/visuals/animated-image-visual.h", "AnimatedImageVisual::GetCacheSize -> return",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/visuals/animated-image-visual.h", "AnimatedImageVisual::GetPixelArea -> return",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/visuals/animated-image-visual.h", "AnimatedImageVisual::SetBatchSize(batchSize)",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/visuals/animated-image-visual.h", "AnimatedImageVisual::SetCacheSize(cacheSize)",
     "Frame count, not a length"),
    ("dali-ui-foundation/public-api/visuals/animated-image-visual.h", "AnimatedImageVisual::SetPixelArea(pixelArea)",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/visuals/image-visual.h", "ImageVisual::GetPixelArea -> return",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/visuals/image-visual.h", "ImageVisual::SetPixelArea(pixelArea)",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/visuals/lottie-animation-visual.h", "LottieAnimationVisual::GetPixelArea -> return",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/visuals/lottie-animation-visual.h", "LottieAnimationVisual::SetPixelArea(pixelArea)",
     "Normalized texture rectangle, not a length"),
    ("dali-ui-foundation/public-api/visuals/visual-base.h", "VisualBase::GetBorderlineOffset -> return",
     "Fraction of the borderline width in [-1, 1], not a length"),
    ("dali-ui-foundation/public-api/visuals/visual-base.h", "VisualBase::SetBorderlineOffset(offset)",
     "Fraction of the borderline width in [-1, 1], not a length"),
    ("dali-ui-foundation/public-api/visuals/visual-base.h", "VisualBase::SetCornerSquareness(bottomLeft)",
     "Corner squareness ratio, not a length"),
    ("dali-ui-foundation/public-api/visuals/visual-base.h", "VisualBase::SetCornerSquareness(bottomRight)",
     "Corner squareness ratio, not a length"),
    ("dali-ui-foundation/public-api/visuals/visual-base.h", "VisualBase::SetCornerSquareness(topLeft)",
     "Corner squareness ratio, not a length"),
    ("dali-ui-foundation/public-api/visuals/visual-base.h", "VisualBase::SetCornerSquareness(topRight)",
     "Corner squareness ratio, not a length"),
]
# fmt: on

VALID_TAGS = ("LOGICAL", "PHYSICAL")
TAG_RE = re.compile(r"[@\\]UNIT_(\w+)")

# The definitions of the two tags. Each file of DEFINITION_FILES repeats them word for word, a header with each
# definition right after its tag token, and --all reports a file that does not; line breaks, indentation and the
# leading * or // of comment lines do not count.
DEFINITIONS = {
    "LOGICAL": (
        "Logical value. The layout multiplies it by the effective layout scale of the view it belongs to, the UI scale "
        "times the window scale, every time it is used, so a later scale change applies automatically. Getters return "
        "it as set. It equals physical pixels only at scale 1.0. The _dp, _sdp and _spx literals produce logical "
        "values."
    ),
    "PHYSICAL": (
        "Physical value. Pixels in the units dali-core uses, such as Actor::Property::SIZE, POSITION and touch "
        "coordinates, with the effective layout scale already applied. A physical value the framework stores, such as "
        "a scroll position, keeps its number when the scale changes, so it can then point at different content."
    ),
}
DEFINITION_FILES = ("dali-ui-foundation/public-api/types/unit.h", "docs/length-units.md", "rules/length-units.md")

# Types that are a length whatever their name.
ALWAYS_LENGTH_TYPES = {"LayoutRect", "MeasuredSize", "Insets", "Extents", "Padding", "PositionSize"}
# Types that are a length only with a length name.
NAMED_LENGTH_TYPES = {
    "float", "double", "int", "short", "long", "short int", "long int", "long long", "signed", "signed int",
    "int8_t", "int16_t", "int32_t", "int64_t", "Vector2", "Vector3", "Vector4", "Size", "Uint16Pair",
}
# A signal or callback alias is a candidate when one of these types is in its argument list.
SIGNAL_ARGUMENT_RE = re.compile(
    r"\b(float|double|Vector2|Vector3|Vector4|LayoutRect|MeasuredSize|Insets|Extents|Padding|PositionSize|Rect\s*<)")

# The last word of a length name. A function name first loses a leading Get, Set, Calculate, Compute or To.
LENGTH_WORDS = set("""
x y z w h width height depth size position pos offset offsets spacing gap margin padding inset insets radius
thickness extent distance delta dx dy length step peek bounds rect area constraint threshold speed velocity
displacement coordinate point origin indent outset outsets border basis left right top bottom start end pixels extents
dash range
""".split())
FUNCTION_PREFIX_RE = re.compile(r"^(Get|Set|Calculate|Compute|To)(?=[A-Z0-9_]|$)")
# A parameter with one of these names takes the length name of its setter, as in SetTitleFontSize(float value).
GENERIC_PARAMETER_NAMES = {"value", "v", "val"}
# A parameter with one of these names is a length when its type is a length type, as in SetMargin(float uniform).
DIRECTION_PARAMETER_NAMES = {"uniform", "horizontal", "vertical"}

# Namespaces whose content is not checked (besides anonymous namespaces).
HIDDEN_NAMESPACES = {"Internal", "Detail", "detail"}
# Leading scopes left out of the item keys.
KEY_SKIPPED_SCOPES = {"DALI_NAMESPACE", "Dali", "Tizen", "Ui"}

# Commands that start a new paragraph of a documentation block.
SECTION_COMMANDS = {
    "param", "tparam", "return", "returns", "result", "retval", "brief", "short", "details", "note", "notes", "see",
    "sa", "pre", "post", "warning", "attention", "since", "deprecated", "remark", "remarks", "par", "paragraph",
    "throw", "throws", "exception", "invariant", "todo", "bug", "test", "copydoc", "copybrief", "copydetails",
    "code", "endcode", "verbatim", "endverbatim", "dot", "enddot", "cond", "endcond", "internal", "endinternal",
    "li", "arg", "image", "snippet", "include", "example", "ingroup", "addtogroup", "defgroup", "name", "class",
    "struct", "enum", "fn", "var", "typedef", "file", "namespace", "page", "section", "subsection",
}
SECTION_RE = re.compile(r"(?:(?<=\s)|^)[@\\](\w+)")
INHERITED_DOC_RE = re.compile(r"[@\\](copydoc|copybrief|copydetails)\b")
COND_RE = re.compile(r"[@\\](cond|endcond)\b")
MESSAGES = {
    "MISSING": "no @UNIT_LOGICAL or @UNIT_PHYSICAL tag",
    "MULTIPLE": "more than one tag",
    "MISPLACED": "the tag is not the first token",
}

SPECIFIERS = {
    "static", "virtual", "inline", "explicit", "constexpr", "consteval", "friend", "extern", "mutable", "typename",
    "thread_local", "register", "volatile",
}


class CheckError(Exception):
    """A failure that ends the check with exit code 2."""


class Comment:
    """One comment of a header."""

    def __init__(self, start, end, text, line, end_line):
        self.start = start
        self.end = end
        self.text = text
        self.line = line
        self.end_line = end_line
        self.line_comment = text.startswith("//")
        self.trailing = text.startswith(("///<", "//!<", "/**<", "/*!<"))
        self.doc = (text.startswith(("/**", "/*!")) and not text.startswith(("/**/", "/***"))) or (
            text.startswith(("///", "//!")) and not text.startswith("////"))

    def doc_lines(self):
        """Return [(line, text)] of the documentation text without the comment markers."""
        if self.line_comment:
            return [(self.line, re.sub(r"^//[/!]<?", "", self.text))]
        body = re.sub(r"^/\*[*!]<?", "", self.text)
        if body.endswith("*/"):
            body = body[:-2]
        result = []
        for offset, part in enumerate(body.split("\n")):
            if offset > 0:
                part = re.sub(r"^\s*\*", "", part)
            result.append((self.line + offset, part))
        return result


class Item:
    """A length candidate: a parameter, a return value, a member, a registered property or a signal alias."""

    def __init__(self, path, key, kind, scope, function, name, type_name, decl_lines, docs, trailing):
        self.path = path
        self.line = decl_lines[0]
        self.key = key
        self.kind = kind  # PARAM, OUT, RETURN, MEMBER, PROPERTY, ALIAS
        self.scope = scope
        self.function = function
        self.name = name
        self.type_name = type_name
        self.decl_lines = decl_lines
        self.docs = docs
        self.trailing = trailing
        self.tag = None
        self.problem = None

    def lines(self):
        result = set(range(self.decl_lines[0], self.decl_lines[1] + 1))
        for comment in self.docs + self.trailing:
            result.update(range(comment.line, comment.end_line + 1))
        return result


class Scope:
    def __init__(self, kind, names, hidden, access=None):
        self.kind = kind  # namespace, class, enum or block
        self.names = names
        self.hidden = hidden
        self.access = access


def split_words(name):
    name = re.sub(r"^m(?=[A-Z])", "", name)
    if "_" in name:
        return [part.lower() for part in name.split("_") if part]
    return [part.lower() for part in re.findall(r"[A-Z]+(?=[A-Z][a-z]|\d|$)|[A-Z]?[a-z]+|[A-Z]+|\d+", name)]


def is_length_name(name, function=False):
    if function:
        name = FUNCTION_PREFIX_RE.sub("", name)
    words = split_words(name)
    return bool(words) and words[-1] in LENGTH_WORDS


def base_type(type_text):
    """Return the type without qualifiers, references, pointers and the Dali and Ui namespaces."""
    text = re.sub(r"\b(const|volatile|struct|class|typename)\b", " ", type_text)
    text = text.replace("&", " ").replace("*", " ")
    text = re.sub(r"\b(DALI_NAMESPACE|Dali|Ui)\s*::\s*", "", text)
    text = re.sub(r"^\s*::\s*", "", text)
    text = re.sub(r"\s+", " ", text).strip()
    return re.sub(r"\s*([<>,])\s*", r"\1", text)


def length_type(type_text):
    """Return 'always', 'named' or None."""
    text = base_type(type_text)
    if text in ALWAYS_LENGTH_TYPES or re.fullmatch(r"Rect<.*>", text):
        return "always"
    if text in NAMED_LENGTH_TYPES:
        return "named"
    return None


def is_out_type(type_text):
    text = type_text.strip()
    return "&&" not in text and ("&" in text or "*" in text) and not text.startswith("const")


def matching(text, start, open_char, close_char):
    """Return the index of the bracket that closes the one at start, or -1."""
    depth = 0
    for index in range(start, len(text)):
        if text[index] == open_char:
            depth += 1
        elif text[index] == close_char:
            depth -= 1
            if depth == 0:
                return index
    return -1


def split_top_level(text, separator=","):
    parts = []
    depth = 0
    current = []
    for char in text:
        if char in "(<[{":
            depth += 1
        elif char in ")>]}":
            depth -= 1
        if char == separator and depth == 0:
            parts.append("".join(current))
            current = []
        else:
            current.append(char)
    parts.append("".join(current))
    return parts


def strip_template_prefix(text):
    if not text.startswith("template"):
        return text
    open_at = text.find("<")
    close = matching(text, open_at, "<", ">") if open_at >= 0 else -1
    return text[close + 1:].strip() if close >= 0 else text


def remove_template_arguments(text):
    previous = None
    while previous != text:
        previous = text
        text = re.sub(r"<[^<>]*>", "", text)
    return text


class Header:
    """Parses one header into its classes and length candidates."""

    def __init__(self, root, path):
        self.path = path
        try:
            self.text = (root / path).read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as error:
            raise CheckError(f"{path}: cannot read: {error}")
        self.line_starts = [0] + [index + 1 for index, char in enumerate(self.text) if char == "\n"]
        self.code, self.comments = self.lex()
        self.comment_starts = [comment.start for comment in self.comments]
        self.cond_ranges = self.find_cond_ranges()
        self.items = []
        self.classes = []  # (qualified name, simple name, documentation comments, line)

    def line_of(self, offset):
        return bisect.bisect_right(self.line_starts, offset)

    def line_end(self, line):
        return self.line_starts[line] if line < len(self.line_starts) else len(self.text)

    def lex(self):
        """Blank comments, literals and preprocessor lines; return (code, comments)."""
        text = self.text
        length = len(text)
        code = list(text)
        comments = []

        def blank(start, end):
            for index in range(start, end):
                if code[index] != "\n":
                    code[index] = " "

        index = 0
        while index < length:
            char = text[index]
            if text.startswith("//", index):
                end = text.find("\n", index)
                end = length if end < 0 else end
                comments.append(Comment(index, end, text[index:end], self.line_of(index), self.line_of(end - 1)))
                blank(index, end)
                index = end
            elif text.startswith("/*", index):
                end = text.find("*/", index + 2)
                end = length if end < 0 else end + 2
                comments.append(Comment(index, end, text[index:end], self.line_of(index), self.line_of(end - 1)))
                blank(index, end)
                index = end
            elif char in "\"'":
                end = index + 1
                while end < length and text[end] != char and text[end] != "\n":
                    end += 2 if text[end] == "\\" else 1
                blank(index + 1, min(end, length))
                index = end + 1
            else:
                index += 1
        lines = "".join(code).split("\n")
        continued = False
        for number, line in enumerate(lines):
            if continued or line.lstrip().startswith("#"):
                continued = line.rstrip().endswith("\\")
                lines[number] = " " * len(line)
        return "\n".join(lines), comments

    def find_cond_ranges(self):
        ranges = []
        depth = 0
        start = None
        for comment in self.comments:
            for match in COND_RE.finditer(comment.text):
                if match.group(1) == "cond":
                    if depth == 0:
                        start = comment.start
                    depth += 1
                elif depth > 0:
                    depth -= 1
                    if depth == 0:
                        ranges.append((start, comment.end))
        if depth > 0:
            ranges.append((start, len(self.text)))
        return ranges

    def in_cond(self, offset):
        return any(start <= offset < end for start, end in self.cond_ranges)

    def comments_between(self, start, end):
        first = bisect.bisect_left(self.comment_starts, start)
        result = []
        for comment in self.comments[first:]:
            if comment.start >= end:
                break
            result.append(comment)
        return result

    def leading_docs(self, gap_start, decl_start):
        return [comment for comment in self.comments_between(gap_start, decl_start)
                if comment.doc and not comment.trailing and not COND_RE.search(comment.text)]

    def trailing_docs(self, last_code, terminator):
        """Trailing documentation comments on the line of the last code character or of the terminator."""
        lines = {self.line_of(last_code), self.line_of(terminator)}
        end = self.line_end(max(lines))
        return [comment for comment in self.comments_between(last_code, end)
                if comment.trailing and comment.line in lines]

    # ------------------------------------------------------------------------------------------------ parsing

    def parse(self):
        code = self.code
        length = len(code)
        stack = []
        statement_start = 0
        paren = 0
        index = 0
        while index < length:
            char = code[index]
            if char == "(":
                paren += 1
            elif char == ")":
                paren = max(0, paren - 1)
            elif paren == 0 and char in ";{},":
                if char == "," and not (stack and stack[-1].kind == "enum"):
                    index += 1
                    continue
                if char == "{":
                    if self.open_scope(statement_start, index, stack):
                        statement_start = index + 1
                    else:
                        close = matching(code, index, "{", "}")
                        if close < 0:
                            break
                        after = code[close + 1:].lstrip()[:1]
                        if after in (";", ",", ")", ""):
                            index = close + 1  # A braced initializer: the statement goes on.
                            continue
                        self.handle(statement_start, index, stack)  # A function body.
                        index = close + 1
                        statement_start = index
                        continue
                elif char == "}":
                    if stack and stack[-1].kind == "enum":
                        self.handle(statement_start, index, stack)
                    else:
                        self.strip_labels(code[statement_start:index], stack)
                    if stack:
                        stack.pop()
                    statement_start = index + 1
                else:
                    self.handle(statement_start, index, stack)
                    statement_start = index + 1
            index += 1

    @staticmethod
    def scope_path(stack):
        names = [name for scope in stack if scope.kind in ("namespace", "class", "enum") for name in scope.names
                 if name]
        while names and names[0] in KEY_SKIPPED_SCOPES:
            names.pop(0)
        return "::".join(names)

    @staticmethod
    def hidden(stack):
        if not stack:
            return False
        return stack[-1].hidden or (stack[-1].kind == "class" and stack[-1].access == "private")

    @staticmethod
    def strip_labels(statement, stack):
        """Apply and blank the access labels at the start of a statement."""
        while True:
            match = re.match(r"\s*(public|protected|private)\s*:(?!:)", statement)
            if not match:
                return statement
            if stack and stack[-1].kind == "class":
                stack[-1].access = match.group(1)
            statement = " " * match.end() + statement[match.end():]

    def declaration_start(self, start, end, stack):
        """Return (gap start, declaration start) after the access labels and macro calls of a statement."""
        statement = self.strip_labels(self.code[start:end], stack)
        gap = start
        while True:
            first = re.search(r"\S", statement)
            if not first:
                return gap, None
            match = re.match(r"[A-Z][A-Z0-9_]*\s*\(", statement[first.start():])
            if not match:
                return gap, start + first.start()
            close = matching(statement, first.start() + match.end() - 1, "(", ")")
            if close < 0:
                return gap, None
            statement = " " * (close + 1) + statement[close + 1:]
            gap = start + close + 1

    def open_scope(self, start, end, stack):
        """Push a namespace, class, struct, union, enum or linkage scope; return False when it is none of them."""
        gap, decl_start = self.declaration_start(start, end, stack)
        text = re.sub(r"\s+", " ", self.code[decl_start:end] if decl_start is not None else "").strip()
        text = strip_template_prefix(text)
        text = re.sub(r"\[\[.*?\]\]", "", text).strip()
        parent_hidden = self.hidden(stack)
        match = re.fullmatch(r"(?:inline )?namespace ?([\w: ]*)", text)
        if match:
            names = [name.strip() for name in match.group(1).split("::")] if match.group(1).strip() else [""]
            hidden = parent_hidden or any(name in HIDDEN_NAMESPACES or not name for name in names)
            stack.append(Scope("namespace", names, hidden))
            return True
        if re.fullmatch(r'extern " *"', text):
            stack.append(Scope("block", [], parent_hidden))
            return True
        match = re.match(r"(?:typedef )?enum\b(?: (?:class|struct)\b)?(.*)$", text)
        if match:
            head = match.group(1).split(":")[0].split()
            stack.append(Scope("enum", [head[-1] if head else ""], parent_hidden))
            return True
        match = re.match(r"(?:typedef )?(class|struct|union)\b(.*)$", text)
        if match:
            head = re.split(r"(?<!:):(?!:)", remove_template_arguments(match.group(2)))[0]
            head = re.sub(r"\balignas ?\(.*?\)", "", head)
            tokens = [token for token in re.findall(r"[\w:]+", head) if token != "final"]
            names = tokens[-1].split("::") if tokens else [""]  # class Outer::Inner defines a nested class.
            name = names[-1]
            qualified = "::".join(filter(None, [self.scope_path(stack)] + names))
            stack.append(Scope("class", names, parent_hidden, "private" if match.group(1) == "class" else "public"))
            if not parent_hidden and not self.in_cond(decl_start):
                self.classes.append((qualified, name, self.leading_docs(gap, decl_start), self.line_of(decl_start)))
            return True
        return False

    def handle(self, start, end, stack):
        gap, decl_start = self.declaration_start(start, end, stack)
        if decl_start is None or self.hidden(stack) or self.in_cond(decl_start):
            return
        statement = self.code[decl_start:end]
        last_code = decl_start + len(statement.rstrip())
        docs = self.leading_docs(gap, decl_start)
        trailing = self.trailing_docs(last_code, end)
        text = re.sub(r"\s+", " ", statement).strip()
        scope = self.scope_path(stack)
        decl_lines = (self.line_of(decl_start), self.line_of(max(decl_start, last_code - 1)))

        def add(kind, function, name, type_name, suffix):
            key = "::".join(filter(None, [scope, suffix]))
            self.items.append(Item(self.path, key, kind, scope, function, name, type_name, decl_lines, docs,
                                   trailing))

        if stack and stack[-1].kind == "enum":
            match = re.match(r"([A-Za-z_]\w*)\b", text)
            names = [name for entry in stack for name in entry.names]
            if match and any(name == "Property" or name.endswith("PropertyIndex") for name in names):
                if is_length_name(match.group(1)):
                    add("PROPERTY", None, match.group(1), None, match.group(1))
            return
        if text.startswith(("friend ", "static_assert", "namespace ")):
            return
        text = strip_template_prefix(text)
        text = re.sub(r"\[\[.*?\]\]", "", text).strip()
        match = re.match(r"(?:using (\w+) ?= ?(.*)|typedef (.*?) ?(\w+))$", text)
        if match:
            alias = match.group(1) or match.group(4)
            aliased = match.group(2) or match.group(3) or ""
            signal = re.search(r"\b(Signal|Callback|function)\s*<(.*)>", aliased)
            if signal and SIGNAL_ARGUMENT_RE.search(signal.group(2)):
                add("ALIAS", None, alias, None, alias)
            return
        if text.startswith(("using ", "typedef ")) or re.match(r"(class|struct|union|enum)\b[^({=]*$", text):
            return
        paren_at = text.find("(")
        if re.search(r"\boperator\b", text[:paren_at] if paren_at >= 0 else text):
            return
        equal = re.search(r"(?<![=!<>+\-*/%&|^])=(?!=)", text)
        if paren_at >= 0 and (not equal or paren_at < equal.start()):
            self.handle_function(text, stack, add)
        else:
            self.handle_variables(text, add)

    @staticmethod
    def handle_function(text, stack, add):
        depth = 0
        for index, char in enumerate(text):
            if char in "(<":
                depth += 1
            elif char in ")>":
                depth -= 1
            elif (char == ":" and depth == 0 and text[index + 1:index + 2] != ":" and text[index - 1:index] != ":"
                  and re.search(r"\)\s*(noexcept\s*)?$", text[:index])):
                text = text[:index].strip()  # A constructor initializer list.
                break
        text = re.sub(r"\s*=\s*(0|default|delete)\s*$", "", text).strip()
        close = text.rfind(")")
        while close >= 0 and not re.fullmatch(
                r"(\s*(const|noexcept(\s*\(.*\))?|override|final|volatile|&&?|throw\s*\(\s*\)|->\s*[\w:<>,\s&*]+))*\s*",
                text[close + 1:]):
            close = text.rfind(")", 0, close)
        if close < 0:
            return
        depth = 0
        open_at = -1
        for index in range(close, -1, -1):
            if text[index] == ")":
                depth += 1
            elif text[index] == "(":
                depth -= 1
                if depth == 0:
                    open_at = index
                    break
        if open_at < 0:
            return
        head = text[:open_at].rstrip()
        match = re.search(r"(~?[A-Za-z_]\w*)$", head)
        if not match or match.group(1).startswith("~"):
            return
        name = match.group(1)
        return_text = re.sub(r"[\w:]+::$", "", head[:match.start()].strip()).strip()
        return_type = " ".join(token for token in return_text.split()
                               if token not in SPECIFIERS and not re.fullmatch(r"[A-Z][A-Z0-9_]+", token))
        if return_type and return_type != "void":
            kind = length_type(return_type)
            if kind == "always" or (kind == "named" and is_length_name(name, function=True)):
                add("RETURN", name, None, return_type, f"{name} -> return")
        params = text[open_at + 1:close].strip()
        if not params or params == "void":
            return
        for param in split_top_level(params):
            param = split_top_level(param, "=")[0].strip()
            param = re.sub(r"\[[^\]]*\]$", "", param).strip()
            match = re.match(r"(.*[\w>&*])\s*\b([A-Za-z_]\w*)$", param)
            if not match:
                continue
            type_text = match.group(1).strip()
            param_name = match.group(2)
            if type_text in ("const", "unsigned", "signed", "long", "short") or param_name in (
                    "int", "long", "short", "float", "double", "char"):
                continue
            kind = length_type(type_text)
            named = (is_length_name(param_name) or param_name in DIRECTION_PARAMETER_NAMES
                     or (param_name in GENERIC_PARAMETER_NAMES and name.startswith("Set")
                         and is_length_name(name, function=True)))
            if kind == "always" or (kind == "named" and named):
                add("OUT" if is_out_type(type_text) else "PARAM", name, param_name, type_text,
                    f"{name}({param_name})")

    @staticmethod
    def handle_variables(text, add):
        text = re.sub(r"\{.*\}$", "", text).strip()
        declarators = split_top_level(text)
        first = re.sub(r"\[[^\]]*\]$", "", split_top_level(declarators[0], "=")[0].strip()).strip()
        match = re.match(r"(.*[\w>&*])\s*\b([A-Za-z_]\w*)$", first)
        if not match:
            return
        type_text = " ".join(token for token in match.group(1).split()
                             if token not in SPECIFIERS and not re.fullmatch(r"[A-Z][A-Z0-9_]+_API", token))
        names = [match.group(2)]
        for declarator in declarators[1:]:
            declarator = split_top_level(declarator, "=")[0].strip()
            declarator = re.sub(r"\[[^\]]*\]$", "", declarator).strip().lstrip("*&").strip()
            if re.fullmatch(r"[A-Za-z_]\w*", declarator):
                names.append(declarator)
        kind = length_type(type_text)
        for name in names:
            if kind == "always" or (kind == "named" and is_length_name(name)):
                add("MEMBER", None, name, type_text, name)


# ------------------------------------------------------------------------------------------------ documentation


def paragraphs(comments):
    """Split documentation comments into paragraphs: [(command or None, text)].

    A run of // comments on consecutive lines is one block; a block comment starts a new block.
    """
    result = []
    current = None
    in_code = False
    previous = None
    for comment in comments:
        if not (comment.line_comment and previous is not None and previous.line_comment
                and comment.line == previous.end_line + 1):
            current = None
        previous = comment
        for line, content in comment.doc_lines():
            if in_code:
                if re.search(r"[@\\]end(code|verbatim|dot)\b", content):
                    in_code = False
                continue
            if not content.strip():
                current = None
                continue
            position = 0
            for match in SECTION_RE.finditer(content):
                command = match.group(1)
                if command not in SECTION_COMMANDS and not command.startswith("SINCE"):
                    continue
                before = content[position:match.start()]
                if current is not None:
                    current[1].append(before)
                elif before.strip():
                    result.append([None, [before]])
                current = [command, []]
                result.append(current)
                position = match.end()
                if command in ("code", "verbatim", "dot"):
                    in_code = True
                    current = None
                    break
            if in_code:
                continue
            if current is None:
                current = [None, []]
                result.append(current)
            current[1].append(content[position:])
    return [(command, " ".join(" ".join(parts).split())) for command, parts in result]


def tag_of(text):
    """Return (tag, problem) of a paragraph whose first token must be the tag."""
    valid = [word for word in TAG_RE.findall(text) if word in VALID_TAGS]
    if not valid:
        return None, "MISSING"
    if len(valid) > 1:
        return None, "MULTIPLE"
    words = text.split()
    if not words or not TAG_RE.fullmatch(words[0]):
        return valid[0], "MISPLACED"
    return valid[0], None


def param_texts(paras, name):
    texts = []
    for command, text in paras:
        if command != "param":
            continue
        match = re.match(r"(?:\[[^\]]*\])?\s*([\w,\s]*?\w)(?:\s+|$)(.*)$", text)
        if match and name in [part.strip() for part in match.group(1).split(",")]:
            texts.append(match.group(2))
    return texts


def last_block(comments):
    """The documentation block nearest to the declaration: its last block comment or its last run of // lines."""
    if not comments:
        return []
    block = [comments[-1]]
    if comments[-1].line_comment:
        for comment in reversed(comments[:-1]):
            if not (comment.line_comment and comment.end_line + 1 == block[0].line):
                break
            block.insert(0, comment)
    return block


def own_text(paras):
    """The first paragraph of a member, property or alias documentation, or None."""
    for command, text in paras:
        if command in (None, "brief", "short", "details"):
            return text
        if command in ("param", "tparam", "return", "returns"):
            return None
    return None


# ------------------------------------------------------------------------------------------------ checking


def class_tags_of(headers):
    """Return ({qualified class: tag}, {simple class name: tag}, violations)."""
    qualified_tags = {}
    simple_tags = {}
    violations = []
    for header in headers:
        for qualified, simple, docs, line in header.classes:
            valid = [word for comment in docs for word in TAG_RE.findall(comment.text) if word in VALID_TAGS]
            if len(valid) > 1:
                lines = {number for comment in docs for number in range(comment.line, comment.end_line + 1)}
                violations.append((header.path, line, "MULTIPLE", qualified, "more than one tag in the class "
                                                                             "documentation", lines))
            elif valid:
                qualified_tags[qualified] = valid[0]
                simple_tags.setdefault(simple, valid[0])
    return qualified_tags, simple_tags, violations


def resolve(header, item, qualified_tags, simple_tags):
    """Set item.tag or item.problem."""
    docs = item.docs + item.trailing
    if INHERITED_DOC_RE.search(" ".join(comment.text for comment in docs)):
        return
    covering = None
    scope = item.scope
    while scope and covering is None:
        covering = qualified_tags.get(scope)
        scope = scope.rpartition("::")[0]
    if covering is None and item.type_name and length_type(item.type_name) == "always":
        covering = simple_tags.get(base_type(item.type_name))
    if not docs:
        code = header.code[header.line_starts[item.decl_lines[0] - 1]:header.line_end(item.decl_lines[1])]
        if covering:
            item.tag = covering
        elif not re.search(r"\boverride\b", code):
            item.problem = ("MISSING", "no documentation")
        return
    if item.kind in ("PARAM", "OUT", "RETURN"):
        paras = paragraphs(item.docs) + paragraphs(item.trailing)
        if item.kind == "RETURN":
            texts = [text for command, text in paras if command in ("return", "returns", "result")]
            what = "@return"
        else:
            texts = param_texts(paras, item.name)
            what = f"@param {item.name}"
        if not texts:
            # A declaration documented only by its brief carries the tag right after @brief.
            text = own_text(paragraphs(item.trailing) or paragraphs(last_block(item.docs)))
            if text is not None:
                texts = [text]
                what = f"{what} or brief"
    else:
        text = own_text(paragraphs(item.trailing)) if item.trailing else None
        if text is None and item.docs:
            text = own_text(paragraphs(last_block(item.docs)))
        texts = [text] if text is not None else []
        what = "description"
    if not texts:
        if covering:
            item.tag = covering
        else:
            item.problem = ("MISSING", f"no {what} paragraph")
        return
    tags = set()
    for text in texts:
        tag, problem = tag_of(text)
        if problem and not (problem == "MISSING" and covering):
            item.problem = (problem, f"{what}: {MESSAGES[problem]}")
            return
        if tag:
            tags.add(tag)
    if len(tags) > 1:
        item.problem = ("MULTIPLE", f"{what}: different tags on its overloads")
        return
    tag = tags.pop() if tags else covering
    if covering and tag != covering:
        item.problem = ("CLASS_CONFLICT", f"{what}: @UNIT_{tag} differs from the class tag @UNIT_{covering}")
        return
    item.tag = tag


def check_definitions(root):
    """Return a violation for each definition of DEFINITIONS that a file of DEFINITION_FILES lacks.

    A header must put each definition right after its tag token.
    """
    violations = []
    for path in DEFINITION_FILES:
        try:
            text = (root / path).read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as error:
            raise CheckError(f"{path}: cannot read: {error}")
        words = " ".join(re.sub(r"(?m)^[ \t]*(?:\*|//+)", "", text).split())
        for tag, definition in DEFINITIONS.items():
            expected = f"@UNIT_{tag} {definition}" if path.endswith(".h") else definition
            if expected not in words:
                violations.append((path, 0, "DEFINITION", f"@UNIT_{tag}",
                                   "the definition is missing or differs from DEFINITIONS in the script", set()))
    return violations


def check(root, paths, changed=None):
    """Return the sorted violations [(path, line, kind, key, message)]."""
    headers = []
    for path in paths:
        header = Header(root, path)
        header.parse()
        headers.append(header)
    qualified_tags, simple_tags, violations = class_tags_of(headers)
    allowed = {(path, key) for path, key, _ in ALLOWLIST}
    used = set()
    pairs = {}
    for header in headers:
        for comment in header.comments:
            for word in TAG_RE.findall(comment.text):
                if comment.doc and word not in VALID_TAGS:
                    violations.append((header.path, comment.line, "UNKNOWN", f"@UNIT_{word}", "unknown tag",
                                       set(range(comment.line, comment.end_line + 1))))
        for item in header.items:
            if (item.path, item.key) in allowed:
                used.add((item.path, item.key))
                continue
            resolve(header, item, qualified_tags, simple_tags)
            if item.problem:
                violations.append((item.path, item.line, item.problem[0], item.key, item.problem[1], item.lines()))
                continue
            match = re.match(r"(Set|Get)(\w+)$", item.function or "")
            if match and item.tag:
                if match.group(1) == "Set" and item.kind == "PARAM":
                    pairs.setdefault((item.path, item.scope, match.group(2)), []).append(("set", item))
                elif match.group(1) == "Get" and item.kind in ("RETURN", "OUT"):
                    pairs.setdefault((item.path, item.scope, match.group(2)), []).append(("get", item))
    for (path, scope, name), entries in pairs.items():
        overloads = {}
        for role, item in entries:
            if role == "set":
                overloads.setdefault(item.decl_lines, []).append(item)
        setters = [items[0] for items in overloads.values() if len(items) == 1]
        getters = [item for role, item in entries if role == "get"]
        if not setters or not getters or len({item.tag for item in setters + getters}) == 1:
            continue
        setter = setters[0]
        getter = next((item for item in getters if item.tag != setter.tag), getters[0])
        lines = set().union(*(item.lines() for item in setters + getters))
        violations.append((path, setter.line, "PAIR", "::".join(filter(None, [scope, f"Set{name}"])),
                           f"@UNIT_{setter.tag} differs from Get{name} (@UNIT_{getter.tag}, line {getter.line})",
                           lines))
    if changed is None:
        for path, key, _ in ALLOWLIST:
            if (path, key) not in used:
                violations.append((path, 0, "STALE_ALLOWLIST", key, "the allowlist entry matches nothing", set()))
        violations.extend(check_definitions(root))
    else:
        violations = [violation for violation in violations if violation[0] in changed
                      and (changed[violation[0]] is True or violation[5] & changed[violation[0]])]
    return sorted({violation[:5] for violation in violations})


# ------------------------------------------------------------------------------------------------ command line


def git(root, *args):
    try:
        result = subprocess.run(["git", "-C", str(root), *args], capture_output=True, text=True, check=True)
    except (OSError, subprocess.CalledProcessError) as error:
        detail = getattr(error, "stderr", None) or str(error)
        raise CheckError(f"git {' '.join(args)} failed: {detail.strip()}")
    return result.stdout


def api_headers(root):
    paths = []
    for directory in API_DIRS:
        base = root / directory
        if not base.is_dir():
            raise CheckError(f"{directory}: no such directory under {root}")
        for path in sorted(base.rglob("*.h")):
            relative = path.relative_to(root).as_posix()
            if "/internal/" not in f"/{relative}" and not relative.endswith(".autogen.h"):
                paths.append(relative)
    return paths


def changed_lines(root, revision):
    """Return {path: set of new-side line numbers changed since revision, or True for an untracked file}."""
    changed = {}
    path = None
    for line in git(root, "diff", "--unified=0", "--no-color", "--no-ext-diff", revision, "--", *API_DIRS).split("\n"):
        if line.startswith("+++ "):
            target = line[4:].strip()
            path = target[2:] if target.startswith("b/") else None
            if path:
                changed.setdefault(path, set())
        elif line.startswith("@@") and path:
            match = re.match(r"@@ -\S+ \+(\d+)(?:,(\d+))? @@", line)
            if match:
                start = int(match.group(1))
                count = int(match.group(2) or 1)
                # A deletion-only hunk (+start,0) touches the lines on both sides of the deletion point.
                changed[path].update(range(start, start + count) if count else (start, start + 1))
    for path in git(root, "ls-files", "--others", "--exclude-standard", "--", *API_DIRS).split("\n"):
        if path.endswith(".h"):
            changed[path] = True
    return changed


def self_test():
    """Reproduce the two --diff cases that must work; return the failures."""
    import tempfile

    view_text = """namespace Dali { namespace Ui {
class View
{
public:
  /**
   * @param[in] width @UNIT_LOGICAL The minimum width
   */
  void SetMinimumWidth(float width);
};
}}
"""
    types_text = """namespace Dali { namespace Ui {
/**
 * @brief The result of a measure.
 *
 * @UNIT_PHYSICAL Applies to every length in this class.
 */
class MeasuredSize { public: float width; float height; };
}}
"""
    failures = []
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        for api in API_DIRS:
            (root / api).mkdir(parents=True)
        (root / "dali-ui-foundation/public-api/types.h").write_text(types_text)
        view = root / "dali-ui-foundation/public-api/view.h"
        view.write_text(view_text)
        git(root, "init", "-q")
        git(root, "add", "-A")
        git(root, "-c", "user.name=self-test", "-c", "user.email=self-test@localhost", "-c", "commit.gpgsign=false",
            "-c", "core.hooksPath=/dev/null", "commit", "-q", "-m", "base")
        # 1. A use of a class-tagged type added to another header is covered by the tag of the unchanged header.
        text = view_text.replace("  void SetMinimumWidth(float width);\n",
                                 "  void SetMinimumWidth(float width);\n\n  /**\n   * @return The previous result\n"
                                 "   */\n  MeasuredSize GetPreviousMeasuredSize() const;\n")
        view.write_text(text)
        status, output = run_quietly(["--diff", "HEAD", "--root", str(root), "--quiet"])
        if status != 0 or output:
            failures.append(f"class tag of an unchanged header not applied: exit {status}, {output!r}")
        # 2. Deleting a tagged line is reported, although the hunk adds no line.
        view.write_text(text.replace("   * @param[in] width @UNIT_LOGICAL The minimum width\n", ""))
        status, output = run_quietly(["--diff", "HEAD", "--root", str(root), "--quiet"])
        if status != 1 or "MISSING: View::SetMinimumWidth(width)" not in output:
            failures.append(f"deleted tag not reported: exit {status}, {output!r}")
    return failures


def run_quietly(argv):
    """Run main() and return (exit status, standard output)."""
    import contextlib
    import io

    buffer = io.StringIO()
    with contextlib.redirect_stdout(buffer):
        status = main(argv)
    return status, buffer.getvalue()


def main(argv=None):
    parser = argparse.ArgumentParser(description="Check the @UNIT_LOGICAL and @UNIT_PHYSICAL tags of the API headers.")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--all", action="store_true", help="check all API headers, the allowlist and the tag definitions")
    mode.add_argument("--diff", metavar="REV", help="check only the items whose lines changed since REV")
    mode.add_argument("--self-test", action="store_true", help="check that --diff handles class tags and deletions")
    parser.add_argument("--root", help="repository root (default: the top level of the current git checkout)")
    parser.add_argument("--quiet", action="store_true", help="do not print the violation count")
    try:
        args = parser.parse_args(argv)
    except SystemExit as error:
        return 2 if error.code else 0
    for path, key, reason in ALLOWLIST:
        assert reason.strip(), f"allowlist entry without a reason: {path} {key}"
    try:
        if args.self_test:
            failures = self_test()
            for failure in failures:
                print(f"self-test failed: {failure}", file=sys.stderr)
            if not args.quiet:
                print("self-test " + ("failed" if failures else "passed"), file=sys.stderr)
            return 1 if failures else 0
        root = Path(args.root) if args.root else Path(git(Path.cwd(), "rev-parse", "--show-toplevel").strip())
        # Every header is parsed, also in --diff mode, because a class tag in an unchanged header covers the uses of its
        # type in a changed one; only the report is limited to the changed lines.
        paths = api_headers(root)
        changed = changed_lines(root, args.diff) if args.diff else None
        violations = check(root, paths, changed)
    except CheckError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    for path, line, kind, key, message in violations:
        print(f"{path}:{line}: {kind}: {key}: {message}")
    if not args.quiet:
        print(f"{len(violations)} violation(s)", file=sys.stderr)
    return 1 if violations else 0


if __name__ == "__main__":
    sys.exit(main())
