# -*- coding: utf-8 -*-
"""
Realistic Lake Water Shader Generator for Unreal Engine 5 (Project R1)
3중 비동기 파동(3-Octave Asynchronous Ripples)을 적용하여:
1. '너무 자주 흔들리는 문제' -> 파동 속도를 대폭 낮추고 파장을 4~5배 넓혀 아주 잠잠하고 유유하게 흐르도록 개선
2. '모두 동일하게 흔들리는 문제' -> 서로 다른 3개의 방향/속도/파장(서로소 비율)을 교차 합성하여 불규칙하고 유기적인 자연 수면 연출
"""

import unreal

def build_advanced_lake_material():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    mat_dir = "/Game/Asset/Material"
    if not editor_asset_lib.does_directory_exist(mat_dir):
        editor_asset_lib.make_directory(mat_dir)
        
    mat_name = "M_Water_Sample"
    mat_path = f"{mat_dir}/{mat_name}"
    
    # 머티리얼 에셋 생성
    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(mat_name, mat_dir, unreal.Material, factory)
    
    # 1. 반투명(Translucent), 픽셀 조명, 양면
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    mat.set_editor_property("two_sided", True)
    
    # -------------------------------------------------------------------------
    # 2. Depth Fade (물가 투명도 & 깊이 색상 그라데이션)
    # -------------------------------------------------------------------------
    depth_fade = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionDepthFade, -1000, -200)
    fade_dist = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1200, -100)
    fade_dist.set_editor_property("parameter_name", "DepthFadeDistance")
    fade_dist.set_editor_property("default_value", 250.0)
    unreal.MaterialEditingLibrary.connect_material_expressions(fade_dist, "", depth_fade, "FadeDistance")
    
    # 얕은 물 색상 (Shallow Water - 맑은 청록빛)
    shallow_color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1000, -450)
    shallow_color.set_editor_property("parameter_name", "ShallowWaterColor")
    shallow_color.set_editor_property("default_value", unreal.LinearColor(0.03, 0.32, 0.36, 1.0))
    
    # 깊은 물 색상 (Deep Water - 짙은 딥 네이비)
    deep_color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1000, -320)
    deep_color.set_editor_property("parameter_name", "DeepWaterColor")
    deep_color.set_editor_property("default_value", unreal.LinearColor(0.005, 0.05, 0.14, 1.0))
    
    depth_color_lerp = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -750, -350)
    unreal.MaterialEditingLibrary.connect_material_expressions(shallow_color, "", depth_color_lerp, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(deep_color, "", depth_color_lerp, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(depth_fade, "", depth_color_lerp, "Alpha")
    
    # -------------------------------------------------------------------------
    # 3. Fresnel (시선 각도에 따른 은은한 하늘 반사)
    # -------------------------------------------------------------------------
    fresnel = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionFresnel, -750, -100)
    fresnel_exp = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -950, -50)
    fresnel_exp.set_editor_property("parameter_name", "FresnelExponent")
    fresnel_exp.set_editor_property("default_value", 4.0)
    unreal.MaterialEditingLibrary.connect_material_expressions(fresnel_exp, "", fresnel, "Exponent")
    
    sky_color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -750, 50)
    sky_color.set_editor_property("parameter_name", "SkyReflectionColor")
    sky_color.set_editor_property("default_value", unreal.LinearColor(0.35, 0.55, 0.72, 1.0))
    
    final_base_color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -500, -200)
    unreal.MaterialEditingLibrary.connect_material_expressions(depth_color_lerp, "", final_base_color, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(sky_color, "", final_base_color, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(fresnel, "", final_base_color, "Alpha")
    
    # -------------------------------------------------------------------------
    # 4. 3중 비동기 파동 (3-Octave Asynchronous Natural Ripples)
    # -------------------------------------------------------------------------
    world_pos = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1800, 400)
    time_node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionTime, -1800, 650)
    
    # 마스터 속도 계수 (아주 느리고 여유로운 속도: 0.08)
    master_speed = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1800, 800)
    master_speed.set_editor_property("parameter_name", "MasterWaveSpeed")
    master_speed.set_editor_property("default_value", 0.08)
    
    scaled_time = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1600, 700)
    unreal.MaterialEditingLibrary.connect_material_expressions(time_node, "", scaled_time, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(master_speed, "", scaled_time, "B")
    
    # --- Wave 1: 거대하고 아주 완만한 배경 파동 (방향: (1.0, 0.2), 파장: 1800cm) ---
    wave1_scale = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -1600, 350)
    wave1_scale.set_editor_property("r", 1800.0)
    wave1_pos = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionDivide, -1400, 350)
    unreal.MaterialEditingLibrary.connect_material_expressions(world_pos, "", wave1_pos, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wave1_scale, "", wave1_pos, "B")
    
    wave1_add = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionAdd, -1200, 350)
    unreal.MaterialEditingLibrary.connect_material_expressions(wave1_pos, "", wave1_add, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(scaled_time, "", wave1_add, "B")
    
    wave1_sine = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionSine, -1000, 350)
    unreal.MaterialEditingLibrary.connect_material_expressions(wave1_add, "", wave1_sine, "")
    
    # --- Wave 2: 중간 크기의 대각선 교차 파동 (방향: (-0.4, 0.9), 파장: 850cm, 속도: 1.3배) ---
    wave2_time_mult = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -1400, 500)
    wave2_time_mult.set_editor_property("r", 1.37)
    wave2_time = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1200, 500)
    unreal.MaterialEditingLibrary.connect_material_expressions(scaled_time, "", wave2_time, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wave2_time_mult, "", wave2_time, "B")
    
    wave2_scale = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -1600, 480)
    wave2_scale.set_editor_property("r", 850.0)
    wave2_pos = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionDivide, -1400, 450)
    unreal.MaterialEditingLibrary.connect_material_expressions(world_pos, "", wave2_pos, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wave2_scale, "", wave2_pos, "B")
    
    wave2_add = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionAdd, -1000, 480)
    unreal.MaterialEditingLibrary.connect_material_expressions(wave2_pos, "", wave2_add, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wave2_time, "", wave2_add, "B")
    
    wave2_cos = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionCosine, -850, 480)
    unreal.MaterialEditingLibrary.connect_material_expressions(wave2_add, "", wave2_cos, "")
    
    # --- Wave 3: 미세한 표면 잔물결 (파장: 360cm, 속도: 0.8배) ---
    wave3_time_mult = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -1400, 650)
    wave3_time_mult.set_editor_property("r", 0.79)
    wave3_time = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1200, 650)
    unreal.MaterialEditingLibrary.connect_material_expressions(scaled_time, "", wave3_time, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wave3_time_mult, "", wave3_time, "B")
    
    wave3_scale = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -1600, 600)
    wave3_scale.set_editor_property("r", 360.0)
    wave3_pos = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionDivide, -1400, 600)
    unreal.MaterialEditingLibrary.connect_material_expressions(world_pos, "", wave3_pos, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wave3_scale, "", wave3_pos, "B")
    
    wave3_add = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionAdd, -1000, 620)
    unreal.MaterialEditingLibrary.connect_material_expressions(wave3_pos, "", wave3_add, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wave3_time, "", wave3_add, "B")
    
    wave3_sine = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionSine, -850, 620)
    unreal.MaterialEditingLibrary.connect_material_expressions(wave3_add, "", wave3_sine, "")
    
    # --- 3개 파동의 가중치 합성 (0.5 * Wave1 + 0.35 * Wave2 + 0.15 * Wave3) ---
    w1_weight = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -850, 350)
    w1_weight.set_editor_property("r", 0.5)
    w1_weighted = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -700, 350)
    unreal.MaterialEditingLibrary.connect_material_expressions(wave1_sine, "", w1_weighted, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(w1_weight, "", w1_weighted, "B")
    
    w2_weight = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -700, 480)
    w2_weight.set_editor_property("r", 0.35)
    w2_weighted = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -550, 480)
    unreal.MaterialEditingLibrary.connect_material_expressions(wave2_cos, "", w2_weighted, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(w2_weight, "", w2_weighted, "B")
    
    w3_weight = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -700, 620)
    w3_weight.set_editor_property("r", 0.15)
    w3_weighted = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -550, 620)
    unreal.MaterialEditingLibrary.connect_material_expressions(wave3_sine, "", w3_weighted, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(w3_weight, "", w3_weighted, "B")
    
    comb12 = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionAdd, -400, 400)
    unreal.MaterialEditingLibrary.connect_material_expressions(w1_weighted, "", comb12, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(w2_weighted, "", comb12, "B")
    
    combined_wave = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionAdd, -250, 450)
    unreal.MaterialEditingLibrary.connect_material_expressions(comb12, "", combined_wave, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(w3_weighted, "", combined_wave, "B")
    
    # 노멀 강도 (RippleStrength: 0.06 - 은은하고 부드러운 일렁임)
    ripple_strength = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -250, 600)
    ripple_strength.set_editor_property("parameter_name", "RippleStrength")
    ripple_strength.set_editor_property("default_value", 0.06)
    
    mult_xy = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -100, 500)
    unreal.MaterialEditingLibrary.connect_material_expressions(combined_wave, "", mult_xy, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(ripple_strength, "", mult_xy, "B")
    
    const_one = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -100, 650)
    const_one.set_editor_property("r", 1.0)
    
    append_xyz = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionAppendVector, 80, 550)
    unreal.MaterialEditingLibrary.connect_material_expressions(mult_xy, "", append_xyz, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(const_one, "", append_xyz, "B")
    
    normal_vector = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionNormalize, 250, 550)
    unreal.MaterialEditingLibrary.connect_material_expressions(append_xyz, "", normal_vector, "")
    
    # -------------------------------------------------------------------------
    # 5. Roughness & Specular & Opacity
    # -------------------------------------------------------------------------
    roughness = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 50)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.04)
    
    specular = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 150)
    specular.set_editor_property("parameter_name", "Specular")
    specular.set_editor_property("default_value", 0.85)
    
    opacity_lerp = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -300, 250)
    opac_shallow = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 200)
    opac_shallow.set_editor_property("parameter_name", "OpacityShallow")
    opac_shallow.set_editor_property("default_value", 0.3)
    
    opac_deep = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 300)
    opac_deep.set_editor_property("parameter_name", "OpacityDeep")
    opac_deep.set_editor_property("default_value", 0.88)
    
    unreal.MaterialEditingLibrary.connect_material_expressions(opac_shallow, "", opacity_lerp, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(opac_deep, "", opacity_lerp, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(depth_fade, "", opacity_lerp, "Alpha")
    
    # -------------------------------------------------------------------------
    # 6. 최종 마스터 머티리얼 출력 연결
    # -------------------------------------------------------------------------
    unreal.MaterialEditingLibrary.connect_material_property(final_base_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)
    unreal.MaterialEditingLibrary.connect_material_property(opacity_lerp, "", unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.connect_material_property(normal_vector, "", unreal.MaterialProperty.MP_NORMAL)
    
    unreal.MaterialEditingLibrary.recompile_material(mat)
    editor_asset_lib.save_asset(mat_path)
    print(f"[R1] 3중 비동기 자연 파도 머티리얼 생성 완료: {mat_path}")
    return mat

if __name__ == "__main__":
    build_advanced_lake_material()
