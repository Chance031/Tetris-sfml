# Tetris-sfml
SFML로 다시 만드는 C++ 테트리스 프로젝트입니다.

## 기술 스택
[![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=cplusplus&logoColor=white)](https://img.shields.io/badge/C++-00599C?style=flat&logo=cplusplus&logoColor=white)
[![SFML](https://img.shields.io/badge/SFML-8CC445?style=flat&logo=sfml&logoColor=white)](https://img.shields.io/badge/SFML-8CC445?style=flat&logo=sfml&logoColor=white)

## 사용 AI
[![Claude](https://img.shields.io/badge/Claude-D97757?style=flat&logo=anthropic&logoColor=white)](https://img.shields.io/badge/Claude-D97757?style=flat&logo=anthropic&logoColor=white)
[![ChatGPT](https://img.shields.io/badge/ChatGPT-74aa9c?style=flat&logo=openai&logoColor=white)](https://img.shields.io/badge/ChatGPT-74aa9c?style=flat&logo=openai&logoColor=white)

---

## 플레이 가이드

### 환경
- Windows
- Visual Studio 2022
- MSVC toolset `v145`
- C++20
- SFML 3.0.2

### 빌드
기본 기준은 저장소 루트의 `SFML-3.0.2` 폴더입니다.

```
Tetris-sfml/
  SFML-3.0.2/
    include/
    lib/
    bin/
  Tetris/
```

프로젝트는 위 로컬 폴더를 기본으로 사용하고, 필요하면 `SFML_DIR` 환경변수로 덮어쓸 수 있습니다.

예시:
```
$env:SFML_DIR="D:\libs\SFML-3.0.2"
```

빌드 후 필요한 DLL은 프로젝트 설정에서 출력 폴더로 자동 복사되도록 맞춰두었습니다.

### 조작 키
| 키 | 동작 |
|---|---|
| `←` `→` | 블록 좌우 이동 |
| `↓` | 소프트 드롭 (1점/칸) |
| `Space` | 하드 드롭 (2점/칸) |
| `Z` | 시계 방향 회전 |
| `X` | 반시계 방향 회전 |
| `C` | 홀드 |
| `P` | 일시정지 / 재개 |
| `R` | 재시작 (게임오버 후) |
| `Q` / `Esc` | 종료 |

### 점수 체계
| 액션 | 점수 |
|---|---|
| Single | 100 × 레벨 |
| Double | 300 × 레벨 |
| Triple | 500 × 레벨 |
| Tetris | 800 × 레벨 |
| T-Spin Single | 800 × 레벨 |
| T-Spin Double | 1200 × 레벨 |
| T-Spin Triple | 1600 × 레벨 |
| Back-to-Back 보너스 | 위 점수의 +50% |
| 콤보 보너스 | 50 × 콤보 수 × 레벨 |

---

## 관련 프로젝트

| 레포 | 설명 |
|---|---|
| [Tetris-cpp](https://github.com/Chance031/Tetris-cpp) | 콘솔 C++ 버전 |
