import unreal

def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)
        unreal.log("Created directory: {}".format(path))


def create_base_material():
    package_path = "/Game/Materials"
    ensure_dir(package_path)
    asset_name = "M_Base_Color"
    full_path = "{}/{}".format(package_path, asset_name)

    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        unreal.log("Material already exists: {}".format(full_path))
        return

    factory = unreal.MaterialFactoryNew()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = asset_tools.create_asset(asset_name, package_path, unreal.Material, factory)

    color_param = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -400, -50)
    color_param.set_editor_property("parameter_name", "Color")
    color_param.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))

    roughness_param = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -400, 150)
    roughness_param.set_editor_property("parameter_name", "Roughness")
    roughness_param.set_editor_property("default_value", 0.85)

    unreal.MaterialEditingLibrary.connect_material_property(
        color_param, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(full_path)
    unreal.log("Created material: {}".format(full_path))


def create_empty_level():
    maps_path = "/Game/Maps"
    ensure_dir(maps_path)
    level_path = "{}/Map_Farm".format(maps_path)

    if unreal.EditorAssetLibrary.does_asset_exist(level_path):
        unreal.log("Level already exists: {}".format(level_path))
        return

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    result = les.new_level(level_path)
    unreal.log("new_level result: {}".format(result))


unreal.log("=== Farm Simulator asset setup: START ===")
create_base_material()
create_empty_level()
unreal.log("=== Farm Simulator asset setup: DONE ===")
