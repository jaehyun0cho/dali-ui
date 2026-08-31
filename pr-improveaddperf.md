### Summary

`View::New` / `View::Add`의 뷰당 상수 비용을 제거해 생성·추가 성능을
개선합니다(비교군: toolkit `Control.Add`). Layout 결과물, scheduling,
애플리케이션 사용 방식은 변경되지 않으며, C++ setter fast path는
dali-core의 property 경로와 동일한 계약(객체 pin, event-thread assert,
`OnPropertySet` → `PropertySetSignal` tail)을 유지합니다. per-View
object 크기 증가는 0 byte이고(`sizeof(ViewDataImpl)` 312B 실측 불변),
leaf 뷰는 signal 연결 지연으로 heap이 오히려 감소합니다.

### Changes

- `OnChildAdded`의 transition 부기를 `HasAnyInstance()` 전역 카운터로
  게이트(transition 없는 앱은 Window/controller/dispatcher 왕복 생략)
- 뷰당 무필터 debug log 제거; 부모 조회를 `GetParentView()` 하나로 통일
  (`GetParentLayout()` 사장·삭제)
- `SetRequestedWidth/Height`: property 시스템 왕복 대신 직접 경로 —
  단 pin·assert·`PropertySetSignal` tail은 core와 등가 유지
- property callback: 두 번째 handle 생성 제거(release `dynamic_cast`
  검증은 유지); `ViewImpl::New`의 refcount 왕복 제거
- child-order signal을 첫 View 자식 add 시 lazy 연결
- 샘플 root-mode 토글(키 0, 평균 리셋 포함) + UTC 12건 추가

### Examples

```cpp
// 사용 방식은 그대로입니다. 아래 오버라이드가 setter 도중 마지막 외부
// handle을 놓아도 core와 동일한 pin이 쓰기 전체를 보호합니다.
void MyViewImpl::OnPropertySet(Property::Index index, const Property::Value& value) override
{
  if(index == Ui::View::Property::REQUESTED_WIDTH)
  {
    gMyLastHandle.Reset(); // 마지막 handle 해제 — UAF 없이 쓰기가 완료됨
  }
  ViewImpl::OnPropertySet(index, value);
}
```
