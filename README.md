# Tetris-sfml

SFML로 다시 만드는 C++ 테트리스 프로젝트입니다.

## 현재 구성

- Visual Studio C++20 프로젝트
- SFML 기반 렌더링/윈도우 초기 세팅
- 바로 실행 가능한 최소 `main.cpp`

## 폴더 구조

```text
Tetris/
  Tetris/
    src/
      main.cpp
    Tetris.vcxproj
```

## 개발 환경

- Visual Studio 2022
- MSVC toolset `v145`
- C++20
- SFML 3.0.2

## SFML 연결 방법

기본 기준은 저장소 루트의 `SFML-3.0.2` 폴더입니다.

```text
Tetris-sfml/
  SFML-3.0.2/
    include/
    lib/
    bin/
  Tetris/
```

프로젝트는 위 로컬 폴더를 기본으로 사용하고, 필요하면 `SFML_DIR` 환경변수로 덮어쓸 수 있습니다.

예시:

```powershell
$env:SFML_DIR="D:\libs\SFML-3.0.2"
```

## 실행 전 체크

1. 저장소 루트에 `SFML-3.0.2` 폴더가 있는지 확인합니다.
2. 다른 경로를 쓸 경우에만 `SFML_DIR`를 설정합니다.
3. 빌드 후 필요한 DLL은 프로젝트 설정에서 출력 폴더로 자동 복사되도록 맞춰두었습니다.

## 다음 작업 추천

- 게임 보드/블록 데이터 구조 추가
- 입력 처리와 중력 업데이트 루프 분리
- 텍스처, 폰트, 사운드 리소스 폴더 구성
