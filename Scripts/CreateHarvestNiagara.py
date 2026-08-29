# -*- coding: utf-8 -*-
"""
Harvest Niagara FX Generator for Unreal Engine 5
이 스크립트는 언리얼 에디터의 Output Log (Cmd/Python) 창에서 실행하면
/Game/FX/ 폴더에 채집용 파편 및 스위트스팟 나이아가라 시스템을 자동 생성합니다.

실행 방법:
언리얼 에디터 하단의 Output Log 창에서 모드를 Python으로 바꾼 뒤:
exec(open(r"e:/Unreal Projects/R1/Scripts/CreateHarvestNiagara.py", encoding="utf-8").read())
"""

import unreal

def create_harvest_niagara_systems():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    fx_dir = "/Game/FX"
    if not editor_asset_lib.does_directory_exist(fx_dir):
        editor_asset_lib.make_directory(fx_dir)
        print(f"[R1] '{fx_dir}' 디렉토리를 생성했습니다.")

    # 엔진 내장 나이아가라 템플릿 검색 (Simple Burst / Omnidirectional Burst / Directional Burst)
    templates = [
        "/Niagara/DefaultAssets/Templates/SimpleBurst.SimpleBurst",
        "/Niagara/DefaultAssets/Templates/OmnidirectionalBurst.OmnidirectionalBurst",
        "/Niagara/DefaultAssets/Templates/DirectionalBurst.DirectionalBurst",
        "/Niagara/DefaultAssets/Templates/SimpleParticleSystem.SimpleParticleSystem"
    ]
    
    source_template = None
    for tmpl in templates:
        if editor_asset_lib.does_asset_exist(tmpl):
            source_template = tmpl
            break
            
    # 1. 일반 타격 파편 시스템 (NS_Harvest_Impact)
    harvest_impact_path = f"{fx_dir}/NS_Harvest_Impact"
    if not editor_asset_lib.does_asset_exist(harvest_impact_path):
        if source_template:
            editor_asset_lib.duplicate_asset(source_template, harvest_impact_path)
            print(f"[R1] '{harvest_impact_path}' 생성 완료 (기반 템플릿: {source_template})")
        else:
            factory = unreal.NiagaraSystemFactoryNew()
            asset_tools.create_asset("NS_Harvest_Impact", fx_dir, unreal.NiagaraSystem, factory)
            print(f"[R1] '{harvest_impact_path}' 기본 생성 완료")
    else:
        print(f"[R1] '{harvest_impact_path}'가 이미 존재합니다.")

    # 2. 스위트 스팟 적중 스파크/충격 시스템 (NS_SweetSpot_Impact)
    sweetspot_impact_path = f"{fx_dir}/NS_SweetSpot_Impact"
    if not editor_asset_lib.does_asset_exist(sweetspot_impact_path):
        spark_template = "/Niagara/DefaultAssets/Templates/DirectionalBurst.DirectionalBurst"
        if not editor_asset_lib.does_asset_exist(spark_template):
            spark_template = source_template

        if spark_template:
            editor_asset_lib.duplicate_asset(spark_template, sweetspot_impact_path)
            print(f"[R1] '{sweetspot_impact_path}' 생성 완료 (기반 템플릿: {spark_template})")
        else:
            factory = unreal.NiagaraSystemFactoryNew()
            asset_tools.create_asset("NS_SweetSpot_Impact", fx_dir, unreal.NiagaraSystem, factory)
            print(f"[R1] '{sweetspot_impact_path}' 기본 생성 완료")
    else:
        print(f"[R1] '{sweetspot_impact_path}'가 이미 존재합니다.")

    # 에셋 저장
    editor_asset_lib.save_directory(fx_dir, only_if_is_dirty=False, recursive=True)
    print("[R1] 모든 나이아가라 FX 에셋이 /Game/FX/ 에 성공적으로 저장되었습니다!")

if __name__ == "__main__":
    create_harvest_niagara_systems()
