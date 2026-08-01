#include "my-view-impl.h"
#include <dali/devel-api/object/type-registry-helper.h>

using namespace Dali;
using namespace Dali::Ui;

namespace MyViewSample
{

namespace
{
// For type Registration
BaseHandle Create()
{
  return MyView::New();
}

// Type Registration
DALI_TYPE_REGISTRATION_BEGIN(MyViewImpl, ViewImpl, Create)
DALI_TYPE_REGISTRATION_END()

} // namespace

IntrusivePtr<MyViewImpl> MyViewImpl::New()
{
  IntrusivePtr<MyViewImpl> impl(new MyViewImpl());

  // Arrange 캐시 opt-in. 기본값은 ArrangePurity::IMPURE 이므로, 선언하지 않으면
  // 이 View 의 arrange producer 는 매 pass 마다 다시 실행됩니다(항상 정확하지만
  // 느립니다). 이 클래스는 OnArrange 를 재정의하지 않으므로 producer 는
  // ViewImpl::OnArrange -> 기본 배치이고, 그것은 arrange bounds / padding / margin /
  // 자식의 measured size / effective scale 만 읽으므로 PURE 로 선언할 수 있습니다.
  //
  // 반드시 생성자가 아니라 이 New() 에서 선언해야 합니다. 여기서 만들어지는 객체의
  // 최종 타입이 MyViewImpl 로 확정되어 있기 때문입니다. 생성자에서 선언하면 그
  // 선언이 모든 파생 클래스로 새어 나가, 검증되지 않은 파생 OnArrange 가 캐시로
  // 대체되어 버립니다.
  //
  // OnArrange 를 재정의하면서 그 안에서 조상/월드 좌표(SCREEN_POSITION 등)를 읽거나
  // actor 트리 밖의 표면에 상태를 밀어 넣는다면 이 선언을 하면 안 됩니다.
  impl->SetArrangePurity(ArrangePurity::PURE);

  return impl;
}

MyViewImpl::MyViewImpl()
: ViewImpl()
{
  // [NOTE] 이 시점에는 핸들에 접근할 수 없습니다. OnInitialize 부터 가능합니다.
}

void MyViewImpl::OnInitialize()
{
  ViewImpl::OnInitialize();

  MyView handle = MyView::DownCast(Self()); // Get handle
  handle.SetBackgroundColor(UiColor(0x00FFFF));
  handle.SetRequestedWidth(200_spx);
  handle.SetRequestedHeight(200_spx);
}

void MyViewImpl::ChangeBackground()
{
  static UiColor colors[] = {
    UiColor(0x00FFFF),
    UiColor(0xFF00FF),
    UiColor(0xFFFF00),
    UiColor(0xFF0000),
    UiColor(0x00FF00),
    UiColor(0x0000FF)};
  static const int size = sizeof(colors) / sizeof(colors[0]);

  mChangeCount = (mChangeCount + 1) % size;

  MyView handle = MyView::DownCast(Self());
  handle.SetBackgroundColor(colors[mChangeCount]);
}
} //namespace MyViewSample
