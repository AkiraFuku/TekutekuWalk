import bpy

bl_info = {
    "name": "レベルエディタ",
    "author": "Fuku Akira",
    "version": (1, 2),
    "blender": (3, 3, 1),
    "location": "TopBar > MyMenu / Object Properties",
    "description": "ステージエディタ（地形グリッド生成・OBJ変換・レール自動吸着・シーン入出力対応）",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object",
}

from .stretch_vertex import MYADDON_OT_stretch_vertex
from .object_type import (
    OBJECT_PT_type_settings,
    MYADDON_OT_init_object_type,
    MYADDON_OT_set_object_type,
    MYADDON_OT_apply_all_preview_models,
)
from .colider import (
    MYADDON_OT_add_collider,
    MYADDON_OT_snap_to_ground,
    MYADDON_OT_snap_all_to_ground,
    OBJECT_PT_collider,
    DrawCollider,
    ground_auto_snap_handler,
)
from .operater import (
    MYADDON_OT_add_behavior_tree,
    MYADDON_OT_open_behavior_editor_window,
    OBJECT_PT_behavior_tree,
    OBJECT_PT_enemy_spawn_settings,
    OBJECT_PT_file_name,
    MYADDON_OT_add_filwname,
    MYADDON_OT_ICO_sphere,
    MYADDON_OT_add_slope_block,
    MYADDON_OT_add_stairs_block,
    MYADDON_OT_add_prefab_block,
    MYADDON_OT_set_enemy_type,
    MYADDON_OT_add_enemy,
)
from .export_scene import MYADDON_OT_export_scene
from .import_scene import MYADDON_OT_import_scene
from .terrain_generator import (
    MYADDON_OT_generate_terrain,
    MYADDON_OT_convert_obj_to_terrain,
    MYADDON_OT_deform_terrain_to_rail,
    MYADDON_OT_enter_terrain_sculpt,
    MYADDON_OT_draw_rail_mode,
    MYADDON_OT_carve_prism_depression,
)
from .road_generator import (
    MYADDON_OT_generate_road_along_rail,
)
from .stage_generator import (
    MYADDON_OT_ai_generate_stage,
)
from .rail_snap import (
    MYADDON_OT_snap_to_rail,
    MYADDON_OT_calculate_rail_pos,
    MYADDON_OT_add_rail_pos,
    MYADDON_OT_sync_camera_rail,
    MYADDON_OT_unlink_camera_rail,
    MYADDON_OT_set_interpolation_type,
    MYADDON_OT_set_rail_mode,
    MYADDON_OT_make_rail_arc,
    MYADDON_OT_enter_rail_draw_tool,
    MYADDON_OT_extend_rail_end,
    MYADDON_OT_subdivide_rail_segment,
    MYADDON_OT_create_stage_rail,
    MYADDON_OT_create_preset_rail,
    OBJECT_PT_rail_curve_settings,
    OBJECT_PT_rail_position_settings,
    rail_magnet_snap_handler,
    register_rail_snap,
    unregister_rail_snap,
)


class TOPBAR_MT_my_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "MyMenu"
    bl_description = "拡張メニュー by " + bl_info["author"]

    def draw(self, context):
        self.layout.operator(MYADDON_OT_stretch_vertex.bl_idname, text=MYADDON_OT_stretch_vertex.bl_label)
        self.layout.operator(MYADDON_OT_ICO_sphere.bl_idname, text=MYADDON_OT_ICO_sphere.bl_label)
        self.layout.operator(MYADDON_OT_export_scene.bl_idname, text=MYADDON_OT_export_scene.bl_label)
        self.layout.operator(MYADDON_OT_import_scene.bl_idname, text=MYADDON_OT_import_scene.bl_label, icon='IMPORT')
        self.layout.separator()
        self.layout.operator(MYADDON_OT_create_stage_rail.bl_idname, text="🛤️ ステージレールを新規作成", icon='CURVE_BEZCURVE')
        self.layout.operator(MYADDON_OT_create_preset_rail.bl_idname, text="🔄 レールプリセット生成 (円形/直線/S字)", icon='CURVE_PATH')
        self.layout.separator()
        self.layout.operator(MYADDON_OT_ai_generate_stage.bl_idname, text="🎲 ランダムステージ自動生成・伸長", icon='PLAY')
        self.layout.separator()
        self.layout.operator(MYADDON_OT_apply_all_preview_models.bl_idname, text=MYADDON_OT_apply_all_preview_models.bl_label, icon='MESH_DATA')
        self.layout.separator()
        self.layout.operator(MYADDON_OT_deform_terrain_to_rail.bl_idname, text="レール沿いに地面を変形して道をつくる", icon='MOD_SMOOTH')
        self.layout.operator(MYADDON_OT_draw_rail_mode.bl_idname, text="ペンでなぞってレールを描く", icon='GREASEPENCIL')
        self.layout.operator(MYADDON_OT_enter_terrain_sculpt.bl_idname, text="なぞって地形変形 (スカルプト)", icon='SCULPTMODE_HLT')
        self.layout.separator()
        self.layout.operator(MYADDON_OT_snap_all_to_ground.bl_idname, text="全オブジェクトの地面埋まりを一括解消", icon='SNAP_ON')
        self.layout.separator()
        self.layout.operator(MYADDON_OT_generate_terrain.bl_idname, text=MYADDON_OT_generate_terrain.bl_label, icon='MESH_GRID')
        self.layout.operator(MYADDON_OT_convert_obj_to_terrain.bl_idname, text=MYADDON_OT_convert_obj_to_terrain.bl_label, icon='MOD_OCEAN')
        self.layout.separator()
        self.layout.operator(MYADDON_OT_generate_road_along_rail.bl_idname, text="（補助）独立道路メッシュを生成", icon='CURVE_PATH')
        self.layout.separator()
        self.layout.operator(MYADDON_OT_add_slope_block.bl_idname, text=MYADDON_OT_add_slope_block.bl_label, icon='MOD_SOLIDIFY')
        self.layout.operator(MYADDON_OT_add_stairs_block.bl_idname, text=MYADDON_OT_add_stairs_block.bl_label, icon='MOD_BEVEL')
        self.layout.separator()
        row_e = self.layout.row(align=True)
        op_e1 = row_e.operator(MYADDON_OT_add_enemy.bl_idname, text="通常敵 (Normal)", icon='MESH_CYLINDER')
        op_e1.enemy_type = 'Normal'
        op_e2 = row_e.operator(MYADDON_OT_add_enemy.bl_idname, text="バウンド敵 (Bound)", icon='IPO_BOUNCE')
        op_e2.enemy_type = 'Bound'
        op_e3 = row_e.operator(MYADDON_OT_add_enemy.bl_idname, text="追跡敵 (Chase - taru3)", icon='TRACKING')
        op_e3.enemy_type = 'Chase'
        self.layout.separator()
        self.layout.operator(MYADDON_OT_sync_camera_rail.bl_idname, text=MYADDON_OT_sync_camera_rail.bl_label, icon='CAMERA_DATA')
        self.layout.operator(MYADDON_OT_enter_rail_draw_tool.bl_idname, text="カーブペン描画ツール起動", icon='EDITMODE_HLT')

def draw_my_menu(self, context):
    self.layout.menu(TOPBAR_MT_my_menu.bl_idname)


classes = (
    TOPBAR_MT_my_menu,
    MYADDON_OT_ai_generate_stage,
    MYADDON_OT_export_scene,
    MYADDON_OT_import_scene,
    MYADDON_OT_ICO_sphere,
    MYADDON_OT_add_slope_block,
    MYADDON_OT_add_stairs_block,
    MYADDON_OT_add_prefab_block,
    MYADDON_OT_set_enemy_type,
    MYADDON_OT_add_enemy,
    MYADDON_OT_stretch_vertex,
    MYADDON_OT_add_filwname,
    OBJECT_PT_file_name,
    MYADDON_OT_add_collider,
    MYADDON_OT_snap_to_ground,
    MYADDON_OT_snap_all_to_ground,
    OBJECT_PT_collider,
    MYADDON_OT_add_behavior_tree,
    MYADDON_OT_open_behavior_editor_window,
    OBJECT_PT_behavior_tree,
    OBJECT_PT_enemy_spawn_settings,
    OBJECT_PT_type_settings,
    MYADDON_OT_init_object_type,
    MYADDON_OT_set_object_type,
    MYADDON_OT_apply_all_preview_models,
    MYADDON_OT_generate_terrain,
    MYADDON_OT_convert_obj_to_terrain,
    MYADDON_OT_deform_terrain_to_rail,
    MYADDON_OT_enter_terrain_sculpt,
    MYADDON_OT_draw_rail_mode,
    MYADDON_OT_carve_prism_depression,
    MYADDON_OT_generate_road_along_rail,
    MYADDON_OT_snap_to_rail,
    MYADDON_OT_calculate_rail_pos,
    MYADDON_OT_add_rail_pos,
    MYADDON_OT_sync_camera_rail,
    MYADDON_OT_unlink_camera_rail,
    MYADDON_OT_set_interpolation_type,
    MYADDON_OT_set_rail_mode,
    MYADDON_OT_make_rail_arc,
    MYADDON_OT_enter_rail_draw_tool,
    MYADDON_OT_extend_rail_end,
    MYADDON_OT_subdivide_rail_segment,
    MYADDON_OT_create_stage_rail,
    MYADDON_OT_create_preset_rail,
    OBJECT_PT_rail_curve_settings,
    OBJECT_PT_rail_position_settings,
)


def register():
    for cls in classes:
        try:
            bpy.utils.register_class(cls)
        except ValueError:
            pass

    # シーンプロパティ初期化（コライダー・レイキャスト・自動接地）
    bpy.types.Scene.draw_collider_always = bpy.props.BoolProperty(
        name="コライダー自動表示",
        description="3Dビューポートにコライダーのワイヤーフレームを自動常時表示します（埋まっている場合は赤色で警告）",
        default=True,
    )
    bpy.types.Scene.draw_raycast_always = bpy.props.BoolProperty(
        name="レイキャスト自動表示",
        description="足元から地面表面へのレイキャストと接地マーカーを自動常時表示します",
        default=True,
    )
    bpy.types.Scene.auto_ground_snap = bpy.props.BoolProperty(
        name="自動地面接地（埋まり防止）",
        description="オブジェクト移動時に地面メッシュを検知し、地面に埋まらないよう自動的に表面にピッタリ乗せます",
        default=True,
    )

    if ground_auto_snap_handler not in bpy.app.handlers.depsgraph_update_post:
        bpy.app.handlers.depsgraph_update_post.append(ground_auto_snap_handler)

    # トップバーメニューに追加
    try:
        bpy.types.TOPBAR_MT_editor_menus.remove(draw_my_menu)
    except Exception:
        pass
    try:
        bpy.types.TOPBAR_MT_editor_menus.append(draw_my_menu)
    except Exception:
        pass

    # 3Dビューポートのヘッダーメニューにも追加（見失い防止）
    try:
        bpy.types.VIEW3D_MT_editor_menus.remove(draw_my_menu)
    except Exception:
        pass
    try:
        bpy.types.VIEW3D_MT_editor_menus.append(draw_my_menu)
    except Exception:
        pass

    if DrawCollider.handle is None:
        DrawCollider.handle = bpy.types.SpaceView3D.draw_handler_add(
            DrawCollider.draw_colider, (), "WINDOW", "POST_VIEW"
        )

    register_rail_snap()
    print("レベルエディタが有効化されました。（コライダー・レイキャスト自動表示＆地面埋まり防止対応）")


def unregister():
    unregister_rail_snap()

    if DrawCollider.handle:
        bpy.types.SpaceView3D.draw_handler_remove(DrawCollider.handle, "WINDOW")
        DrawCollider.handle = None

    if ground_auto_snap_handler in bpy.app.handlers.depsgraph_update_post:
        bpy.app.handlers.depsgraph_update_post.remove(ground_auto_snap_handler)

    for prop in ["draw_collider_always", "draw_raycast_always", "auto_ground_snap"]:
        if hasattr(bpy.types.Scene, prop):
            delattr(bpy.types.Scene, prop)

    try:
        bpy.types.TOPBAR_MT_editor_menus.remove(draw_my_menu)
    except Exception:
        pass
    try:
        bpy.types.VIEW3D_MT_editor_menus.remove(draw_my_menu)
    except Exception:
        pass

    for cls in reversed(classes):
        try:
            bpy.utils.unregister_class(cls)
        except Exception:
            pass
    print("レベルエディタが無効化されました。")


if __name__ == "__main__":
    try:
        unregister()
    except Exception:
        pass
    register()