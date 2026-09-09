# wdk59 변경 부분 주석 위치

범위: `afe7cc2` (feat: 모닥불 구현, 에셋 이주) 포함부터 현재 `52e0580`까지. 기준 커밋 작성자 `wdk59 <dookong59@gmail.com>`의 변경 중 현재 남아 있는 코드에 설명을 추가했습니다.

코드에서 `[wdk59]`를 검색하면 추가한 주석을 찾을 수 있습니다. 기존 Campfire.h 작업 내용은 유지했습니다.

## 코드 위치

### Source/R1/Private/BuildingSystem/Component/BuildingPlacementComponent.cpp

- [609행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/BuildingSystem/Component/BuildingPlacementComponent.cpp:609>): 모닥불 설치와 아이템 소비가 성공한 지점에서 설치 사운드를 알린다.

### Source/R1/Private/Character/ActionCharacter.cpp

- [137행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Character/ActionCharacter.cpp:137>): 기본 아이템 디버그 지급 중 인벤토리 누락은 중단하고 비어 있는 아이템 항목은 건너뛰어 크래시를 방지한다.

### Source/R1/Private/Character/ActionPlayerController.cpp

- [44행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Character/ActionPlayerController.cpp:44>): 모닥불 이동 및 점화 RPC에서 현재 폰과 상호작용 가능 거리를 공통으로 검사한다.
- [59행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Character/ActionPlayerController.cpp:59>): 소유권이 있는 플레이어 컨트롤러를 통해 공유 모닥불의 서버 재고 변경을 요청한다.
- [522행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Character/ActionPlayerController.cpp:522>): Q 키로 개인 제작 화면을 토글하며 작업대 없는 제작은 nullptr로 구분한다.
- [545행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Character/ActionPlayerController.cpp:545>): 개인 제작 컴포넌트와 선택 작업대를 위젯에 연결하고 제작 화면에 입력 포커스를 준다.

### Source/R1/Private/Component/CraftingComponent.cpp

- [69행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Component/CraftingComponent.cpp:69>): 중복 재료를 합산하고 잘못된 수량과 제작 시간을 걸러 최대 제작량 및 서버 차감에 같은 비용을 사용한다.
- [116행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Component/CraftingComponent.cpp:116>): 서버가 레시피, 재료, 작업대 거리와 큐 여유를 재검사한 후 재료를 선불 차감하고 주문을 등록한다.
- [176행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Component/CraftingComponent.cpp:176>): 개인 제작은 지급 가능할 때까지 기다리고, 작업대 제작은 즉시 지급하지 못한 결과를 회수 대기로 보관한다.
- [232행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Component/CraftingComponent.cpp:232>): 작업대 파괴 시 완료 아이템과 아직 제작하지 않은 주문의 재료를 월드에 떨어뜨린다.

### Source/R1/Private/Component/InteractionComponent.cpp

- [22행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Component/InteractionComponent.cpp:22>): 현재 UI로 사용 중인 모닥불을 공유하여 벨트 우클릭도 같은 모닥불로 이동하게 한다.

### Source/R1/Private/Item/PlaceableItem/Campfire/Campfire.cpp

- [48행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/Campfire.cpp:48>): 서버가 상호작용 거리를 확인한 뒤 요청한 플레이어에게 모닥불 UI를 연다.
- [58행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/Campfire.cpp:58>): 복제된 점화 상태의 전환에 맞춰 반복 연소음과 소화음을 재생한다.
- [83행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/Campfire.cpp:83>): 설치 성공 알림을 한 번만 멀티캐스트하여 주변 클라이언트에 설치음을 전달한다.

### Source/R1/Private/Item/PlaceableItem/Campfire/CampfireComponent.cpp

- [82행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/CampfireComponent.cpp:82>): 연료 부산물과 요리 결과가 출력 슬롯 두 칸을 공유하며, 같은 아이템의 기존 스택을 우선 채운다.
- [121행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/CampfireComponent.cpp:121>): 연료는 연소 시작 시 한 개 차감하고, 남은 연소 시간과 부산물을 슬롯 재고와 별도로 보관한다.
- [150행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/CampfireComponent.cpp:150>): 서버에서 실제 연소한 시간만큼 요리를 진행하고, 결과물 추가에 성공한 뒤 입력 재료를 차감한다.
- [218행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/CampfireComponent.cpp:218>): 허용 레시피와 스택 여유를 검사하고 지정 수량 또는 빈 슬롯 대상 절반 분할을 서버에서 처리한다.
- [249행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/CampfireComponent.cpp:249>): 모닥불 재고를 인벤토리로 회수하며 장비 슬롯은 거절하고 SetSlot을 통해 변경 알림을 유지한다.
- [280행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/CampfireComponent.cpp:280>): 우클릭한 아이템의 레시피를 기준으로 요리 입력 또는 연료 슬롯을 자동 선택한다.
- [293행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Campfire/CampfireComponent.cpp:293>): 우클릭 회수는 메인 인벤토리의 같은 아이템 스택을 먼저 채운 뒤 빈 슬롯을 찾는다.

### Source/R1/Private/Item/PlaceableItem/Workbench.cpp

- [13행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Workbench.cpp:13>): 파괴되기 전에 서버에서 제작 결과물과 남은 재료를 반환한다.
- [31행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Item/PlaceableItem/Workbench.cpp:31>): 서버의 거리 검사 후 해당 작업대의 제작 화면을 요청자에게 열어 준다.

### Source/R1/Private/Widget/Campfire/CampfireSlotWidget.cpp

- [73행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Campfire/CampfireSlotWidget.cpp:73>): 공용 드래그 데이터에 모닥불 출처, 슬롯, 아이템과 가운데 버튼 분할 여부를 담는다.
- [119행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Campfire/CampfireSlotWidget.cpp:119>): 드롭 출처와 허용 아이템을 확인하여 인벤토리 이동 요청을 전달하고 거절된 UI 드롭도 소비한다.

### Source/R1/Private/Widget/Campfire/CampfireWidget.cpp

- [23행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Campfire/CampfireWidget.cpp:23>): 이전 모닥불 구독을 해제한 뒤 새 대상의 복제 상태 변경에 슬롯과 진행 표시를 연결한다.

### Source/R1/Private/Widget/Crafting/CraftingItemWidget.cpp

- [22행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Crafting/CraftingItemWidget.cpp:22>): 레시피의 제작 가능 여부와 선택 강조를 표시하며 큐에서는 같은 타일을 수량·시간 표시로 재사용한다.

### Source/R1/Private/Widget/Crafting/CraftingMaterialWidget.cpp

- [5행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Crafting/CraftingMaterialWidget.cpp:5>): 개당 비용과 선택 수량으로 필요량을 계산하고 보유량이 부족한 재료를 색으로 구분한다.

### Source/R1/Private/Widget/CraftingWidget.cpp

- [30행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/CraftingWidget.cpp:30>): 개인 제작과 작업대 제작이 화면을 공유하되 레시피 및 큐 조회 대상은 모드별로 선택한다.
- [72행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/CraftingWidget.cpp:72>): 작업대 필요 조건과 공백·대소문자를 정규화한 검색어로 레시피 타일을 필터링한다.
- [133행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/CraftingWidget.cpp:133>): 진행 주문과 회수 대기 주문을 별도로 표시하고 서버 시간과 앞선 주문을 기준으로 남은 시간을 계산한다.
- [169행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/CraftingWidget.cpp:169>): 작업대 거리 이탈 시 화면을 닫고, 유지 중에는 0.2초 간격으로 재료와 큐 표시를 갱신한다.

### Source/R1/Private/Widget/Inventory/BeltBarWidget.cpp

- [145행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Inventory/BeltBarWidget.cpp:145>): 열린 모닥불이 있으면 벨트 우클릭으로 해당 모닥불에 빠르게 넣는다.
- [175행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Inventory/BeltBarWidget.cpp:175>): 모닥불에서 벨트로 드롭한 아이템을 컨트롤러의 서버 이동 RPC로 전달한다.

### Source/R1/Private/Widget/Inventory/DetailInfoWidget.cpp

- [405행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Inventory/DetailInfoWidget.cpp:405>): 상세창의 분할 드래그에도 출처와 아이템을 넣어 모닥불이 허용 여부와 지정 수량을 처리하게 한다.

### Source/R1/Private/Widget/Inventory/InventorySlotWidget.cpp

- [291행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Inventory/InventorySlotWidget.cpp:291>): 모닥불 출처 드롭을 일반 인벤토리 이동과 구분하여 전용 델리게이트로 전달한다.

### Source/R1/Private/Widget/Inventory/InventoryWidget.cpp

- [239행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Inventory/InventoryWidget.cpp:239>): 모닥불 UI를 사용하는 동안 우클릭을 해당 모닥불로 빠르게 옮기는 요청으로 처리한다.
- [271행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/Inventory/InventoryWidget.cpp:271>): 모닥불에서 메인 인벤토리로 드롭한 수량과 분할 옵션을 서버 RPC에 전달한다.

### Source/R1/Private/Widget/MainHUDWidget.cpp

- [80행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/MainHUDWidget.cpp:80>): 거리 이탈이나 대상 소멸 시 모닥불과 함께 열린 인벤토리도 닫고 입력 상태를 복구한다.
- [240행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/MainHUDWidget.cpp:240>): 인벤토리, 모닥불 위젯, 상호작용 컴포넌트의 대상을 맞추고 세션 시작 시 열기 소리를 재생한다.
- [269행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Private/Widget/MainHUDWidget.cpp:269>): 위젯 델리게이트와 활성 대상을 해제하고 열린 세션에 대해서만 닫기 소리를 재생한다.

### Source/R1/Public/Character/ActionPlayerController.h

- [32행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Character/ActionPlayerController.h:32>): 개인 제작 큐를 컨트롤러에 두고 작업대 제작 요청도 이 컴포넌트를 통해 서버에 전달한다.
- [50행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Character/ActionPlayerController.h:50>): 모닥불 UI 열기와 아래 서버 RPC들은 공유 액터 조작을 플레이어 소유 컨트롤러로 중계한다.

### Source/R1/Public/Component/CraftingComponent.h

- [38행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Component/CraftingComponent.h:38>): 개인 또는 작업대 소유의 제작 큐, 재료 검증 및 결과물 지급·보관을 담당한다.

### Source/R1/Public/Component/InteractionComponent.h

- [54행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Component/InteractionComponent.h:54>): 모닥불 UI 세션의 대상을 저장하고 벨트의 빠른 이동에서 조회한다.

### Source/R1/Public/Component/InventoryComponent.h

- [268행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Component/InventoryComponent.h:268>): 모닥불이 기존 SetSlot 경로로 인벤토리를 수정하여 슬롯 변경 알림과 복제를 유지하도록 접근을 허용한다.

### Source/R1/Public/Data/Campfire/CampfireConfigDataAsset.h

- [35행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Data/Campfire/CampfireConfigDataAsset.h:35>): DA_Campfire_Default에서 허용 요리·연료와 변환 결과, 처리 시간을 설정하는 데이터 형식이다.

### Source/R1/Public/Data/Item/ItemDataBase.h

- [90행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Data/Item/ItemDataBase.h:90>): 이후 제작 시스템에서 추가한 개당 제작 시간이며 큐 완료 시각 계산에 사용한다.
- [94행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Data/Item/ItemDataBase.h:94>): 작업대가 필요한 레시피를 개인 제작에서 제외하고 서버의 제작 조건 검사에도 사용한다.

### Source/R1/Public/Item/PlaceableItem/Campfire/Campfire.h

- [24행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Item/PlaceableItem/Campfire/Campfire.h:24>): PlaceableItem으로 이주한 모닥불의 설치 확정 알림이며 서버에서 설치음을 한 번 전송한다.

### Source/R1/Public/Item/PlaceableItem/Campfire/CampfireComponent.h

- [14행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Item/PlaceableItem/Campfire/CampfireComponent.h:14>): 모닥불의 서버 연소·요리, 슬롯 이동과 복제 상태를 담당하는 컴포넌트다.

### Source/R1/Public/Item/PlaceableItem/Campfire/CampfireTypes.h

- [14행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Item/PlaceableItem/Campfire/CampfireTypes.h:14>): 연료·입력·출력 종류와 인덱스로 모닥불 슬롯을 식별하여 UI와 서버 이동 요청에 공유한다.

### Source/R1/Public/Item/PlaceableItem/Workbench.h

- [10행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Item/PlaceableItem/Workbench.h:10>): 설치 가능한 작업대에 공유 제작 컴포넌트와 거리 기반 상호작용을 연결한다.

### Source/R1/Public/Widget/Campfire/CampfireSlotWidget.h

- [23행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/Campfire/CampfireSlotWidget.h:23>): 모닥불 슬롯의 아이템 표시, 허용 드롭 강조와 인벤토리 간 드래그 정보를 담당한다.

### Source/R1/Public/Widget/Campfire/CampfireWidget.h

- [15행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/Campfire/CampfireWidget.h:15>): 모닥불 슬롯과 진행도를 표시하며 사용자 조작은 컨트롤러의 서버 RPC로 전달한다.

### Source/R1/Public/Widget/Crafting/CraftingItemWidget.h

- [15행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/Crafting/CraftingItemWidget.h:15>): 레시피 선택과 제작 큐의 수량·남은 시간 표시에 공통으로 사용하는 아이템 타일이다.

### Source/R1/Public/Widget/Crafting/CraftingMaterialWidget.h

- [10행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/Crafting/CraftingMaterialWidget.h:10>): 제작 재료의 개당 비용, 총 필요량과 보유량을 표시하는 독립 행 위젯이다.

### Source/R1/Public/Widget/CraftingWidget.h

- [20행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/CraftingWidget.h:20>): 제작 화면의 검색·수량·재료·큐 표시를 조정하고 개별 타일과 재료 행은 분리된 위젯으로 구성한다.

### Source/R1/Public/Widget/Inventory/BeltBarWidget.h

- [83행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/Inventory/BeltBarWidget.h:83>): 모닥불 출처의 드롭을 받아 벨트 슬롯으로 옮기는 서버 요청을 연결한다.

### Source/R1/Public/Widget/Inventory/InventoryDragDropOperation.h

- [31행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/Inventory/InventoryDragDropOperation.h:31>): 모닥불 연동을 위해 드래그 출처와 아이템 정보를 추가하여 드롭 대상의 허용 판정에 사용한다.

### Source/R1/Public/Widget/Inventory/InventorySlotWidget.h

- [30행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/Inventory/InventorySlotWidget.h:30>): 모닥불 출처와 목적 인벤토리 슬롯, 수량 및 분할 옵션을 상위 위젯에 전달한다.

### Source/R1/Public/Widget/Inventory/InventoryWidget.h

- [62행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/Inventory/InventoryWidget.h:62>): 메인 인벤토리 우클릭의 모닥불 이동 대상을 지정하며 UI 종료 시 해제한다.

### Source/R1/Public/Widget/MainHUDWidget.h

- [96행](<C:/JSH/Unreal/TeamProj/ProjGit/Source/R1/Public/Widget/MainHUDWidget.h:96>): 열린 모닥불의 유효성과 사용 거리를 확인하여 UI 세션을 자동 종료한다.

## 에셋 및 설정 변경

바이너리 .uasset/.umap에는 텍스트 주석을 넣지 않았습니다. 아래는 동일 작성자의 범위 내 커밋에 등장한 에셋·설정 경로이며, 삭제되거나 이주한 과거 경로도 포함합니다. JSON 형식의 R1.uproject에는 주석을 추가하지 않았습니다.

- `Config/DefaultEngine.ini`
- `Content/AlphaTest/Map/M_AutoMaterial_A1.umap`
- `Content/Asset/Campfire/Icons/T_Fire.uasset`
- `Content/Asset/Campfire/Icons/T_Food_cooked.uasset`
- `Content/Asset/Campfire/Icons/T_Food_raw.uasset`
- `Content/Asset/Campfire/Icons/T_Tree_fire.uasset`
- `Content/Asset/Food/Icons/T_Cooked_Horse_Meat_Iicon.uasset`
- `Content/Asset/Food/Icons/T_CookedFish_icon.uasset`
- `Content/Asset/Food/Icons/T_Raw_Horse_Meat_icon.uasset`
- `Content/Asset/Food/Icons/T_RawFish_icon.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Materials/MI_FIshMeat_Cooked.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Materials/MI_FIshMeat_Raw.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Meshes/SM_fish_steak_LOD0_Cooked.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Meshes/SM_fish_steak_LOD0_Raw.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Textures/T_fish_meat_cooked_ao.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Textures/T_fish_meat_cooked_bc.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Textures/T_fish_meat_cooked_mg.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Textures/T_fish_meat_cooked_nrm.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Textures/T_fish_meat_raw_ao.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Textures/T_fish_meat_raw_bc.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Textures/T_fish_meat_raw_mg.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Textures/T_fish_meat_raw_nrm.uasset`
- `Content/Asset/Food/Meshes/FIshMeat/Textures/T_fish_meat_t.uasset`
- `Content/Asset/Food/Meshes/Horse/Materials/MI_Horse_Cooked.uasset`
- `Content/Asset/Food/Meshes/Horse/Materials/MI_Horse_Raw.uasset`
- `Content/Asset/Food/Meshes/Horse/Meshes/SM_horse_meat_LOD0_Cooked.uasset`
- `Content/Asset/Food/Meshes/Horse/Meshes/SM_horse_meat_LOD0_Raw.uasset`
- `Content/Asset/Food/Meshes/Horse/Textures/T_horse_meat_cooked_ao.uasset`
- `Content/Asset/Food/Meshes/Horse/Textures/T_horse_meat_cooked_bc.uasset`
- `Content/Asset/Food/Meshes/Horse/Textures/T_horse_meat_cooked_mg.uasset`
- `Content/Asset/Food/Meshes/Horse/Textures/T_horse_meat_cooked_nrm.uasset`
- `Content/Asset/Food/Meshes/Horse/Textures/T_horse_meat_raw_ao.uasset`
- `Content/Asset/Food/Meshes/Horse/Textures/T_horse_meat_raw_bc.uasset`
- `Content/Asset/Food/Meshes/Horse/Textures/T_horse_meat_raw_mg.uasset`
- `Content/Asset/Food/Meshes/Horse/Textures/T_horse_meat_raw_nrm.uasset`
- `Content/Asset/FX/Campfire/campfire-close-001.uasset`
- `Content/Asset/FX/Campfire/campfire-deploy.uasset`
- `Content/Asset/FX/Campfire/campfire-extinguish.uasset`
- `Content/Asset/FX/Campfire/campfire-loop.uasset`
- `Content/Asset/FX/Campfire/campfire-open-001.uasset`
- `Content/Asset/FX/Campfire/SA_Campfire.uasset`
- `Content/Asset/Item/CampFire/Icons/T_Fire.uasset`
- `Content/Asset/Item/CampFire/Icons/T_Food_cooked.uasset`
- `Content/Asset/Item/CampFire/Icons/T_Food_raw.uasset`
- `Content/Asset/Item/CampFire/Icons/T_Tree_fire.uasset`
- `Content/Asset/Wood/T_Wood_icon.uasset`
- `Content/Blueprint/0831_Copy/PC_0831_Copy.uasset`
- `Content/Blueprint/Campfire/BP_CampfireActor.uasset`
- `Content/Blueprint/Campfire/MM_Temp_FireMaterial.uasset`
- `Content/Blueprint/Player/PC_PlayerController.uasset`
- `Content/Blueprint/TestRespawn/BP_0831_ReSpawn.uasset`
- `Content/Blueprint/Widget/Campfire/WBP_Campfire.uasset`
- `Content/Blueprint/Widget/Campfire/WBP_CampfireSlot.uasset`
- `Content/Blueprint/Widget/Crafting/WBP_Crafting.uasset`
- `Content/Blueprint/Widget/Crafting/WBP_CraftingItem.uasset`
- `Content/Blueprint/Widget/Crafting/WBP_CraftingMaterial.uasset`
- `Content/Blueprint/Widget/Inventory/WBP_DetailInfo.uasset`
- `Content/Blueprint/Widget/Inventory/WBP_InventorySlot.uasset`
- `Content/Blueprint/Widget/Inventory/WBP_InventoryTest.uasset`
- `Content/Blueprint/Widget/WBP_InventoryTest.uasset`
- `Content/Blueprint/Widget/WBP_MainHUD_cyh.uasset`
- `Content/Blueprint/Widget/WBP_MainHUD.uasset`
- `Content/Data/Animation/Player/ABP_MainPlayer.uasset`
- `Content/Data/Assets/Building/roof_LOD0/Meshes/SM_roof_LOD0_Copy.uasset`
- `Content/Data/Assets/Building/roof_triangle_down_LOD0/Meshes/SM_roof_triangle_hip_LOD0.uasset`
- `Content/Data/Campfire/DA_Campfire_Default.uasset`
- `Content/Data/Item/ConsumableItem/DA_Item_Consumable_Bandage.uasset`
- `Content/Data/Item/ConsumableItem/DA_Item_Consumable_CookedFish.uasset`
- `Content/Data/Item/ConsumableItem/DA_Item_Consumable_CookedHorseMeat.uasset`
- `Content/Data/Item/ConsumableItem/DA_Item_Consumable_RawFish.uasset`
- `Content/Data/Item/ConsumableItem/DA_Item_Consumable_RawHorseMeat.uasset`
- `Content/Data/Item/EquipmentItem/DA_Item_Tool_StoneHatchet.uasset`
- `Content/Data/Item/EquipmentItem/DA_Item_Tool_StonePickaxe.uasset`
- `Content/Data/Item/Food/Item_Food_RawFish.uasset`
- `Content/Data/Item/HeldItem/DA_Item_Held_FishingRod_2.uasset`
- `Content/Data/Item/HeldItem/DA_Item_Held_Hammer.uasset`
- `Content/Data/Item/HeldItem/DA_Item_Held_Hatchet.uasset`
- `Content/Data/Item/HeldItem/DA_Item_Held_Pickaxe.uasset`
- `Content/Data/Item/HeldItem/NewDataAsset.uasset`
- `Content/Data/Item/Misc/DA_Item_Misc_Charcoal.uasset`
- `Content/Data/Item/Misc/DA_Item_Misc_Wood.uasset`
- `Content/Data/Item/Placeable/DA_Item_Placeable_Camffire.uasset`
- `Content/Data/Item/Placeable/DA_Item_Placeable_Campfire.uasset`
- `Content/Data/Item/Placeable/DA_Item_Placeable_SleepingBag.uasset`
- `Content/Data/Item/Placeable/DA_Item_Placeable_Workbench.uasset`
- `Content/Framework/GM_SessionTest.uasset`
- `Content/Imported/Rust/_Shared/M_RustAsset_Master.uasset`
- `Content/Item/Misc/CampFire/Materials/MI_CampFire_new_campfire_off.uasset`
- `Content/Item/Misc/CampFire/Meshes/SM_campfire_model.uasset`
- `Content/Item/Misc/CampFire/Textures/T_campfire_ao.uasset`
- `Content/Item/Misc/CampFire/Textures/T_campfire_bc.uasset`
- `Content/Item/Misc/CampFire/Textures/T_campfire_nrm.uasset`
- `Content/Item/Misc/CampFire/Textures/T_campfire_sg.uasset`
- `Content/Item/Misc/Workbench_Full/SM_MERGED_StaticMeshActor_40.uasset`
- `Content/Item/Placeable/BP_Campfire.uasset`
- `Content/Item/Placeable/BP_Workbench.uasset`
- `Content/Maps/Lv_AttackTest.umap`
- `Content/Maps/Lv_SessionTest.umap`
- `Content/Maps/Lv01_Test.umap`
- `R1.uproject`

## 삭제 및 이주

- 이전 CampfireActor와 Campfire 경로는 현재 Item/PlaceableItem/Campfire 코드 위치에 설명했습니다.
- 삭제된 Scripts/SetupCraftingItems.py 및 멀티플레이 메뉴 생성 스크립트는 복원하지 않았습니다.

## 검증

- 실행 코드 변경 없이 설명 주석만 추가했습니다. 빌드 및 에디터 실행은 수행하지 않았습니다.
