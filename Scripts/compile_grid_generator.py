import unreal

BEL = unreal.BlueprintEditorLibrary
EAL = unreal.EditorAssetLibrary

blueprint = EAL.load_asset("/Game/Blueprints/BP_GridGenerator")
if blueprint is None:
    raise RuntimeError("Missing BP_GridGenerator")

BEL.compile_blueprint(blueprint)
EAL.save_loaded_asset(blueprint)
unreal.log("Compiled and saved BP_GridGenerator")
