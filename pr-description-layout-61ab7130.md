### Summary

측정 제약이 작게 바뀌었을 때 이전 줄바꿈 결과를 재사용하던 문제를 정확 비교로 수정합니다.
기본 View의 MATCH_PARENT 배치에 기존 측정 산술을 공유해, 같은 예산을 서로 다른 식으로 계산하면서 발생하던 반복 미스를 줄입니다.
View 데이터 멤버를 추가하지 않고 Stack·Flex의 임시 저장 비용과 Recycler extent 벡터의 순차 확장 비용을 줄입니다.

### Changes

- 정규화·min/max 적용 후의 측정 제약 두 값을 `==`로 비교합니다. valid/dirty/poison, 스케일 키, 무효화 및 결과 게시 계약은 유지합니다.
- 기본 View의 기존 cold Measure 식을 helper로 공유합니다. Arrange는 실제 슬롯을 사용하며 부모의 요청 크기·min/max를 다시 적용하지 않습니다. WRAP/fixed 축과 MP의 반대 축 슬롯 정책은 유지합니다.
- 유한한 양수 스케일, 유한한 0 이상 inset 및 유한한 중간값에서 산술을 공유하고, 나머지는 기존 Arrange 뺄셈을 유지합니다.
- Stack의 읽히지 않는 `workingSizes` 버퍼를 제거하고 FlexLine을 복사 대신 이동합니다.
- Recycler의 기존 extent 벡터를 순차 확장할 때 약 1.5배 용량을 예약합니다. 희소 첫 접근, 재측정 통계, reset 후 용량 재사용을 유지하고 인덱스·할당 크기 overflow를 검사합니다.
- 공개 UTC 12개·내부 UTC 4개를 추가하고 영문·국문 Layout 문서의 캐시 계약과 비용 설명을 수정합니다.

### Examples

스케일 1.0, ROW/WRAP Flex, 50×20 자식 두 개에서 폭을 `100` → `99.99951171875`로 줄이면 결과는 `50×40`이어야 합니다. 기존 캐시의 warm 결과는 cold 결과와 달랐고, 정확 비교 단독안과 최종안에서는 일치했습니다.

폭 100, 좌우 padding 3, 스케일 1.1의 기본 View와 MP 자식을 동일한 UTC로 계측했습니다. 표는 자식의 **측정/배치 producer 실행 횟수**이며, 캐시 hit 시의 배치 replay 작업까지 0이라는 뜻은 아닙니다.

| 상황 | 기존 코드 | 정확 비교만 | 정확 비교 + 산술 공유 |
|---|---:|---:|---:|
| 자식 자체 무효화 후 한 pass | 1/1 | 2/1 | 1/1 |
| 형제만 변경한 한 pass | 0/0 | 2/1 | 0/0 |
| 형제만 변경한 16 pass 합계 | 0/0 | 32/16 | 0/0 |
| 입력·상태 변화 없는 pass | 0/0 | 0/0 | 0/0 |

부모 producer도 각 변경 pass에서 실제로 실행되는지 함께 검사했습니다. 해당 예시의 MP 배치 폭은 `103.39999389648438`에서 기존 cold Measure와 같은 `103.4000015258789`로 바뀝니다.

### Validation

- 기준 revision: `1f794aa393ecd1016df31f672e89d0ceff08706e`. 최종 commit: `61ab71308927271516f7d17c6281bce1fa980c18`.
- Ubuntu 25.10, GCC 15.2.0, CMake 3.31.6, clang-format 20.1.8에서 검증했습니다. 테스트에 필요한 Core Debug/export 심볼을 맞춘 뒤 UI를 Debug/coverage 설정으로 빌드·설치했습니다.
- 별도 worktree에서 최종 제품 파일 6개의 내용이 checkout과 같은지 SHA-256으로 대조했습니다. 해당 cpp 재컴파일과 설치 라이브러리의 새 helper 포함을 확인했습니다.
- 공개 모듈 빌드 성공. 내부 모듈은 아래의 기존 경고 예외를 적용한 뒤 빌드 성공. 새 테스트의 경고 설정은 완화하지 않았습니다.

| 선택한 회귀 범위 | 실행 | 실패 |
|---|---:|---:|
| 공개: View, ViewLayoutBoundary, LayoutConstraintCache, LayoutTypes, Layout, LayoutController, LayoutTransition, StackLayout, FlexLayout, GridLayout, AbsoluteLayout, ScrollView, RecyclerView, GroupAdapter | 825 | 0 |
| 내부: LinearItemsLayouterStorage, ArrangeCacheHit, LayoutDependencyScope, LayoutInvalidationGeneration, LayoutDirectionArrangeCache, UiScaleEffectiveScale | 77 | 0 |
| 합계 | 902 | 0 |

추가한 16개 UTC는 위 합계에 포함됩니다. 경계 줄바꿈, 깨끗한 형제 재사용, 폭·높이 및 Arrange-only 변경, clamp된 부모의 실제 슬롯, 분수·음수·NaN·Inf inset, 정규화 overflow, 스케일 왕복, 고정 폭 자손 재사용을 검사했습니다. Recycler에서는 4096개 순차 기록의 용량 증가·복사량 상한, 희소 기록, 재측정 통계, reset과 overflow를 검사했습니다.

주요 빌드 명령은 다음과 같습니다. 환경은 `. /home/jae/setenv`, 설치 prefix는 `/home/jae/dali/dali-core/dali-env/opt`를 사용했습니다.

```sh
# 최종 제품 소스와 내용이 같은 worktree의 build/tizen에서 실행
CXXFLAGS='-g -O0 --coverage' LDFLAGS='--coverage' cmake -DCMAKE_INSTALL_PREFIX="$DESKTOP_PREFIX" -DCMAKE_BUILD_TYPE=Debug -DENABLE_GRAPHICS_BACKEND=GLES .
make install -j7

# checkout의 automated-tests에서 실행
./build.sh -r dali-ui-foundation
./build.sh -r dali-ui-foundation-internal
# 아래의 기존 두 UTC 오브젝트만 경고 예외로 컴파일한 뒤 이어서 실행
cmake --build build -j7
```

내부 빌드의 기존 `utc-Dali-WindowsWarningCoverage-internal.cpp`와 `utc-Dali-ViewFittingMode-internal.cpp`는 변경되지 않은 TextVisual의 `GetVisualObject` 이름 숨김 때문에 `-Werror=overloaded-virtual`에서 실패했습니다. 생성된 `build.make`로 이 두 오브젝트만 기존 `CXX_FLAGS`에 `-Wno-error=overloaded-virtual`을 추가해 컴파일했습니다. 저장소의 경고 정책이나 해당 기존 소스는 수정하지 않았습니다.

UTC는 `dbus-run-session` 환경에서 `build/src/<module>/tct-<module>-core <testcase>`를 선택 범위의 각 testcase에 대해 별도 프로세스로 실행하고 종료 코드를 기록했습니다. 수정한 12개 파일의 해시는 검증 전후 및 commit hook 이후에도 같았으며, `git diff --check`와 clang-format 검사도 통과했습니다.

### Limits

- 계측은 위 fixture의 producer 횟수입니다. 전체 프레임 시간 개선이나 모든 트리에서의 동일한 효과를 보장하지 않습니다. Arrange 사전 계산·유효성 검사 비용도 있으므로 제품 workload의 Release 성능 측정은 별도로 필요합니다.
- 산술 공유는 MP 배치의 반올림 결과를 바꿉니다. 실제 슬롯·요청 크기·clamp·정규화 경로가 다르면 이후에도 키 미스가 생기며, 자손 재계산은 유효한 캐시가 있는 지점에서 멈춥니다.
- 일반 View에 새 데이터 멤버는 없습니다. Recycler의 기존 벡터에는 용량 여유분이 늘 수 있고, reset 후 기존 용량을 보존하므로 항상 현재 항목 수 이하라고 보장하지 않습니다.
- setter·검증의 0.001 비교, Flex 줄바꿈 경계, 전이의 0.5px 임계값은 변경 대상에 포함하지 않았습니다.
- 관련 foundation 회귀 범위로 검증했습니다. 전체 모듈의 모든 UTC, GUI 수동 확인, samples 및 32비트 실행 검증은 수행하지 않았습니다. 변형 빌드 간 기존 gcda checksum 경고가 있어 coverage 비율은 근거로 사용하지 않았습니다.
