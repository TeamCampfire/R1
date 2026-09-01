import unreal


ASSET_PATH = "/Game/Blueprint/Widget/Multiplayer"
ASSET_NAME = "WBP_MultiplayerMenu"


def main():
    asset_object_path = f"{ASSET_PATH}/{ASSET_NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_object_path):
        unreal.EditorAssetLibrary.delete_asset(asset_object_path)

    parent_class = unreal.load_class(None, "/Script/R1.MultiplayerMenuWidget")
    if not parent_class:
        raise RuntimeError("UMultiplayerMenuWidget is not compiled or could not be loaded")
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, ASSET_PATH, unreal.WidgetBlueprint, factory
    )
    if not asset:
        raise RuntimeError("Failed to create WBP_MultiplayerMenu")

    if not unreal.MultiplayerWidgetEditorLibrary.build_multiplayer_menu_widget_tree(asset):
        raise RuntimeError("Failed to build WBP_MultiplayerMenu WidgetTree")

    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    unreal.log(f"Created {ASSET_PATH}/{ASSET_NAME}")


main()
