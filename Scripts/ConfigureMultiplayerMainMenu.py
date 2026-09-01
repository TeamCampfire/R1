import unreal


MAP_PATH = "/Game/Maps/Lv_MainMenu"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"Could not load {MAP_PATH}")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not unreal.MultiplayerWidgetEditorLibrary.configure_multiplayer_main_menu_world(world):
    raise RuntimeError("Could not configure Lv_MainMenu World Settings")

if not level_subsystem.save_current_level():
    raise RuntimeError("Could not save Lv_MainMenu")

unreal.log("Configured Lv_MainMenu to use AMultiplayerMenuGameMode")
