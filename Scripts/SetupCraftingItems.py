import unreal

ROOT = '/Game/Data/Item/'
materials = {key: unreal.load_asset(ROOT + 'Misc/DA_Item_Misc_' + name) for key, name in [('wood','Wood'),('stone','Stones'),('cloth','Cloth'),('metal','MetalFragments')]}
assert all(materials.values()), 'Missing crafting material asset'
recipes = [
('ConsumableItem/DA_Item_Consumable_Bandage', '붕대', {'cloth':4}, False),
('HeldItem/DA_Item_Held_StoneHatchet_2', '돌 도끼', {'wood':200,'stone':100}, False),
('HeldItem/DA_Item_Held_StonePickaxe_2', '돌 곡괭이', {'wood':200,'stone':100}, False),
('Placeable/DA_Item_Placeable_Campfire', '모닥불', {'wood':100}, False),
('HeldItem/DA_Item_Held_FishingRod_2', '낚싯대', {'cloth':2,'wood':200}, False),
('Placeable/DA_Item_Placeable_SleepingBag', '침낭', {'cloth':30}, False),
('Placeable/DA_Item_Placeable_Workbench', '작업대', {'wood':500,'metal':100}, False),
('HeldItem/DA_Item_Held_Hatchet', '금속 도끼', {'wood':100,'metal':75}, True),
('HeldItem/DA_Item_Held_Pickaxe', '금속 곡괭이', {'wood':100,'metal':125}, True),
('HeldItem/DA_Item_Held_Hammer', '망치', {'wood':100}, True),
]
for path, name, cost, bench in recipes:
    item = unreal.load_asset(ROOT + path)
    assert item, 'Missing asset: ' + path
    item.set_editor_property('display_name', name)
    ingredients = []
    for key, amount in cost.items():
        ingredient = unreal.CraftIngredient()
        ingredient.set_editor_property('item', materials[key])
        ingredient.set_editor_property('amount', amount)
        ingredients.append(ingredient)
    item.set_editor_property('crafting_cost', ingredients)
    item.set_editor_property('crafting_seconds', 5.0)
    item.set_editor_property('requires_workbench', bench)
    if not str(item.get_editor_property('description')):
        item.set_editor_property('description', name + (' · 작업대에서 제작할 수 있습니다.' if bench else ' · 기본 제작으로 만들 수 있습니다.'))
    if name == '붕대':
        heal = unreal.ItemEffect()
        heal.set_editor_property('effect_type', unreal.ItemEffectType.HEAL)
        heal.set_editor_property('magnitude', 15.0)
        heal.set_editor_property('duration', 0.0)
        item.set_editor_property('effects', [heal])
    unreal.EditorAssetLibrary.save_loaded_asset(item)
    unreal.log('CRAFTING_CONFIGURED ' + path)
unreal.log('CRAFTING_SETUP_COMPLETE: 10 recipes (torch owned by another developer)')
