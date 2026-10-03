import unreal
import os

STAGE = "C:/Users/Corentin/AppData/Local/Temp/opencode/assets/stage"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def import_fbx(filename, dest_path, dest_name):
    full_dest = "{}/{}".format(dest_path, dest_name)
    if unreal.EditorAssetLibrary.does_asset_exist(full_dest):
        unreal.log("Skip (exists): {}".format(full_dest))
        return unreal.load_asset(full_dest)

    ensure_dir(dest_path)

    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = dest_path
    task.destination_name = dest_name
    task.automated = True
    task.save = True
    task.replace_existing = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = True
    options.import_textures = True
    options.import_animations = False
    options.create_physics_asset = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH

    smd = options.static_mesh_import_data
    smd.combine_meshes = True
    smd.generate_lightmap_u_vs = False
    smd.auto_generate_collision = True

    task.options = options

    asset_tools.import_asset_tasks([task])

    if unreal.EditorAssetLibrary.does_asset_exist(full_dest):
        mesh = unreal.load_asset(full_dest)
        try:
            origin, extent = mesh.get_bounding_box().get_bounds() if hasattr(mesh, "get_bounding_box") else (None, None)
            unreal.log("Imported {} -> extent ~ {}".format(full_dest, extent))
        except Exception as e:
            unreal.log("Imported {} (size query skipped: {})".format(full_dest, e))
        return mesh
    else:
        unreal.log_error("FAILED to import: {}".format(full_dest))
        return None


def import_sound(filename, dest_path, dest_name):
    full_dest = "{}/{}".format(dest_path, dest_name)
    if unreal.EditorAssetLibrary.does_asset_exist(full_dest):
        unreal.log("Skip (exists): {}".format(full_dest))
        return

    ensure_dir(dest_path)

    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = dest_path
    task.destination_name = dest_name
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.factory = unreal.SoundFactory()

    asset_tools.import_asset_tasks([task])

    if unreal.EditorAssetLibrary.does_asset_exist(full_dest):
        unreal.log("Imported sound -> {}".format(full_dest))
    else:
        unreal.log_error("FAILED to import sound: {}".format(full_dest))


unreal.log("=== Farm Simulator: importing 3D assets & music ===")

CROPS = "/Game/Assets/Crops"
DECOR = "/Game/Assets/Decoration"
BUILD = "/Game/Assets/Building"
AUDIO = "/Game/Audio"

crop_files = [
    "crops_dirtSingle", "crop_carrot", "crop_pumpkin", "crop_turnip",
    "crops_cornStageA", "crops_cornStageB", "crops_cornStageC", "crops_cornStageD",
    "crops_wheatStageA", "crops_wheatStageB",
    "crops_leafsStageA", "crops_leafsStageB",
    "strawberry",
]
for name in crop_files:
    import_fbx("{}/Crops/{}.fbx".format(STAGE, name), CROPS, name)

decor_files = ["fence_simple", "fence_gate", "tree_default", "tree_pineRoundA", "flower_redA", "flower_yellowA", "rock_smallA"]
for name in decor_files:
    import_fbx("{}/Decoration/{}.fbx".format(STAGE, name), DECOR, name)

import_fbx("{}/Building/tent_detailedOpen.fbx".format(STAGE), BUILD, "tent_detailedOpen")

import_sound("{}/Audio/bgm_farm_relaxed.wav".format(STAGE), AUDIO, "BGM_Farm_Relaxed")

unreal.log("=== DONE ===")

unreal.SystemLibrary.quit_editor()
