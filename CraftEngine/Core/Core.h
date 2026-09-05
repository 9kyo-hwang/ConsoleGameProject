#pragma once

/*
warning C4251: 'Craft::Actor::owner': 'std::weak_ptr<Craft::Level>'에서는 'Craft::Actor'의 클라이언트에서 DLL 인터페이스를 사용하도록 지정해야 함
동적 라이브러리(DLL)에서 작성된 '템플릿'은 DLL 외부로 넘어가면 위험함을 알려주는 MSVC 경고 메시지
이는 해결이 불가능한 사항이라 #pragma로 경고 메시지를 안뜨도록 조치
*/
#pragma warning(disable: 4251)

#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)

/*
* Engine 프로젝트는 해당 매크로를 사용(속성 -> C/C++ -> 전처리기 -> 전처리기 정의): EXPORT
* 그 외에 컨텐츠 단에는: IMPORT
* 이제부터 CraftEngine 빌드 결과물은 동적 라이브러리(dll)
*/

#if defined(ENGINE_BUILD_DLL)
#define CRAFT_API DLLEXPORT
#else
#define CRAFT_API DLLIMPORT
#endif