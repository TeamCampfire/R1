import unreal

path = "/Game/Blueprint/Widget/Multiplayer/WBP_MultiplayerMenu"
asset = unreal.EditorAssetLibrary.load_asset(path)
if not asset:
    raise RuntimeError(f"Could not load {path}")
unreal.BlueprintEditorLibrary.compile_blueprint(asset)
unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.log("Verified WBP_MultiplayerMenu BindWidget compilation")
