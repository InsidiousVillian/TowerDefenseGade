import unreal

BEL = unreal.BlueprintEditorLibrary
EAL = unreal.EditorAssetLibrary

GRID_PATH = "/Game/Blueprints/BP_GridGenerator"


def compile_and_save(blueprint, label):
    BEL.compile_blueprint(blueprint)
    EAL.save_loaded_asset(blueprint)
    unreal.log("Saved " + label)


def get_ubergraph(blueprint):
    for method_name in ("get_all_graphs", "get_graphs"):
        method = getattr(BEL, method_name, None)
        if callable(method):
            try:
                graphs = list(method(blueprint))
                for graph in graphs:
                    if graph and "EventGraph" in str(graph.get_name()):
                        return graph
                if graphs:
                    return graphs[0]
            except Exception:
                pass

    for prop in ("ubergraph_pages", "UbergraphPages", "function_graphs", "FunctionGraphs"):
        try:
            pages = blueprint.get_editor_property(prop)
            if pages:
                return list(pages)[0]
        except Exception:
            pass

    path_name = blueprint.get_path_name()
    for suffix in (":EventGraph", ".EventGraph"):
        graph = unreal.load_object(None, path_name + suffix)
        if graph:
            return graph

    graph = unreal.load_object(None, "/Game/Blueprints/BP_GridGenerator.BP_GridGenerator:EventGraph")
    if graph:
        return graph

    return None


def get_nodes(graph):
    if graph is None:
        return []
    for prop in ("nodes", "Nodes"):
        try:
            nodes = graph.get_editor_property(prop)
            if nodes:
                return list(nodes)
        except Exception:
            pass
    attr = getattr(graph, "nodes", None)
    if callable(attr):
        try:
            return list(attr())
        except Exception:
            pass
    elif attr:
        return list(attr)
    getter = getattr(graph, "get_nodes", None)
    if callable(getter):
        try:
            return list(getter())
        except Exception:
            pass
    return []


def get_pins(node):
    try:
        return list(node.get_pins())
    except Exception:
        try:
            return list(node.get_editor_property("pins"))
        except Exception:
            return []


def pin_name(pin):
    return str(pin.get_editor_property("pin_name"))


def node_class_name(node):
    return node.get_class().get_name()


def function_member_name(node):
    try:
        func_ref = node.get_editor_property("function_reference")
        return str(func_ref.get_editor_property("member_name"))
    except Exception:
        return ""


def get_generated_class(blueprint):
    try:
        generated = blueprint.get_editor_property("generated_class")
        if generated is not None and not callable(generated):
            return generated
    except Exception:
        pass

    generated_attr = getattr(blueprint, "generated_class", None)
    if callable(generated_attr):
        try:
            return generated_attr()
        except Exception:
            return None
    return generated_attr


def find_enemy_class_count(blueprint):
    generated = get_generated_class(blueprint)
    if generated is None:
        return 0

    cdo = None
    try:
        cdo = unreal.get_default_object(generated)
    except Exception:
        getter = getattr(generated, "get_default_object", None)
        if callable(getter):
            try:
                cdo = getter()
            except Exception:
                cdo = None
    if cdo is None:
        return 0

    for name in ("Enemyclasses", "EnemyClasses"):
        try:
            values = cdo.get_editor_property(name)
            if values is not None:
                return len(list(values))
        except Exception:
            pass
    return 0


def iter_grid_k2_nodes(graph):
    nodes = get_nodes(graph)
    if nodes:
        return nodes

    found = []
    iterator_type = getattr(unreal, "ObjectIterator", None)
    if iterator_type is None:
        return found
    for obj in iterator_type():
        try:
            path = obj.get_path_name()
            class_name = obj.get_class().get_name()
        except Exception:
            continue
        if "BP_GridGenerator" in path and "K2Node" in class_name:
            found.append(obj)
    return found


def patch_random_max(graph, last_index):
    nodes = iter_grid_k2_nodes(graph)
    unreal.log("Candidate node count: {}".format(len(nodes)))
    patched = 0
    for node in nodes:
        class_name = node_class_name(node)
        member_name = function_member_name(node)
        if "RandomInteger" in class_name or "RandomInteger" in member_name or class_name == "K2Node_CallFunction":
            unreal.log("Node {} member={}".format(class_name, member_name))

        if "RandomIntegerInRange" not in member_name:
            continue

        max_pin = None
        for pin in get_pins(node):
            name = pin_name(pin)
            unreal.log("  pin {} default={} linked={}".format(
                name,
                pin.get_editor_property("default_value"),
                pin.get_editor_property("linked_to"),
            ))
            if name == "Max":
                max_pin = pin
        if max_pin is None:
            continue

        old_value = str(max_pin.get_editor_property("default_value"))
        linked = list(max_pin.get_editor_property("linked_to") or [])
        if linked:
            unreal.log("RandomIntegerInRange Max is already wired (old default {}).".format(old_value))
            continue

        max_pin.set_editor_property("default_value", str(last_index))
        unreal.log("Patched RandomIntegerInRange Max {} -> {}".format(old_value, last_index))
        patched += 1
    return patched


safe_grid_class = unreal.load_class(None, "/Script/Towerdefense_GADE.SafeGridGenerator")
if safe_grid_class is None:
    raise RuntimeError("SafeGridGenerator C++ class was not found. Compile the game module first.")

blueprint = EAL.load_asset(GRID_PATH)
if blueprint is None:
    raise RuntimeError("Missing asset: " + GRID_PATH)

BEL.reparent_blueprint(blueprint, safe_grid_class)
unreal.log("Reparented BP_GridGenerator -> SafeGridGenerator")

compile_and_save(blueprint, GRID_PATH)

try:
    enemy_count = find_enemy_class_count(blueprint)
except Exception as exc:
    unreal.log_warning("Could not read Enemyclasses length: {}".format(exc))
    enemy_count = 0
if enemy_count <= 0:
    last_index = 2
    unreal.log_warning("Enemyclasses length unknown; falling back to Max=2")
else:
    last_index = enemy_count - 1
    unreal.log("Enemyclasses length {} -> last valid index {}".format(enemy_count, last_index))

graph = get_ubergraph(blueprint)
if graph is None:
    unreal.log_error("Could not load BP_GridGenerator EventGraph.")
    raise RuntimeError("Missing EventGraph")
unreal.log("Using graph " + str(graph.get_path_name()))
patched = patch_random_max(graph, last_index)
if patched == 0:
    unreal.log_warning("Did not find an unlinked RandomIntegerInRange Max pin to patch.")

compile_and_save(blueprint, GRID_PATH)
unreal.log("fix_grid_generator.py finished")
