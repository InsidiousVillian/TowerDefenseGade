"""
Phase 2 setup for Towerdefense_GADE (UE 5.7).

Run once in the Unreal Editor after the C++ module compiles:
  Tools > Execute Python Script > pick Content/Python/setup_phase2.py

Requires:
  - Python Editor Script Plugin enabled (listed in the .uproject)
  - Editor Scripting Utilities enabled
  - A successful C++ compile so EnemyBase / RangedEnemy / EnemyData exist

What this script does:
  1. Creates /Game/Data/Enemies/DA_Enemy_Light, DA_Enemy_Heavy, DA_Enemy_Ranged
  2. Reparents BP_Enemy and BP_Enemy2 -> EnemyBase, BP_Enemy3 -> RangedEnemy
  3. Assigns each enemy Blueprint's EnemyData on its Class Defaults
  4. Compiles and saves the touched Blueprints (enemies, tower, grid, build menu)

What it cannot do:
  - Delete Blueprint variables or graph nodes that clash with C++ names
  - Fix reparent failures caused by those name clashes (manual cleanup required)
"""

from __future__ import annotations

import unreal


DATA_DIR = "/Game/Data/Enemies"
ENEMY_DATA_CLASS_PATH = "/Script/Towerdefense_GADE.EnemyData"
ENEMY_BASE_CLASS_PATH = "/Script/Towerdefense_GADE.EnemyBase"
RANGED_ENEMY_CLASS_PATH = "/Script/Towerdefense_GADE.RangedEnemy"

BP_ENEMY = "/Game/Blueprints/BP_Enemy"
BP_ENEMY2 = "/Game/Blueprints/BP_Enemy2"
BP_ENEMY3 = "/Game/Blueprints/BP_Enemy3"
BP_TOWER = "/Game/Blueprints/BP_TowerBase"
BP_GRID = "/Game/Blueprints/BP_GridGenerator"
WBP_BUILD = "/Game/Blueprints/WBP_BuildMenu"


def log(msg: str) -> None:
    unreal.log("[setup_phase2] {}".format(msg))


def log_warning(msg: str) -> None:
    unreal.log_warning("[setup_phase2] {}".format(msg))


def log_error(msg: str) -> None:
    unreal.log_error("[setup_phase2] {}".format(msg))


def load_class(path: str):
    cls = unreal.load_class(None, path)
    if cls is None:
        log_error("Could not load class: {}. Compile the C++ module first.".format(path))
    return cls


def ensure_folder(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)
        log("Created folder {}".format(path))


def resolve_enemy_type(name: str):
    """Return unreal.EnemyType.<NAME> when the reflected enum exists."""
    enum_type = getattr(unreal, "EnemyType", None)
    if enum_type is None:
        log_warning("unreal.EnemyType not reflected yet; leave EnemyType at asset default.")
        return None
    value = getattr(enum_type, name, None)
    if value is None:
        log_warning("EnemyType.{} not found.".format(name))
    return value


def build_stats():
    """Suggested starting stats (also listed in the PR)."""
    return {
        "DA_Enemy_Light": {
            "enemy_type": resolve_enemy_type("LIGHT"),
            "max_health": 3.0,
            "path_speed": 350.0,
            "exit_damage": 1.0,
            "gold_reward": 10.0,
            "spawn_cost": 10.0,
            "shoot_range": 0.0,
            "shoot_interval": 1.0,
            "tower_damage": 0.0,
        },
        "DA_Enemy_Heavy": {
            "enemy_type": resolve_enemy_type("HEAVY"),
            "max_health": 12.0,
            "path_speed": 150.0,
            "exit_damage": 2.0,
            "gold_reward": 35.0,
            "spawn_cost": 40.0,
            "shoot_range": 0.0,
            "shoot_interval": 1.0,
            "tower_damage": 0.0,
        },
        "DA_Enemy_Ranged": {
            "enemy_type": resolve_enemy_type("RANGED"),
            "max_health": 5.0,
            "path_speed": 220.0,
            "exit_damage": 1.0,
            "gold_reward": 20.0,
            "spawn_cost": 25.0,
            "shoot_range": 600.0,
            "shoot_interval": 1.0,
            "tower_damage": 1.0,
        },
    }


def create_or_update_enemy_data(asset_name: str, stats: dict):
    asset_path = "{}/{}".format(DATA_DIR, asset_name)
    enemy_data_class = load_class(ENEMY_DATA_CLASS_PATH)
    if enemy_data_class is None:
        return None

    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        log("Updating existing data asset {}".format(asset_path))
    else:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.DataAssetFactory()
        try:
            factory.set_editor_property("data_asset_class", enemy_data_class)
        except Exception as exc:
            log_warning("Could not set factory data_asset_class ({}).".format(exc))

        asset = asset_tools.create_asset(asset_name, DATA_DIR, enemy_data_class, factory)
        if asset is None:
            log_error("Failed to create {}".format(asset_path))
            return None
        log("Created data asset {}".format(asset_path))

    for prop_name, value in stats.items():
        if value is None:
            continue
        try:
            asset.set_editor_property(prop_name, value)
        except Exception as exc:
            log_error("Failed to set {} on {}: {}".format(prop_name, asset_name, exc))

    unreal.EditorAssetLibrary.save_asset(asset_path)
    return asset


def reparent_blueprint(bp_path: str, parent_class_path: str) -> bool:
    parent_class = load_class(parent_class_path)
    if parent_class is None:
        return False

    if not unreal.EditorAssetLibrary.does_asset_exist(bp_path):
        log_error("Blueprint not found: {}".format(bp_path))
        return False

    blueprint = unreal.EditorAssetLibrary.load_asset(bp_path)
    if blueprint is None:
        log_error("Could not load Blueprint: {}".format(bp_path))
        return False

    try:
        unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, parent_class)
        log("Reparented {} -> {}".format(bp_path, parent_class_path))
        return True
    except Exception as exc:
        log_error(
            "Reparent failed for {}: {}. "
            "Delete Blueprint variables that clash with C++ names, then re-run.".format(bp_path, exc)
        )
        return False


def assign_enemy_data(bp_path: str, data_asset) -> bool:
    if data_asset is None:
        log_error("No data asset to assign on {}".format(bp_path))
        return False

    blueprint = unreal.EditorAssetLibrary.load_asset(bp_path)
    if blueprint is None:
        log_error("Could not load Blueprint: {}".format(bp_path))
        return False

    try:
        generated = blueprint.generated_class()
        cdo = unreal.get_default_object(generated)
        cdo.set_editor_property("enemy_data", data_asset)
        log("Assigned EnemyData on Class Defaults of {}".format(bp_path))
        return True
    except Exception as exc:
        log_error("Could not assign EnemyData on {}: {}".format(bp_path, exc))
        return False


def compile_and_save(bp_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_asset_exist(bp_path):
        log_warning("Skip compile; missing {}".format(bp_path))
        return

    blueprint = unreal.EditorAssetLibrary.load_asset(bp_path)
    if blueprint is None:
        log_warning("Skip compile; could not load {}".format(bp_path))
        return

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        log("Compiled {}".format(bp_path))
    except Exception as exc:
        log_error("Compile failed for {}: {}".format(bp_path, exc))

    unreal.EditorAssetLibrary.save_asset(bp_path)
    log("Saved {}".format(bp_path))


def main() -> None:
    log("Starting Phase 2 setup...")

    if load_class(ENEMY_DATA_CLASS_PATH) is None:
        log_error("Aborting: C++ types are not available.")
        return

    ensure_folder(DATA_DIR)

    stats_by_asset = build_stats()
    light = create_or_update_enemy_data("DA_Enemy_Light", stats_by_asset["DA_Enemy_Light"])
    heavy = create_or_update_enemy_data("DA_Enemy_Heavy", stats_by_asset["DA_Enemy_Heavy"])
    ranged = create_or_update_enemy_data("DA_Enemy_Ranged", stats_by_asset["DA_Enemy_Ranged"])

    ok_enemy = reparent_blueprint(BP_ENEMY, ENEMY_BASE_CLASS_PATH)
    ok_enemy2 = reparent_blueprint(BP_ENEMY2, ENEMY_BASE_CLASS_PATH)
    ok_enemy3 = reparent_blueprint(BP_ENEMY3, RANGED_ENEMY_CLASS_PATH)

    if ok_enemy:
        assign_enemy_data(BP_ENEMY, light)
    if ok_enemy2:
        assign_enemy_data(BP_ENEMY2, heavy)
    if ok_enemy3:
        assign_enemy_data(BP_ENEMY3, ranged)

    for path in (BP_ENEMY, BP_ENEMY2, BP_ENEMY3, BP_TOWER, BP_GRID, WBP_BUILD):
        compile_and_save(path)

    log("Done.")
    log("Remaining manual steps (only if reparent failed or old graphs still run):")
    log("  1. On BP_Enemy / BP_Enemy2 / BP_Enemy3, delete Blueprint variables that")
    log("     duplicate C++ intent: Health, TargetSpline, Distance, MovementSpeed,")
    log("     BaseDamage, Damage, AttackRange, TargetTower, isAttacking?, AttackTimerHandle.")
    log("  2. Delete Blueprint Event Tick movement, ReceiveAnyDamage / death, and doAttack")
    log("     graphs that now live in C++ (otherwise both systems fight each other).")
    log("  3. On BP_TowerBase, remove or disconnect ReceiveAnyDamage if it fights TowerHealth.")
    log("  4. Confirm Buydefender / Buydefender_1 / Buydefender_2 are Is Variable so BindWidget works.")
    log("  5. PIE: light = fast/fragile, heavy = slow/tanky, ranged = stop-and-shoot towers.")


if __name__ == "__main__":
    main()
