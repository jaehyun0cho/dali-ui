### Summary

`View` 생성/추가 상수 비용 절감 커밋(upstream 반영분)의 리뷰에서 도출된
후속 최적화 3건과, 효과를 직접 측정할 수 있는 벤치마크 키를 추가합니다.
레이아웃 결과물·스케줄링·앱 사용 방식은 그대로입니다: 각 최적화는
"증명 가능한 no-op 경로 생략" 또는 "지연 할당"이며, 관찰 가능한 응답
(null 시 반환값, corner latch 타이밍, resource status, relayout 요청,
processor 등록 타이밍)은 전부 기존과 동일하게 보존했습니다.
후보였던 `RemoveAll`의 O(n²) 완화는 저렴한 방법이 모두 제거 순서를
바꿔 보류했고, 측정 키만 추가했습니다. 커밋 4개는 독립적으로 revert
가능합니다.

### Changes

- **Remove 게이트** (`65faf1cc`): `LayoutTransition`이 전무하면 `View::Remove`의
  `Window::Get`+자식 탐색+상속 EXIT resolver 워크를 생략(add 게이트의 대칭)
- **VisualData 지연 생성** (`f5957040`): 모든 기본 View가 `Initialize`에서 하던
  빈 `VisualData` heap 할당을 첫 visual 사용 시점(`EnsureVisualData`)으로 이동
- **known-parent invalidation** (`25f0b105`): `OnChildAdded`가 이미 쥔 부모를
  `Actor::GetParent`+`View::DownCast`로 재발견하던 것 제거
- **벤치마크 키** (`db496c28`): `R`/`A`(제거 10000 개별/일괄), `N`(New 단독),
  `T`(미부착 transition 토글=게이트 대조실험); 결과 라인에 root/transition 표기
- UTC 5건 추가(빌드 검증); upstream 키 7/8·standalone 평균과 무충돌 병합

### Examples

```
# 샘플(view-creation-perf) 실행 중:
r        # Remove 10000 — gate-off (transitions=none)
t        # dormant LayoutTransition 생성 → HasAnyInstance()=true
r        # Remove 10000 — gate-on 과 비교 (두 게이트 모두 동일 방식)
n        # View::New only 10000 — VisualData 지연 생성 효과
```
