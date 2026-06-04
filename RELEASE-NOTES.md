## LBX-GL v0.1.70 (2026-06-04)

* 새 기능: Linux 배포 환경에서 라이브러리가 자동으로 자신의 버전 정보를 포함하도록 지원 추가됨
* 개선: lbx-gl 프로젝트의 빌드 리소스 구조가 개선되어 버전을 관리하기 쉬워짐
* 호환성·주의사항: 모듈 내 헤더 파일 및 빌드 의존성이 표준화되어 변경 사항에 대한 주의 필요

### Dependencies (빌드 시 의존 라이브러리 버전)
- lib/lbx: 29618a95-dirty

## LBX-GL v0.1.69 (2026-06-01)

- OpenGL ES 호환성 강화 및 버전 플래그 (LBX_GLES_VERSION) 를 통한 렌더러 전환 기능 추가
- TGlyph 의 SetSourceRegion 을 SetBoundary 로 원복하고 외부에서 fit/border 상태 판단 가능하도록 함수 추가
- 동적 Vertex Scaling 개선, 좌표 보정 로직 최적화 및 외부 셰이더 핸들 지정 기능 제공
- TGlyph 의 이미지 처리 기능 향상 및 렌더링 관련 구조화 정의 도입

### Dependencies (빌드 시 의존 라이브러리 버전)
- lib/lbx: 74f43b94-dirty
