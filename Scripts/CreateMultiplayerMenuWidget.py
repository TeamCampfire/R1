import unreal


# 생성되는 두 Widget Blueprint가 공유하는 콘텐츠 경로와 에셋 이름이다.
ASSET_PATH = "/Game/Blueprint/Widget/Multiplayer"
MENU_ASSET_NAME = "WBP_MultiplayerMenu"
ROW_ASSET_NAME = "WBP_MultiplayerSessionRow"


def create_widget_blueprint(asset_name, parent_class_path):
    """기존 에셋을 삭제한 뒤 지정한 C++ 부모를 가진 Widget Blueprint를 새로 만든다."""
    asset_object_path = f"{ASSET_PATH}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_object_path):
        unreal.EditorAssetLibrary.delete_asset(asset_object_path)

    parent_class = unreal.load_class(None, parent_class_path)
    if not parent_class:
        raise RuntimeError(f"Could not load parent class {parent_class_path}")
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, ASSET_PATH, unreal.WidgetBlueprint, factory
    )
    if not asset:
        raise RuntimeError(f"Failed to create {asset_name}")
    return asset


def main():
    # 메뉴가 참조할 행 WBP를 먼저 생성하고 BindWidget 구조가 유효한지 컴파일한다.
    row_asset = create_widget_blueprint(
        ROW_ASSET_NAME, "/Script/R1.MultiplayerSessionRowWidget"
    )
    if not unreal.MultiplayerWidgetEditorLibrary.build_multiplayer_session_row_widget_tree(row_asset):
        raise RuntimeError("Failed to build WBP_MultiplayerSessionRow WidgetTree")
    unreal.BlueprintEditorLibrary.compile_blueprint(row_asset)
    unreal.EditorAssetLibrary.save_loaded_asset(row_asset)

    # 행 클래스가 저장된 뒤 이를 사용하는 메인 메뉴 WBP를 생성한다.
    menu_asset = create_widget_blueprint(
        MENU_ASSET_NAME, "/Script/R1.MultiplayerMenuWidget"
    )
    if not unreal.MultiplayerWidgetEditorLibrary.build_multiplayer_menu_widget_tree(menu_asset):
        raise RuntimeError("Failed to build WBP_MultiplayerMenu WidgetTree")
    unreal.BlueprintEditorLibrary.compile_blueprint(menu_asset)
    unreal.EditorAssetLibrary.save_loaded_asset(menu_asset)
    unreal.log(f"Created {ASSET_PATH}/{ROW_ASSET_NAME} and {MENU_ASSET_NAME}")


main()
