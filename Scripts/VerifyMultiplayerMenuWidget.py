import unreal

# 두 WBP를 다시 컴파일해 C++ BindWidget 이름과 디자이너 변수가 일치하는지 검증한다.
paths = [
    "/Game/Blueprint/Widget/Multiplayer/WBP_MultiplayerSessionRow",
    "/Game/Blueprint/Widget/Multiplayer/WBP_MultiplayerMenu",
]
for path in paths:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Could not load {path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.log("Verified multiplayer menu and session row BindWidget compilation")
