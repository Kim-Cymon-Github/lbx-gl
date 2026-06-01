## LBX-GL v0.1.69 (2026-06-01)

- OpenGL ES 호환성 강화 및 버전 플래그 (LBX_GLES_VERSION) 를 통한 렌더러 전환 기능 추가
- TGlyph 의 SetSourceRegion 을 SetBoundary 로 원복하고 외부에서 fit/border 상태 판단 가능하도록 함수 추가
- 동적 Vertex Scaling 개선, 좌표 보정 로직 최적화 및 외부 셰이더 핸들 지정 기능 제공
- TGlyph 의 이미지 처리 기능 향상 및 렌더링 관련 구조화 정의 도입

### Dependencies (빌드 시 의존 라이브러리 버전)
- lib/lbx: 74f43b94-dirty
