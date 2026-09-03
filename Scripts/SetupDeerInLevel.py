# -*- coding: utf-8 -*-
import unreal

def setup_deer():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    editor_level_lib = unreal.EditorLevelLibrary

    target_dir = "/Game/AnimalVarietyPack/DeerStagAndDoe/Blueprints"
    if not editor_asset_lib.does_directory_exist(target_dir):
        editor_asset_lib.make_directory(target_dir)

    skeleton_path = "/Game/AnimalVarietyPack/DeerStagAndDoe/Meshes/SK_DeerStag_Skeleton"
    skeleton = editor_asset_lib.load_asset(skeleton_path)

    # 1. ABP_DeerStag (애니메이션 블루프린트) 생성
    abp_name = "ABP_DeerStag"
    abp_path = f"{target_dir}/{abp_name}"
    abp_asset = None

    if not editor_asset_lib.does_asset_exist(abp_path):
        factory = unreal.AnimBlueprintFactory()
        factory.target_skeleton = skeleton
        
        # AnimalAnimInstance C++ 클래스 탐색
        parent_class = unreal.load_class(None, "/Script/R1.AnimalAnimInstance")
        if parent_class:
            factory.parent_class = parent_class

        abp_asset = asset_tools.create_asset(abp_name, target_dir, unreal.AnimBlueprint, factory)
        unreal.log(f"[SUCCESS] Created AnimBlueprint: {abp_path}")
    else:
        abp_asset = editor_asset_lib.load_asset(abp_path)
        unreal.log(f"[INFO] AnimBlueprint already exists: {abp_path}")

    # 2. BP_Deer (캐릭터 블루프린트) 생성
    bp_name = "BP_Deer"
    bp_path = f"{target_dir}/{bp_name}"
    bp_asset = None

    parent_char_class = unreal.load_class(None, "/Script/R1.AnimalCharacter")
    if not parent_char_class:
        parent_char_class = unreal.Character

    if not editor_asset_lib.does_asset_exist(bp_path):
        bp_factory = unreal.BlueprintFactory()
        bp_factory.set_editor_property("parent_class", parent_char_class)
        bp_asset = asset_tools.create_asset(bp_name, target_dir, unreal.Blueprint, bp_factory)
        unreal.log(f"[SUCCESS] Created Blueprint: {bp_path}")
    else:
        bp_asset = editor_asset_lib.load_asset(bp_path)
        unreal.log(f"[INFO] Blueprint already exists: {bp_path}")

    # 3. 현재 레벨(Lv_AnimalTest)에 NavMeshBoundsVolume 및 BP_Deer 배치
    # NavMeshBoundsVolume 배치 (반경 100m)
    nav_actors = editor_level_lib.get_all_level_actors()
    has_nav = any("NavMeshBoundsVolume" in a.get_name() for a in nav_actors)
    if not has_nav:
        nav_vol = editor_level_lib.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        if nav_vol:
            nav_vol.set_actor_scale3d(unreal.Vector(100.0, 100.0, 20.0))
            unreal.log("[SUCCESS] Spawned NavMeshBoundsVolume (Scale: 100, 100, 20)")
    else:
        unreal.log("[INFO] NavMeshBoundsVolume already present in level")

    # BP_Deer 액터 스폰
    deer_generated_class = unreal.EditorAssetLibrary.load_blueprint_class(bp_path)
    if deer_generated_class:
        # 사슴 2마리 배치
        deer1 = editor_level_lib.spawn_actor_from_class(deer_generated_class, unreal.Vector(200.0, 200.0, 50.0), unreal.Rotator(0.0, 0.0, 0.0))
        deer2 = editor_level_lib.spawn_actor_from_class(deer_generated_class, unreal.Vector(-300.0, 100.0, 50.0), unreal.Rotator(0.0, 45.0, 0.0))
        unreal.log(f"[SUCCESS] Spawned Deer Actors in Level: {deer1}, {deer2}")

    # 레벨 및 에셋 저장
    editor_level_lib.save_current_level()
    unreal.log("[SUCCESS] Level saved successfully!")

if __name__ == "__main__":
    setup_deer()
