import unreal

BEL = unreal.BlueprintEditorLibrary
EAL = unreal.EditorAssetLibrary


def compile_and_save(blueprint, label):
    BEL.compile_blueprint(blueprint)
    EAL.save_loaded_asset(blueprint)
    unreal.log("Saved " + label)


def reparent(asset_path, parent_class, label):
    blueprint = EAL.load_asset(asset_path)
    if blueprint is None:
        unreal.log_error("Missing asset: " + asset_path)
        return False
    BEL.reparent_blueprint(blueprint, parent_class)
    compile_and_save(blueprint, label)
    unreal.log("Reparented " + label + " -> " + str(parent_class))
    return True


safe_tower_class = unreal.load_class(None, "/Script/Towerdefense_GADE.SafeTowerBase")
if safe_tower_class is None:
    raise RuntimeError("SafeTowerBase C++ class was not found. Compile the game module first.")

enemy_parent = unreal.load_class(None, "/Game/Blueprints/BP_Enemy.BP_Enemy_C")
if enemy_parent is None:
    raise RuntimeError("BP_Enemy parent class was not found.")

reparent("/Game/Blueprints/BP_TowerBase", safe_tower_class, "BP_TowerBase")
reparent("/Game/Blueprints/BP_Enemy2", enemy_parent, "BP_Enemy2")
reparent("/Game/Blueprints/BP_Enemy3", enemy_parent, "BP_Enemy3")

# Recompile towers/enemies so Cast to BP_Enemy sees the child classes.
for path in (
    "/Game/Blueprints/BP_TowerBase",
    "/Game/Blueprints/BP_Enemy",
    "/Game/Blueprints/BP_Enemy2",
    "/Game/Blueprints/BP_Enemy3",
    "/Game/Blueprints/BP_MortarDefense",
    "/Game/Blueprints/BP_InfantryVehicle",
):
    asset = EAL.load_asset(path)
    if asset:
        compile_and_save(asset, path)

unreal.log("fix_tower_blueprints.py finished")
