import bpy
import math
import mathutils
import os

# ==========================================
# コース地形自動生成モジュール（terrain_grid 版）
#
# StageRail のベジェ曲線コースに合わせて、
# ゲーム側の動的地形生成機能（terrain_grid）用の
# TERRAINオブジェクトをBlenderシーンに自動配置する。
#
# ゲーム側の動作:
#   file_name == "terrain_grid" の TERRAIN が読み込まれると
#   StageBlock::Initialize() が ModelManager::CreateTerrainModel() を呼び出し、
#   properties の size_x/size_y/divisions_x/divisions_y/uv_tile を使って
#   頂点データを動的生成する。
# ==========================================

_TERRAIN_OBJECT_NAME = "TerrainGround"


# ------------------------------------------
# ベジェ補間ユーティリティ
# ------------------------------------------

def _evaluate_bezier(p0, p1, p2, p3, t):
    """3次ベジェ曲線の補間点 (mathutils.Vector)"""
    u = 1.0 - t
    return (u**3) * p0 + 3.0*(u**2)*t*p1 + 3.0*u*(t**2)*p2 + (t**3)*p3


def _get_stage_rail():
    """シーン内の StageRail (STAGE_RAIL) カーブを取得"""
    for name in ("StageRail", "stageRail", "stage_rail"):
        if name in bpy.data.objects:
            obj = bpy.data.objects[name]
            if obj.type == "CURVE":
                return obj
    for obj in bpy.data.objects:
        if obj.type == "CURVE" and obj.get("object_type") == "STAGE_RAIL":
            return obj
    for obj in bpy.data.objects:
        if obj.type == "CURVE":
            return obj
    return None


def _sample_curve_points(curve_obj, num_samples=64):
    """
    カーブ上を num_samples 点でサンプリングしてワールド座標リストを返す。
    """
    spline = curve_obj.data.splines[0]
    points = spline.bezier_points
    num_pts = len(points)
    is_loop = spline.use_cyclic_u
    num_segs = num_pts if is_loop else (num_pts - 1)
    if num_segs <= 0:
        return []

    mat = curve_obj.matrix_world
    sampled = []

    for step in range(num_samples):
        global_t = (float(step) / float(num_samples)) * float(num_segs)

        if is_loop:
            global_t = global_t % float(num_segs)
        else:
            global_t = min(global_t, float(num_segs))

        idx1 = int(global_t)
        if is_loop:
            idx2 = (idx1 + 1) % num_pts
        else:
            idx1 = min(idx1, num_pts - 2)
            idx2 = idx1 + 1

        local_t = global_t - float(idx1)
        bp1 = points[idx1]
        bp2 = points[idx2]

        p0 = mat @ bp1.co
        p1 = mat @ bp1.handle_right
        p2 = mat @ bp2.handle_left
        p3 = mat @ bp2.co

        pos = _evaluate_bezier(p0, p1, p2, p3, local_t)
        sampled.append(pos)

    return sampled


def _compute_rail_bounds(sampled_points):
    """
    サンプリング点から XY（Blender XY = ゲームの XZ）の
    バウンディングボックス中心・サイズを計算して返す。
    returns: (center_x, center_y, span_x, span_y)
    """
    if not sampled_points:
        return (0.0, 0.0, 100.0, 100.0)

    xs = [p.x for p in sampled_points]
    ys = [p.y for p in sampled_points]

    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)

    cx = (min_x + max_x) * 0.5
    cy = (min_y + max_y) * 0.5
    sx = max_x - min_x
    sy = max_y - min_y

    return cx, cy, sx, sy


# ------------------------------------------
# Blender プレビューメッシュの生成
# ------------------------------------------

# ------------------------------------------
# テクスチャ・マテリアル ユーティリティ
# ------------------------------------------

def _resolve_texture_path(rel_or_abs_path):
    """テクスチャパスの絶対パスを解決"""
    if not rel_or_abs_path:
        return ""
    if os.path.isabs(rel_or_abs_path) and os.path.exists(rel_or_abs_path):
        return rel_or_abs_path

    # プロジェクトルートからの相対パス探索
    try:
        cur_file = os.path.abspath(__file__)
        editer_dir = os.path.dirname(os.path.dirname(cur_file))
        project_root = os.path.dirname(editer_dir)
        cand = os.path.join(project_root, rel_or_abs_path)
        if os.path.exists(cand):
            return cand
    except Exception:
        pass

    fixed_root = r"c:\Users\k024g\OneDrive\デスクトップ\study\3nenn\AL\Project\project"
    cand = os.path.join(fixed_root, rel_or_abs_path)
    if os.path.exists(cand):
        return cand

    return ""


def _get_or_create_terrain_material(mat_name, texture_path):
    """テクスチャ画像を貼った Principled BSDF マテリアルを作成・取得"""
    if mat_name in bpy.data.materials:
        return bpy.data.materials[mat_name]

    mat = bpy.data.materials.new(name=mat_name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links

    bsdf = nodes.get("Principled BSDF")
    full_path = _resolve_texture_path(texture_path)
    if bsdf and full_path and os.path.exists(full_path):
        try:
            img = bpy.data.images.load(full_path, check_existing=True)
            tex_node = nodes.new(type="ShaderNodeTexImage")
            tex_node.image = img
            links.new(tex_node.outputs["Color"], bsdf.inputs["Base Color"])
        except Exception as e:
            print(f"[TerrainGen] テクスチャ読み込みエラー {full_path}: {e}")

    return mat


# ------------------------------------------
# Blender プレビューメッシュの生成
# ------------------------------------------

def _create_grid_mesh_data(
    mesh_name,
    size_x=100.0,
    size_y=100.0,
    div_x=20,
    div_y=20,
    texture_path="resources/Stagemap/863603.png",
    uv_tile=4.0,
):
    """
    面・UVタイリング・テクスチャマテリアル付きのグリッドメッシュデータ (bpy.types.Mesh) を生成。
    ローカル原点 (0, 0, 0) を中心とし、Z=0 の水平面。
    """
    mesh = bpy.data.meshes.new(mesh_name)

    verts = []
    faces = []
    div_x = max(int(div_x), 1)
    div_y = max(int(div_y), 1)
    step_x = size_x / div_x
    step_y = size_y / div_y
    start_x = -size_x * 0.5
    start_y = -size_y * 0.5

    # 頂点生成（ローカル座標系: Z=0 中心）
    for iy in range(div_y + 1):
        for ix in range(div_x + 1):
            vx = start_x + ix * step_x
            vy = start_y + iy * step_y
            verts.append((vx, vy, 0.0))

    # 面生成（反時計回り: Z+ 上向き法線）
    for iy in range(div_y):
        for ix in range(div_x):
            i0 = iy * (div_x + 1) + ix
            i1 = i0 + 1
            i2 = i0 + (div_x + 1)
            i3 = i2 + 1
            faces.append((i0, i1, i3, i2))

    mesh.from_pydata(verts, [], faces)
    mesh.validate(verbose=False)

    # UV座標の設定（タイリング反映）
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for poly in mesh.polygons:
        for loop_idx in poly.loop_indices:
            vert_idx = mesh.loops[loop_idx].vertex_index
            ix = vert_idx % (div_x + 1)
            iy = vert_idx // (div_x + 1)
            u = (ix / div_x) * uv_tile
            v = (iy / div_y) * uv_tile
            uv_layer.data[loop_idx].uv = (u, v)

    # 法線を更新し、上面（Z+ > 0）が表面になるよう保証
    mesh.update(calc_edges=True)
    if mesh.polygons and mesh.polygons[0].normal.z < 0:
        mesh.flip_normals()
        mesh.update()

    # テクスチャマテリアル適用
    if texture_path:
        mat_name = f"Mat_{mesh_name}"
        mat = _get_or_create_terrain_material(mat_name, texture_path)
        if mat:
            if not mesh.materials:
                mesh.materials.append(mat)
            else:
                mesh.materials[0] = mat

    return mesh


def _create_preview_grid_mesh(
    name,
    center_x=0.0,
    center_y=0.0,
    floor_z=0.0,
    size_x=100.0,
    size_y=100.0,
    div_x=20,
    div_y=20,
    texture_path="resources/Stagemap/863603.png",
    uv_tile=4.0,
    parent=None,
):
    """プレビュー用メッシュオブジェクトを生成（互換用）"""
    if name in bpy.data.objects:
        old = bpy.data.objects[name]
        bpy.data.objects.remove(old, do_unlink=True)

    mesh = _create_grid_mesh_data(
        name + "_mesh",
        size_x=size_x, size_y=size_y,
        div_x=div_x, div_y=div_y,
        texture_path=texture_path,
        uv_tile=uv_tile,
    )
    obj = bpy.data.objects.new(name, mesh)
    obj.show_wire = True
    bpy.context.collection.objects.link(obj)

    if parent:
        obj.parent = parent
        obj.location = (0.0, 0.0, 0.0)
    else:
        obj.location = (center_x, center_y, floor_z)

    return obj


# ------------------------------------------
# メイン生成関数
# ------------------------------------------

def _find_existing_terrain_objects(target_name=_TERRAIN_OBJECT_NAME):
    """
    シーン内から既存の地形オブジェクトを網羅的に検出して返す。
    1. 名前が target_name または target_name で始まるオブジェクト (例: "TerrainGround", "TerrainGround.001")
    2. object_type == "TERRAIN" または file_name == "terrain_grid" のオブジェクト
    3. プレビューオブジェクト ("TerrainGround_Preview")
    """
    found = []
    for obj in bpy.data.objects:
        if obj.name == target_name or obj.name.startswith(target_name + "."):
            if obj not in found:
                found.append(obj)
        elif obj.name.startswith(target_name + "_Preview"):
            if obj not in found:
                found.append(obj)
        elif obj.get("object_type") == "TERRAIN" or obj.get("file_name") == "terrain_grid":
            if obj not in found:
                found.append(obj)
    return found


def generate_terrain_grid(
    margin=10.0,
    divisions_per_unit=2.0,
    max_divisions=64,
    floor_z_offset=-0.5,
    texture_path="resources/Stagemap/863603.png",
    uv_tile=4.0,
    terrain_name=_TERRAIN_OBJECT_NAME,
    create_preview=True,
):
    """
    StageRail の形状を読み取り、コースを包む terrain_grid TERRAIN オブジェクトを
    Blenderシーンに直接メッシュオブジェクト（MESH）として配置する。
    既存の地形オブジェクトが存在する場合は安全に上書き更新する。
    """
    try:
        if bpy.context.mode != 'OBJECT':
            try:
                bpy.ops.object.mode_set(mode='OBJECT')
            except Exception:
                pass

        rail = _get_stage_rail()
        if rail is None:
            return False, "StageRail が見つかりません。STAGE_RAIL タイプのカーブをシーンに配置してください。"

        sampled = _sample_curve_points(rail, num_samples=128)
        if not sampled:
            return False, "カーブのサンプリングに失敗しました。"

        cx, cy, raw_sx, raw_sy = _compute_rail_bounds(sampled)

        size_x = max(raw_sx + margin * 2.0, 10.0)
        size_y = max(raw_sy + margin * 2.0, 10.0)

        div_x = min(int(size_x * divisions_per_unit), max_divisions)
        div_y = min(int(size_y * divisions_per_unit), max_divisions)
        div_x = max(div_x, 4)
        div_y = max(div_y, 4)

        # 既存の地形オブジェクトを検出（上書き対象）
        existing_terrains = _find_existing_terrain_objects(terrain_name)
        primary_obj = None
        target_col = None

        if existing_terrains:
            # MESHタイプのオブジェクトがあれば最優先で上書き対象にする
            mesh_terrains = [o for o in existing_terrains if o.type == 'MESH']
            if mesh_terrains:
                primary_obj = mesh_terrains[0]
            else:
                primary_obj = existing_terrains[0]

            if primary_obj.users_collection:
                target_col = primary_obj.users_collection[0]

            # primary_obj 以外の余分な重複オブジェクトは安全にクリーンアップ
            for extra_obj in existing_terrains:
                if extra_obj != primary_obj and extra_obj.name in bpy.data.objects:
                    try:
                        bpy.data.objects.remove(extra_obj, do_unlink=True)
                    except Exception:
                        pass

        # ─── TERRAIN メッシュオブジェクトを生成 ───
        t_mesh = _create_grid_mesh_data(
            f"Mesh_{terrain_name}",
            size_x=size_x, size_y=size_y,
            div_x=min(div_x, 32), div_y=min(div_y, 32),
            texture_path=texture_path,
            uv_tile=uv_tile,
        )

        obj = None
        is_overwrite = False
        if primary_obj and primary_obj.name in bpy.data.objects:
            if primary_obj.type == 'MESH':
                # 既存の MESH オブジェクトを安全に上書き
                old_mesh = primary_obj.data
                primary_obj.data = t_mesh
                primary_obj.name = terrain_name
                primary_obj.show_wire = True
                primary_obj.location = (cx, cy, floor_z_offset)
                primary_obj.rotation_euler = (0.0, 0.0, 0.0)
                primary_obj.scale = (1.0, 1.0, 1.0)
                obj = primary_obj
                is_overwrite = True

                # 孤立した古いメッシュを安全に削除
                if old_mesh and old_mesh != t_mesh and old_mesh.users == 0:
                    try:
                        bpy.data.meshes.remove(old_mesh, do_unlink=True)
                    except Exception:
                        pass
            else:
                # EMPTY 等の場合は古いオブジェクトを削除し、同じコレクションに MESH を再作成
                if primary_obj.users_collection:
                    target_col = primary_obj.users_collection[0]
                try:
                    bpy.data.objects.remove(primary_obj, do_unlink=True)
                except Exception:
                    pass
                is_overwrite = True

        if obj is None:
            # 新規作成（またはEMPTYからMESHへの再作成）
            obj = bpy.data.objects.new(terrain_name, t_mesh)
            obj.show_wire = True
            obj.location = (cx, cy, floor_z_offset)
            col = target_col or bpy.context.collection
            col.objects.link(obj)

        # ゲームに渡すカスタムプロパティを設定
        obj["object_type"]      = "TERRAIN"
        obj["file_name"]        = "terrain_grid"
        obj["texture"]          = texture_path
        obj["prop_size_x"]      = str(round(size_x, 2))
        obj["prop_size_y"]      = str(round(size_y, 2))
        obj["prop_divisions_x"] = str(div_x)
        obj["prop_divisions_y"] = str(div_y)
        obj["prop_uv_tile"]     = str(round(uv_tile, 2))

        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)

        msg = (
            f"地形{'上書き更新' if is_overwrite else '生成'}完了！\n"
            f"  サイズ: {size_x:.1f} x {size_y:.1f} m\n"
            f"  分割数: {div_x} x {div_y}\n"
            f"  中心: ({cx:.1f}, {cy:.1f}), 高さ(Z): {floor_z_offset:.2f}\n"
            f"  テクスチャ: {texture_path}"
        )
        print(f"[TerrainGen] {msg}")
        return True, msg

    except Exception as e:
        import traceback
        traceback.print_exc()
        return False, f"地形生成処理中にエラーが発生しました: {e}"


# ==========================================
# Blender Operator
# ==========================================

class MYADDON_OT_generate_terrain(bpy.types.Operator):
    bl_idname = "myaddon.generate_terrain"
    bl_label = "コース地形を自動生成"
    bl_description = (
        "StageRail のコース形状を読み取り、"
        "ゲーム側の動的地形生成（terrain_grid）用 TERRAIN オブジェクトをシーンに配置します"
    )
    bl_options = {"REGISTER", "UNDO"}

    margin: bpy.props.FloatProperty(
        name="マージン（m）",
        description="コースのバウンディングボックスに付加するマージン",
        default=10.0,
        min=0.0,
        max=100.0,
    )  # type: ignore

    divisions_per_unit: bpy.props.FloatProperty(
        name="密度（分割数/m）",
        description="地形の 1m あたりの分割数（大きいほど滑らか）",
        default=1.0,
        min=0.1,
        max=5.0,
    )  # type: ignore

    max_divisions: bpy.props.IntProperty(
        name="最大分割数",
        description="1軸あたりの分割数の上限",
        default=64,
        min=4,
        max=256,
    )  # type: ignore

    floor_z_offset: bpy.props.FloatProperty(
        name="地形の高さ（Z）",
        description="地形を置く高さ（Blender Z軸 = ゲームの Y軸）",
        default=-0.5,
        min=-50.0,
        max=50.0,
    )  # type: ignore

    texture_path: bpy.props.StringProperty(
        name="テクスチャパス",
        description="地形に貼るテクスチャ（ゲームプロジェクト相対パス）",
        default="resources/Stagemap/863603.png",
    )  # type: ignore

    uv_tile: bpy.props.FloatProperty(
        name="UVタイル倍率",
        description="テクスチャのタイリング回数",
        default=4.0,
        min=0.1,
        max=50.0,
    )  # type: ignore

    create_preview: bpy.props.BoolProperty(
        name="プレビューメッシュを表示",
        description="Blenderビューポートに地形サイズのワイヤーフレームグリッドを表示します",
        default=True,
    )  # type: ignore

    def invoke(self, context, event):
        # StageRail があれば事前にサイズを見積もってパラメータに反映
        from terrain_generator import _get_stage_rail, _sample_curve_points, _compute_rail_bounds
        rail = _get_stage_rail()
        if rail:
            sampled = _sample_curve_points(rail, 128)
            if sampled:
                _, _, sx, sy = _compute_rail_bounds(sampled)
                # 予想サイズをラベルに使うだけで変更はしない
                self._estimated_size = (sx + self.margin * 2, sy + self.margin * 2)
            else:
                self._estimated_size = None
        else:
            self._estimated_size = None
        return context.window_manager.invoke_props_dialog(self, width=400)

    def draw(self, context):
        layout = self.layout
        layout.label(text="=== terrain_grid 自動地形生成 ===", icon="MESH_GRID")

        col = layout.column(align=True)
        col.prop(self, "margin")
        col.prop(self, "divisions_per_unit")
        col.prop(self, "max_divisions")
        col.prop(self, "floor_z_offset")

        layout.separator()
        layout.prop(self, "texture_path", icon="TEXTURE")
        layout.prop(self, "uv_tile")

        layout.separator()
        layout.prop(self, "create_preview", icon="HIDE_OFF")

        layout.separator()
        box = layout.box()
        if hasattr(self, "_estimated_size") and self._estimated_size:
            sx, sy = self._estimated_size
            div_x = min(int(sx * self.divisions_per_unit), self.max_divisions)
            div_y = min(int(sy * self.divisions_per_unit), self.max_divisions)
            box.label(text=f"推定サイズ: {sx:.0f} x {sy:.0f} m", icon="INFO")
            box.label(text=f"推定分割数: {div_x} x {div_y} = {div_x*div_y} 頂点グリッド", icon="MESH_DATA")
        else:
            box.label(text="StageRail が見つかりません", icon="ERROR")

        layout.separator()
        box2 = layout.box()
        box2.label(text="ゲームの動作:", icon="PLAY")
        box2.label(text="  terrain_grid 検出 → CreateTerrainModel() が動的にメッシュを生成")
        box2.label(text="  プレイヤーの足場・壁判定として機能します")

    def execute(self, context):
        ok, msg = generate_terrain_grid(
            margin=self.margin,
            divisions_per_unit=self.divisions_per_unit,
            max_divisions=self.max_divisions,
            floor_z_offset=self.floor_z_offset,
            texture_path=self.texture_path,
            uv_tile=self.uv_tile,
            create_preview=self.create_preview,
        )
        if not ok:
            self.report({"ERROR"}, msg)
            return {"CANCELLED"}

        first_line = msg.split("\n")[0]
        self.report({"INFO"}, first_line)
        return {"FINISHED"}


# ==========================================
# メッシュ (OBJModel) → terrain_grid 変換
# ==========================================

def _compute_mesh_bounds(obj):
    """
    メッシュオブジェクトのワールド座標系におけるバウンディングボックスを計算。
    returns: (cx, cy, cz, sx, sy, sz, min_z, max_z) or None
    """
    if not obj or obj.type != 'MESH':
        return None
    try:
        mat = obj.matrix_world
        world_bbox = [mat @ mathutils.Vector(corner) for corner in obj.bound_box]
        xs = [v.x for v in world_bbox]
        ys = [v.y for v in world_bbox]
        zs = [v.z for v in world_bbox]
        min_x, max_x = min(xs), max(xs)
        min_y, max_y = min(ys), max(ys)
        min_z, max_z = min(zs), max(zs)
        cx = (min_x + max_x) * 0.5
        cy = (min_y + max_y) * 0.5
        cz = (min_z + max_z) * 0.5
        sx = max(max_x - min_x, 0.1)
        sy = max(max_y - min_y, 0.1)
        sz = max(max_z - min_z, 0.0)
        return (cx, cy, cz, sx, sy, sz, min_z, max_z)
    except Exception as e:
        print(f"[TerrainGen] バウンディングボックス計算エラー: {e}")
        return None


def _detect_mesh_texture(obj):
    """メッシュオブジェクトからテクスチャパスを推定"""
    if "texture" in obj and obj["texture"]:
        return str(obj["texture"])
    if obj.data and hasattr(obj.data, "materials"):
        for mat in obj.data.materials:
            if mat and mat.use_nodes and mat.node_tree:
                for node in mat.node_tree.nodes:
                    if node.type == 'TEX_IMAGE' and node.image and node.image.filepath:
                        raw = node.image.filepath.replace("\\", "/")
                        if raw.startswith("//"):
                            raw = raw[2:]
                        idx = raw.find("resources/")
                        if idx != -1:
                            return raw[idx:]
                        import os
                        return os.path.basename(raw)
    return "resources/Stagemap/863603.png"


def convert_mesh_to_terrain_grid(
    target_obj,
    margin=0.0,
    divisions_per_unit=1.0,
    max_divisions=64,
    floor_z_mode="ORIGIN",
    custom_floor_z=0.0,
    texture_path="",
    uv_tile=4.0,
    replace_mode="REPLACE",
    create_preview=True,
):
    """
    既存のメッシュオブジェクト（OBJModel等）から terrain_grid TERRAIN オブジェクトへ変換する。

    Parameters
    ----------
    target_obj        : 変換元のメッシュオブジェクト
    margin            : スパンに付加する余白（m）
    divisions_per_unit: 1mあたりの分割密度
    max_divisions     : 分割数の上限
    floor_z_mode      : "ORIGIN" (原点Z), "TOP" (上面), "BOTTOM" (下面), "CUSTOM" (指定値)
    custom_floor_z    : floor_z_mode == "CUSTOM" のときの高さ
    texture_path      : テクスチャパス（空なら自動検出）
    uv_tile           : UVタイリング倍率
    replace_mode      : "REPLACE" (元モデル削除), "HIDE" (元モデル非表示化), "KEEP" (残す)
    create_preview    : プレビュー用ワイヤーフレームを生成するか

    Returns
    -------
    (bool, str) = (成功フラグ, メッセージ)
    """
    if not target_obj or target_obj.type != 'MESH':
        return False, "変換対象のメッシュオブジェクトが選択されていません。"

    bounds = _compute_mesh_bounds(target_obj)
    if not bounds:
        return False, f"オブジェクト '{target_obj.name}' のバウンディングボックス計算に失敗しました。"

    cx, cy, cz, raw_sx, raw_sy, raw_sz, min_z, max_z = bounds

    # マージン付加
    size_x = max(raw_sx + margin * 2.0, 1.0)
    size_y = max(raw_sy + margin * 2.0, 1.0)

    # 分割数
    div_x = min(int(size_x * divisions_per_unit), max_divisions)
    div_y = min(int(size_y * divisions_per_unit), max_divisions)
    div_x = max(div_x, 4)
    div_y = max(div_y, 4)

    # 高さの決定
    if floor_z_mode == "ORIGIN":
        floor_z = target_obj.location.z
    elif floor_z_mode == "TOP":
        floor_z = max_z
    elif floor_z_mode == "BOTTOM":
        floor_z = min_z
    else:  # CUSTOM
        floor_z = custom_floor_z

    # テクスチャパス決定
    tex = texture_path.strip() if texture_path else _detect_mesh_texture(target_obj)

    # オブジェクト名決定
    original_name = target_obj.name
    if replace_mode == "REPLACE":
        terrain_name = original_name
        # 先に元オブジェクトを削除
        bpy.data.objects.remove(target_obj, do_unlink=True)
    elif replace_mode == "HIDE":
        terrain_name = original_name + "_TerrainGrid"
        target_obj.hide_viewport = True
        target_obj.hide_render = True
        target_obj["disabled"] = True
    else:  # KEEP
        terrain_name = original_name + "_TerrainGrid"

    # 既存の同名オブジェクトやプレビューがあれば削除
    if terrain_name in bpy.data.objects:
        bpy.data.objects.remove(bpy.data.objects[terrain_name], do_unlink=True)
    preview_name = terrain_name + "_Preview"
    if preview_name in bpy.data.objects:
        bpy.data.objects.remove(bpy.data.objects[preview_name], do_unlink=True)

    # ─── TERRAIN メッシュオブジェクトを直接生成 ───
    t_mesh = _create_grid_mesh_data(
        f"Mesh_{terrain_name}",
        size_x=size_x, size_y=size_y,
        div_x=min(div_x, 32), div_y=min(div_y, 32),
        texture_path=tex,
        uv_tile=uv_tile,
    )
    obj = bpy.data.objects.new(terrain_name, t_mesh)
    obj.show_wire = True
    obj.location = (cx, cy, floor_z)

    # カスタムプロパティ設定
    obj["object_type"]      = "TERRAIN"
    obj["file_name"]        = "terrain_grid"
    obj["texture"]          = tex
    obj["prop_size_x"]      = str(round(size_x, 2))
    obj["prop_size_y"]      = str(round(size_y, 2))
    obj["prop_divisions_x"] = str(div_x)
    obj["prop_divisions_y"] = str(div_y)
    obj["prop_uv_tile"]     = str(round(uv_tile, 2))

    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)

    msg = (
        f"OBJモデル '{original_name}' を地形グリッドに変換しました！\n"
        f"  地形オブジェクト名: {terrain_name}\n"
        f"  サイズ: {size_x:.1f} x {size_y:.1f} m (元: {raw_sx:.1f} x {raw_sy:.1f} m)\n"
        f"  分割数: {div_x} x {div_y}\n"
        f"  中心: ({cx:.1f}, {cy:.1f}), 高さ(Z): {floor_z:.2f}\n"
        f"  テクスチャ: {tex}"
    )
    print(f"[TerrainGen] {msg}")
    return True, msg


class MYADDON_OT_convert_obj_to_terrain(bpy.types.Operator):
    bl_idname = "myaddon.convert_obj_to_terrain"
    bl_label = "OBJモデルを地形グリッドに変換"
    bl_description = (
        "選択中のメッシュオブジェクト（OBJModel等）のサイズ・位置・テクスチャから、"
        "ゲーム側の動的地形生成（terrain_grid）用 TERRAIN オブジェクトへ変換します"
    )
    bl_options = {"REGISTER", "UNDO"}

    margin: bpy.props.FloatProperty(
        name="余白マージン（m）",
        description="元のメッシュ境界に付加する余白",
        default=0.0,
        min=0.0,
        max=100.0,
    )  # type: ignore

    divisions_per_unit: bpy.props.FloatProperty(
        name="密度（分割数/m）",
        description="地形の 1m あたりの分割数",
        default=1.0,
        min=0.1,
        max=5.0,
    )  # type: ignore

    max_divisions: bpy.props.IntProperty(
        name="最大分割数",
        description="1軸あたりの分割数の上限",
        default=64,
        min=4,
        max=256,
    )  # type: ignore

    floor_z_mode: bpy.props.EnumProperty(
        name="高さ基準（Z）",
        description="地形のZ座標（高さ）をどこに合わせるか",
        items=[
            ("ORIGIN", "原点 (Origin Z)", "メッシュオブジェクトの原点Z位置に合わせる"),
            ("TOP", "上面 (Top Z)", "メッシュの最高点Zに合わせる"),
            ("BOTTOM", "下面 (Bottom Z)", "メッシュの最底点Zに合わせる"),
            ("CUSTOM", "カスタム指定", "下の「カスタム高さ」で指定した値にする"),
        ],
        default="ORIGIN",
    )  # type: ignore

    custom_floor_z: bpy.props.FloatProperty(
        name="カスタム高さ（Z）",
        description="floor_z_mode が CUSTOM の場合の地形高さ",
        default=0.0,
        min=-100.0,
        max=100.0,
    )  # type: ignore

    texture_path: bpy.props.StringProperty(
        name="テクスチャパス",
        description="地形テクスチャの相対パス（空欄なら元モデルから自動取得）",
        default="",
    )  # type: ignore

    uv_tile: bpy.props.FloatProperty(
        name="UVタイル倍率",
        description="テクスチャのタイリング倍率",
        default=4.0,
        min=0.1,
        max=50.0,
    )  # type: ignore

    replace_mode: bpy.props.EnumProperty(
        name="元モデルの処理",
        description="変換後に元のOBJメッシュをどう処理するか",
        items=[
            ("REPLACE", "置換（元モデルを削除）", "元メッシュを削除し、同じ名前で地形グリッドを配置します（推奨）"),
            ("HIDE", "非表示化（バックアップ保持）", "元メッシュを非表示・disabled=Trueにし、別名で地形グリッドを配置します"),
            ("KEEP", "そのまま保持", "元メッシュをそのまま残し、別名で地形グリッドを配置します"),
        ],
        default="REPLACE",
    )  # type: ignore

    create_preview: bpy.props.BoolProperty(
        name="プレビューメッシュを表示",
        description="Blenderビューポートにワイヤーフレームのグリッドを表示します",
        default=True,
    )  # type: ignore

    @classmethod
    def poll(cls, context):
        return context.active_object is not None and context.active_object.type == 'MESH'

    def invoke(self, context, event):
        obj = context.active_object
        if not obj or obj.type != 'MESH':
            self.report({"ERROR"}, "メッシュオブジェクトを選択してください。")
            return {"CANCELLED"}

        bounds = _compute_mesh_bounds(obj)
        if bounds:
            cx, cy, cz, sx, sy, sz, min_z, max_z = bounds
            self._mesh_info = {
                "name": obj.name,
                "size_x": sx,
                "size_y": sy,
                "size_z": sz,
                "center": (cx, cy, cz),
                "min_z": min_z,
                "max_z": max_z,
                "origin_z": obj.location.z,
            }
        else:
            self._mesh_info = None

        # テクスチャを事前スキャン
        if not self.texture_path:
            self.texture_path = _detect_mesh_texture(obj)

        return context.window_manager.invoke_props_dialog(self, width=420)

    def draw(self, context):
        layout = self.layout
        layout.label(text="=== OBJModel → 地形グリッド変換 ===", icon="MOD_OCEAN")

        if hasattr(self, "_mesh_info") and self._mesh_info:
            info = self._mesh_info
            box_info = layout.box()
            box_info.label(text=f"元モデル: {info['name']}", icon="MESH_DATA")
            box_info.label(text=f"サイズ: 幅(X) {info['size_x']:.2f}m × 奥(Y) {info['size_y']:.2f}m × 高(Z) {info['size_z']:.2f}m")
            box_info.label(text=f"Z範囲: {info['min_z']:.2f}m ～ {info['max_z']:.2f}m (原点: {info['origin_z']:.2f}m)")

        col = layout.column(align=True)
        col.prop(self, "margin")
        col.prop(self, "divisions_per_unit")
        col.prop(self, "max_divisions")

        layout.separator()
        col_z = layout.column(align=True)
        col_z.prop(self, "floor_z_mode")
        if self.floor_z_mode == "CUSTOM":
            col_z.prop(self, "custom_floor_z")

        layout.separator()
        layout.prop(self, "texture_path", icon="TEXTURE")
        layout.prop(self, "uv_tile")

        layout.separator()
        layout.prop(self, "replace_mode", icon="OUTLINER_OB_MESH")
        layout.prop(self, "create_preview", icon="HIDE_OFF")

        # 変換後推定
        if hasattr(self, "_mesh_info") and self._mesh_info:
            info = self._mesh_info
            ex_sx = info['size_x'] + self.margin * 2.0
            ex_sy = info['size_y'] + self.margin * 2.0
            ex_dx = max(min(int(ex_sx * self.divisions_per_unit), self.max_divisions), 4)
            ex_dy = max(min(int(ex_sy * self.divisions_per_unit), self.max_divisions), 4)

            layout.separator()
            box_calc = layout.box()
            box_calc.label(text=f"変換後サイズ: {ex_sx:.1f} x {ex_sy:.1f} m", icon="INFO")
            box_calc.label(text=f"変換後分割数: {ex_dx} x {ex_dy} ({ex_dx*ex_dy} 頂点グリッド)", icon="GRID")

    def execute(self, context):
        obj = context.active_object
        if not obj or obj.type != 'MESH':
            self.report({"ERROR"}, "メッシュオブジェクトが選択されていません。")
            return {"CANCELLED"}

        ok, msg = convert_mesh_to_terrain_grid(
            target_obj=obj,
            margin=self.margin,
            divisions_per_unit=self.divisions_per_unit,
            max_divisions=self.max_divisions,
            floor_z_mode=self.floor_z_mode,
            custom_floor_z=self.custom_floor_z,
            texture_path=self.texture_path,
            uv_tile=self.uv_tile,
            replace_mode=self.replace_mode,
            create_preview=self.create_preview,
        )
        if not ok:
            self.report({"ERROR"}, msg)
            return {"CANCELLED"}

        first_line = msg.split("\n")[0]
        self.report({"INFO"}, first_line)
        return {"FINISHED"}


# ==========================================
# レール沿い地形変形 ＆ なぞりスカルプト連携
# ==========================================

def export_terrain_mesh_to_obj(terrain_obj, rel_path="resources/Stagemap/terrain_deformed.obj"):
    """
    変形後の地形メッシュを、ゲーム側で直接読み込めるOBJファイルとして書き出す。
    """
    try:
        from .road_generator import _get_project_root
    except ImportError:
        from road_generator import _get_project_root

    root = _get_project_root()
    full_path = os.path.join(root, rel_path)
    os.makedirs(os.path.dirname(full_path), exist_ok=True)

    mesh = terrain_obj.data
    mat_world = terrain_obj.matrix_world

    with open(full_path, "w", encoding="utf-8") as f:
        f.write("# Exported Deformed Terrain from Level Editor\n")
        f.write(f"o {terrain_obj.name}\n")

        # 頂点 (ローカル座標 Blender X, Y, Z -> Game X, Z, Y)
        for v in mesh.vertices:
            f.write(f"v {v.co.x:.4f} {v.co.z:.4f} {v.co.y:.4f}\n")

        # UV
        uv_layer = mesh.uv_layers.active
        has_uv = (uv_layer is not None)
        if has_uv:
            for poly in mesh.polygons:
                for loop_idx in poly.loop_indices:
                    u, v = uv_layer.data[loop_idx].uv
                    f.write(f"vt {u:.4f} {v:.4f}\n")

        # 面法線 (Blender X, Y, Z -> Game X, Z, Y)
        for poly in mesh.polygons:
            nor = poly.normal
            f.write(f"vn {nor.x:.4f} {nor.z:.4f} {nor.y:.4f}\n")

        # 面（ゲーム側DirectXは時計回りが表面のため、Blenderの反時計回りを反転して出力）
        for face_idx, poly in enumerate(mesh.polygons, start=1):
            f.write("f")
            for loop_idx in reversed(poly.loop_indices):
                v_idx = poly.vertices[loop_idx - poly.loop_start] + 1
                if has_uv:
                    f.write(f" {v_idx}/{loop_idx + 1}/{face_idx}")
                else:
                    f.write(f" {v_idx}//{face_idx}")
            f.write("\n")

    print(f"[TerrainGen] 変形後地形OBJ(ローカル座標)を保存しました: {full_path}")


def deform_terrain_mesh_to_rail(
    terrain_obj,
    rail_obj,
    road_width=4.0,
    slope_width=4.0,
    offset_z=0.0,
    falloff="SMOOTH",
    auto_export_obj=True,
):
    """
    レール直下の地形メッシュの頂点を変形し、
    道（中央の平坦な道路天端 + 左右の法面・土手斜面）を地面に直接造形する。
    """
    if not terrain_obj or terrain_obj.type != 'MESH':
        return False, "地形メッシュオブジェクトが見つかりません。"
    if not rail_obj or rail_obj.type != 'CURVE':
        return False, "レール（CURVE）オブジェクトが見つかりません。"

    try:
        from .road_generator import sample_curve_frenet
    except ImportError:
        from road_generator import sample_curve_frenet

    samples = sample_curve_frenet(rail_obj, step_distance=0.2)  # 0.4m → 0.2m で精度向上
    if not samples:
        return False, "レールのサンプリングに失敗しました。"

    mesh = terrain_obj.data
    mat_world = terrain_obj.matrix_world
    mat_inv = mat_world.inverted()

    # レール点列のXY座標とZ座標（ワールド座標系）
    rail_pts = [(s["pos"].x, s["pos"].y, s["pos"].z) for s in samples]
    num_rail = len(rail_pts)

    half_road_w = road_width * 0.5
    total_radius = half_road_w + max(slope_width, 0.1)
    total_radius_sq = total_radius * total_radius

    modified_count = 0

    for v in mesh.vertices:
        w_pos = mat_world @ v.co
        vx, vy, vz = w_pos.x, w_pos.y, w_pos.z

        # XY平面上で最も近いレール上の点を探索する
        # 線分への垂線の足（最近点投影）を使って精度を向上
        min_dist_sq = 1e9
        target_z = vz

        for k in range(num_rail - 1):
            ax, ay, az = rail_pts[k]
            bx, by, bz = rail_pts[k + 1]
            # 線分 AB の方向ベクトル（XY成分のみ）
            abx = bx - ax
            aby = by - ay
            ab_len_sq = abx * abx + aby * aby
            if ab_len_sq < 1e-8:
                # 縮退した線分：端点で判定
                dx, dy = vx - ax, vy - ay
                d_sq = dx*dx + dy*dy
                if d_sq < min_dist_sq:
                    min_dist_sq = d_sq
                    target_z = az + offset_z
                continue

            # 垂線の足のパラメータ t ∈ [0, 1]
            t = ((vx - ax) * abx + (vy - ay) * aby) / ab_len_sq
            t = max(0.0, min(1.0, t))

            # 垂線の足の XY 座標と Z（線形補間）
            foot_x = ax + t * abx
            foot_y = ay + t * aby
            foot_z = az + t * (bz - az)

            dx = vx - foot_x
            dy = vy - foot_y
            d_sq = dx*dx + dy*dy
            if d_sq < min_dist_sq:
                min_dist_sq = d_sq
                target_z = foot_z + offset_z

        # 最後の点との距離もチェック（ループ終端）
        lx, ly, lz = rail_pts[-1]
        dx, dy = vx - lx, vy - ly
        d_sq = dx*dx + dy*dy
        if d_sq < min_dist_sq:
            min_dist_sq = d_sq
            target_z = lz + offset_z

        if min_dist_sq <= total_radius_sq:
            d = math.sqrt(min_dist_sq)

            if d <= half_road_w:
                # 道路天端（中央の平坦部）：レールの高さにピッタリ一致
                weight = 1.0
            else:
                # 法面（斜面部）：道路端から地面へ向かって滑らかに下りる
                t = (d - half_road_w) / max(slope_width, 0.01)
                t = min(max(t, 0.0), 1.0)
                if falloff == "SMOOTH":
                    weight = 1.0 - (3.0 * t * t - 2.0 * t * t * t)
                elif falloff == "LINEAR":
                    weight = 1.0 - t
                else:  # FLAT
                    weight = 1.0

            # Z座標を変形（盛土または掘割）
            new_w_z = vz * (1.0 - weight) + target_z * weight
            new_w_pos = mathutils.Vector((vx, vy, new_w_z))
            v.co = mat_inv @ new_w_pos
            modified_count += 1

    mesh.update()

    # ゲーム連携用にOBJファイルを出力し、オブジェクトプロパティを設定
    if auto_export_obj:
        rel_obj_path = "resources/Stagemap/terrain_deformed.obj"
        try:
            export_terrain_mesh_to_obj(terrain_obj, rel_obj_path)
            terrain_obj["file_name"] = "terrain_deformed.obj"
            terrain_obj["model_dir"] = "resources/Stagemap"
            terrain_obj["object_type"] = "TERRAIN"
            if "texture" not in terrain_obj:
                terrain_obj["texture"] = "resources/Stagemap/863603.png"
        except Exception as e:
            print(f"[TerrainGen] OBJエクスポート警告: {e}")

    return True, (
        f"地面メッシュ '{terrain_obj.name}' を変形して道をつくりました！\n"
        f"  道路幅: {road_width:.1f}m (平坦部), 法面幅: {slope_width:.1f}m\n"
        f"  変形頂点数: {modified_count} 点\n"
        f"  ゲーム用モデル: resources/Stagemap/terrain_deformed.obj (自動更新)"
    )


def deform_terrain_terraces_to_rail(
    terrain_obj,
    rail_obj,
    camera_side="RIGHT",
    terrace_start_dist=3.2,
    terrace_step_width=3.2,
    terrace_step_height=1.25,
    cliff_width=0.5,
    max_steps=3,
    auto_export_obj=False
):
    """
    星のカービィ64風の『棚田（ステップ・テラス）段差地形』を地面メッシュに直接造形する。
    1枚目溶岩画像のように、通路の奥に向かって等高線状に急な崖（高さ1.25m）と
    平坦なテラス面（幅約3m）がシームレスに多段で連なる美しい棚田ステップを形成する。
    """
    if not terrain_obj or terrain_obj.type != 'MESH':
        return False, "地形メッシュオブジェクトが見つかりません。"
    if not rail_obj or rail_obj.type != 'CURVE':
        return False, "レールオブジェクトが見つかりません。"

    try:
        from .road_generator import sample_curve_frenet
    except ImportError:
        from road_generator import sample_curve_frenet

    samples = sample_curve_frenet(rail_obj, step_distance=0.25)
    if not samples:
        return False, "レールのサンプリングに失敗しました。"

    mesh = terrain_obj.data
    mat_world = terrain_obj.matrix_world
    mat_inv = mat_world.inverted()

    # レールサンプルの座標、接線、累積距離を計算
    rail_pts = []
    rail_tans = []
    rail_dists = []
    acc_dist = 0.0
    for i, s in enumerate(samples):
        p = s["pos"]
        rail_pts.append((p.x, p.y, p.z))
        t = s["tangent"]
        t_xy = mathutils.Vector((t.x, t.y, 0.0))
        if t_xy.length_squared > 1e-6:
            t_xy.normalize()
        else:
            t_xy = mathutils.Vector((0.0, 1.0, 0.0))
        rail_tans.append(t_xy)

        if i > 0:
            prev_p = samples[i - 1]["pos"]
            acc_dist += (p - prev_p).length
        rail_dists.append(acc_dist)

    total_rail_len = max(acc_dist, 1.0)
    num_rail = len(rail_pts)

    # 背景側の符号（カメラがRIGHTなら背景は左奥: +1, LEFTなら右奥: -1）
    bg_sign = 1.0 if camera_side == "RIGHT" else -1.0

    modified_count = 0

    for v in mesh.vertices:
        w_pos = mat_world @ v.co
        vx, vy, vz = w_pos.x, w_pos.y, w_pos.z

        # XY平面上で最近接レール線分を探索
        min_dist_sq = 1e9
        best_foot = None
        best_tan = None
        best_s = 0.0

        for k in range(num_rail - 1):
            ax, ay, az = rail_pts[k]
            bx, by, bz = rail_pts[k + 1]
            abx = bx - ax
            aby = by - ay
            ab_len_sq = abx * abx + aby * aby
            if ab_len_sq < 1e-8:
                continue

            t_param = ((vx - ax) * abx + (vy - ay) * aby) / ab_len_sq
            t_param = max(0.0, min(1.0, t_param))

            foot_x = ax + t_param * abx
            foot_y = ay + t_param * aby
            foot_z = az + t_param * (bz - az)

            dx = vx - foot_x
            dy = vy - foot_y
            d_sq = dx * dx + dy * dy
            if d_sq < min_dist_sq:
                min_dist_sq = d_sq
                best_foot = (foot_x, foot_y, foot_z)
                best_tan = rail_tans[k]
                best_s = rail_dists[k] + t_param * math.sqrt(ab_len_sq)

        if not best_foot:
            continue

        foot_x, foot_y, foot_z = best_foot
        rel_x = vx - foot_x
        rel_y = vy - foot_y

        # 背景法線ベクトル N_bg (進行方向 tan に対して時計回りに90度回転 * bg_sign)
        n_bg_x = -best_tan.y * bg_sign
        n_bg_y = best_tan.x * bg_sign

        # 背景側への垂直距離 d_bg
        d_bg = rel_x * n_bg_x + rel_y * n_bg_y

        # 背景側かつ道（terrace_start_dist）より外側の頂点のみ棚田変形
        if d_bg <= terrace_start_dist:
            continue

        # ── 星のカービィ64風の有機的でシームレスな棚田（テラス）計算 ──
        # 進行方向アーク長 best_s に基づく等高線の自然な湾曲（溶岩画像のような滑らかな円弧）
        s_norm = best_s / total_rail_len
        wave1 = 1.8 * math.sin(best_s * 0.12)
        wave2 = 1.0 * math.cos(best_s * 0.28 + 0.8)
        wave3 = 0.6 * math.sin(best_s * 0.05 + 1.5)
        terrace_offset = wave1 + wave2 + wave3

        # 通路端からの有効奥行き距離
        d_eff = d_bg + terrace_offset - terrace_start_dist
        if d_eff <= 0.0:
            continue

        # 進行度に応じた段差のまばらさ（1段〜3段で自然に変化）
        density_mod = 0.5 * (1.0 + math.sin(s_norm * math.pi * 3.0 + 0.8)) # 0.0〜1.0
        cur_max_steps = max(1, min(max_steps, int(1.0 + density_mod * (max_steps - 0.5))))

        # 1段目は通路端から直ちに崖（立ち上がり）が始まり、平坦テラスへ乗る
        step_idx = int(d_eff / terrace_step_width)
        local_u = d_eff - (step_idx * terrace_step_width)

        if step_idx >= cur_max_steps:
            # 最終段のテラス面
            added_h = cur_max_steps * terrace_step_height
        else:
            base_h = step_idx * terrace_step_height
            next_h = (step_idx + 1) * terrace_step_height
            # 各ステップの先頭 cliff_width 区間で次の段へ一気に立ち上がる（急な崖）
            if local_u < cliff_width:
                t = local_u / max(cliff_width, 0.01)
                t = min(max(t, 0.0), 1.0)
                # スムーズステップで急崖を形成
                w = t * t * (3.0 - 2.0 * t)
                added_h = base_h + (next_h - base_h) * w
            else:
                # 崖を登りきった後は、平坦なテラス面（足場）
                added_h = next_h

        # 地面メッシュの高さに棚田の高さを上乗せ
        target_z = vz + added_h

        v.co = mat_inv @ mathutils.Vector((vx, vy, target_z))
        modified_count += 1

    mesh.update()

    if auto_export_obj:
        try:
            export_terrain_mesh_to_obj(terrain_obj)
        except Exception:
            pass

    return True, f"棚田段差地形を変形造形しました（{modified_count}頂点）"


class MYADDON_OT_deform_terrain_to_rail(bpy.types.Operator):
    bl_idname = "myaddon.deform_terrain_to_rail"
    bl_label = "レールに沿って地面を変形して道をつくる"
    bl_description = "既存の地面メッシュ（TerrainGround 等）の頂点を変形し、レールの幅・高さ・勾配に沿った道（土手・道路形状）を直接造形します"
    bl_options = {"REGISTER", "UNDO"}

    road_width: bpy.props.FloatProperty(
        name="道路の平坦幅 (m)",
        description="レール直下の平らな道として歩ける幅（メートル）",
        default=4.0,
        min=0.5,
        max=50.0,
    )
    slope_width: bpy.props.FloatProperty(
        name="法面・土手の斜面幅 (m)",
        description="道路の端から元の地面へと下りる斜面の幅（メートル）",
        default=4.0,
        min=0.1,
        max=50.0,
    )
    offset_z: bpy.props.FloatProperty(
        name="高さオフセット (m)",
        description="レールの高さから地面の高さをどれだけずらすか（例: 0.0m でレール高さ一致、-0.1m でレールのわずか下）",
        default=0.0,
        min=-10.0,
        max=10.0,
    )
    falloff: bpy.props.EnumProperty(
        name="斜面の形状（減衰）",
        description="土手の法面スロープの傾斜形状",
        items=[
            ("SMOOTH", "スムーズ（滑らかな土手）", "自然な丸みを持った斜面で地面になじませます"),
            ("LINEAR", "リニア（台形・直線斜面）", "直線的な台形断面の盛り土にします"),
        ],
        default="SMOOTH",
    )

    def invoke(self, context, event):
        return context.window_manager.invoke_props_dialog(self, width=360)

    def draw(self, context):
        layout = self.layout
        layout.label(text="=== 地面メッシュ道路変形パラメータ ===", icon="MOD_OCEAN")
        col = layout.column(align=True)
        col.prop(self, "road_width")
        col.prop(self, "slope_width")
        col.prop(self, "offset_z")
        layout.separator()
        layout.prop(self, "falloff")

    def execute(self, context):
        # 1. レール取得
        rail_obj = None
        if context.object and context.object.type == 'CURVE':
            rail_obj = context.object
        elif "StageRail" in bpy.data.objects:
            rail_obj = bpy.data.objects["StageRail"]

        if not rail_obj:
            self.report({'ERROR'}, "レール（Curve）が見つかりません。")
            return {'CANCELLED'}

        # 2. 地面メッシュ取得（読み込みステージの Terrain_PlainsGround / GroundTerrain 等を最優先検出）
        terrain_obj = None
        if context.object and context.object.type == 'MESH':
            if context.object.get("object_type") == "TERRAIN" or "terrain" in context.object.name.lower() or "ground" in context.object.name.lower():
                terrain_obj = context.object

        if not terrain_obj:
            # シーン内から TERRAIN タイプを最優先検索
            for o in bpy.data.objects:
                if o.type == 'MESH' and o.get("object_type") == "TERRAIN":
                    terrain_obj = o
                    break

        if not terrain_obj:
            # 名前に Terrain または Ground を含むメッシュを検索
            for o in bpy.data.objects:
                if o.type == 'MESH' and ("terrain" in o.name.lower() or "ground" in o.name.lower()):
                    terrain_obj = o
                    break

        if not terrain_obj and "TerrainGround" in bpy.data.objects:
            terrain_obj = bpy.data.objects["TerrainGround"]

        if not terrain_obj:
            # 地面メッシュがまだなければ自動で高密度グリッドを生成
            self.report({'INFO'}, "地面メッシュが見つからないため、新規に高密度地面グリッドを自動作成します...")
            ok, msg = auto_create_terrain_ground_for_rail(rail_obj)
            if not ok:
                self.report({'ERROR'}, f"地面の自動生成に失敗しました: {msg}")
                return {'CANCELLED'}
            terrain_obj = bpy.data.objects.get("TerrainGround")

        ok, msg = deform_terrain_mesh_to_rail(
            terrain_obj=terrain_obj,
            rail_obj=rail_obj,
            road_width=self.road_width,
            slope_width=self.slope_width,
            offset_z=self.offset_z,
            falloff=self.falloff,
            auto_export_obj=True,
        )

        if not ok:
            self.report({'ERROR'}, msg)
            return {'CANCELLED'}

        first_line = msg.split("\n")[0]
        self.report({'INFO'}, first_line)
        return {'FINISHED'}


def auto_create_terrain_ground_for_rail(rail_obj):
    """
    レールに合わせて高密度な地面グリッド（TerrainGround）を自動生成する。
    """
    try:
        from .road_generator import sample_curve_frenet
    except ImportError:
        from road_generator import sample_curve_frenet

    samples = sample_curve_frenet(rail_obj, step_distance=1.0)
    if not samples:
        return False, "レールのサンプリングに失敗しました。"

    xs = [s["pos"].x for s in samples]
    ys = [s["pos"].y for s in samples]
    zs = [s["pos"].z for s in samples]

    margin = 15.0
    min_x, max_x = min(xs) - margin, max(xs) + margin
    min_y, max_y = min(ys) - margin, max(ys) + margin
    min_z = min(zs)

    cx = (min_x + max_x) * 0.5
    cy = (min_y + max_y) * 0.5
    sx = max(max_x - min_x, 20.0)
    sy = max(max_y - min_y, 20.0)

    # 1mあたり約1〜2分割（最大96分割）
    div_x = min(max(int(sx * 1.5), 32), 96)
    div_y = min(max(int(sy * 1.5), 32), 96)

    t_mesh = _create_grid_mesh_data(
        "Mesh_TerrainGround",
        size_x=sx, size_y=sy,
        div_x=div_x, div_y=div_y,
        texture_path="resources/Stagemap/863603.png",
        uv_tile=4.0,
    )

    # 既存の地形オブジェクトを検出（上書き対象）
    existing_terrains = _find_existing_terrain_objects("TerrainGround")
    primary_obj = None
    target_col = None

    if existing_terrains:
        mesh_terrains = [o for o in existing_terrains if o.type == 'MESH']
        if mesh_terrains:
            primary_obj = mesh_terrains[0]
        else:
            primary_obj = existing_terrains[0]

        if primary_obj.users_collection:
            target_col = primary_obj.users_collection[0]

        for extra_obj in existing_terrains:
            if extra_obj != primary_obj and extra_obj.name in bpy.data.objects:
                try:
                    bpy.data.objects.remove(extra_obj, do_unlink=True)
                except Exception:
                    pass

    t_obj = None
    is_overwrite = False
    if primary_obj and primary_obj.name in bpy.data.objects:
        if primary_obj.type == 'MESH':
            old_mesh = primary_obj.data
            primary_obj.data = t_mesh
            primary_obj.name = "TerrainGround"
            primary_obj.location = (cx, cy, min_z - 1.0)
            primary_obj.rotation_euler = (0.0, 0.0, 0.0)
            primary_obj.scale = (1.0, 1.0, 1.0)
            t_obj = primary_obj
            is_overwrite = True

            if old_mesh and old_mesh != t_mesh and old_mesh.users == 0:
                try:
                    bpy.data.meshes.remove(old_mesh, do_unlink=True)
                except Exception:
                    pass
        else:
            if primary_obj.users_collection:
                target_col = primary_obj.users_collection[0]
            try:
                bpy.data.objects.remove(primary_obj, do_unlink=True)
            except Exception:
                pass
            is_overwrite = True

    if t_obj is None:
        t_obj = bpy.data.objects.new("TerrainGround", t_mesh)
        t_obj.location = (cx, cy, min_z - 1.0)
        col = target_col or bpy.context.collection
        col.objects.link(t_obj)

    t_obj["object_type"] = "BLOCK"
    t_obj["file_name"] = "terrain_deformed.obj"
    t_obj["model_dir"] = "resources/Stagemap"
    t_obj["texture"] = "resources/Stagemap/863603.png"

    msg = "TerrainGround を上書き更新しました" if is_overwrite else "TerrainGround を作成しました"
    return True, msg


class MYADDON_OT_enter_terrain_sculpt(bpy.types.Operator):
    bl_idname = "myaddon.enter_terrain_sculpt"
    bl_label = "なぞって地形変形（スカルプト開始）"
    bl_description = "地形メッシュを選択し、マウスでなぞって山や谷を直接盛り上げるスカルプトモードに切り替えます"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        terrain_obj = None
        if context.object and context.object.type == 'MESH':
            terrain_obj = context.object
        elif "TerrainGround" in bpy.data.objects:
            terrain_obj = bpy.data.objects["TerrainGround"]
        else:
            for o in bpy.data.objects:
                if o.type == 'MESH' and o.get("object_type") == "TERRAIN":
                    terrain_obj = o
                    break

        if not terrain_obj:
            self.report({'ERROR'}, "スカルプト対象の地形メッシュが見つかりません。")
            return {'CANCELLED'}

        bpy.context.view_layer.objects.active = terrain_obj
        terrain_obj.select_set(True)

        try:
            bpy.ops.object.mode_set(mode='SCULPT')
            # ツールをドロー（盛り上げ）に設定
            bpy.ops.wm.tool_set_by_id(name="builtin_brush.Draw")
            self.report({'INFO'}, "スカルプトモードを開始しました！マウスでなぞって地形を盛り上げ、Ctrl+なぞりで掘り下げられます。Tabキーで戻ります。")
        except Exception as e:
            self.report({'WARNING'}, f"スカルプトモード切替: {e}")

        return {'FINISHED'}


class MYADDON_OT_draw_rail_mode(bpy.types.Operator):
    bl_idname = "myaddon.draw_rail_mode"
    bl_label = "フリーハンドでなぞってレールを描く"
    bl_description = "ペンでなぞって空間や地面にレール（カーブ）を描画するドローツールを起動します"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        rail_obj = None
        if context.object and context.object.type == 'CURVE':
            rail_obj = context.object
        elif "StageRail" in bpy.data.objects:
            rail_obj = bpy.data.objects["StageRail"]
        else:
            # 新規レール作成
            curve_data = bpy.data.curves.new(name="StageRail", type='CURVE')
            curve_data.dimensions = '3D'
            rail_obj = bpy.data.objects.new("StageRail", curve_data)
            context.collection.objects.link(rail_obj)
            rail_obj["object_type"] = "STAGE_RAIL"

        bpy.context.view_layer.objects.active = rail_obj
        rail_obj.select_set(True)

        # エディットモードに移行してドローツール起動
        try:
            bpy.ops.object.mode_set(mode='EDIT')
            try:
                bpy.ops.wm.tool_set_by_id(name="builtin.draw")
            except Exception:
                bpy.ops.wm.tool_set_by_id(name="builtin.curve_pen")
            self.report({'INFO'}, "ドローツールを起動しました！マウスやペンでなぞってレールを描いてください。描き終わったらTabキーでオブジェクトモードに戻り「レールに沿って道をつくる」を押してください。")
        except Exception as e:
            self.report({'WARNING'}, f"ドローツール起動: {e}")

        return {'FINISHED'}


# ------------------------------------------
# 多角柱の窪み彫り込み（押し下げ）機能
# ------------------------------------------

def carve_prism_depression_in_terrain(
    terrain_obj,
    center_pos,
    sides=6,
    radius=3.0,
    depth=1.5,
    bevel_ratio=0.2,
    rotation_rad=0.0,
    auto_subdivide=True,
    auto_export_obj=False
):
    """
    指定された地形メッシュの頂点を、3D空間の中心座標を中心とする正多角柱（sides角形）の形状で
    下方向へ押し下げて窪み（ピット/穴）を彫り込む。
    """
    if not terrain_obj or terrain_obj.type != 'MESH':
        return False, "対象のメッシュオブジェクトが見つかりません。"

    mesh = terrain_obj.data
    mat_world = terrain_obj.matrix_world
    mat_inv = mat_world.inverted()

    # ワールド座標での中心
    cx, cy, cz = center_pos.x, center_pos.y, center_pos.z

    import bmesh
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.verts.ensure_lookup_table()
    bm.faces.ensure_lookup_table()

    # 正多角形の幾何パラメータ
    sides = max(3, min(sides, 16))
    alpha = math.pi / float(sides)
    cos_alpha = math.cos(alpha)
    two_alpha = 2.0 * alpha

    # 自動細分化: 粗いポリゴンの場合、窪み範囲内(半径の1.3倍)の面を細分化して綺麗な輪郭を出す
    if auto_subdivide:
        subdiv_faces = []
        for f in bm.faces:
            fc = mat_world @ f.calc_center_median()
            if (fc.x - cx)**2 + (fc.y - cy)**2 <= (radius * 1.3)**2:
                subdiv_faces.append(f)
        if subdiv_faces:
            edges_to_subdiv = list({e for f in subdiv_faces for e in f.edges})
            if edges_to_subdiv:
                bmesh.ops.subdivide_edges(
                    bm,
                    edges=edges_to_subdiv,
                    cuts=1,
                    use_grid_fill=True
                )
                bm.verts.ensure_lookup_table()
                bm.faces.ensure_lookup_table()

    modified_count = 0
    clamped_bevel = max(0.0, min(bevel_ratio, 1.0))

    for v in bm.verts:
        w_pos = mat_world @ v.co
        dx = w_pos.x - cx
        dy = w_pos.y - cy
        dist = math.sqrt(dx * dx + dy * dy)

        # 粗判定
        if dist > radius * 1.5:
            continue

        phi = math.atan2(dy, dx) - rotation_rad
        # [-pi, pi] の範囲に正規化
        phi = (phi + math.pi) % (2.0 * math.pi) - math.pi
        # セグメント内ローカル角 theta (-alpha <= theta <= alpha)
        theta = (phi % two_alpha) - alpha

        cos_theta = math.cos(theta)
        if abs(cos_theta) < 1e-6:
            continue

        r_poly = radius * (cos_alpha / cos_theta)

        if dist <= r_poly:
            # 多角形内部
            r_core = r_poly * (1.0 - clamped_bevel)
            if dist <= r_core or clamped_bevel <= 1e-4:
                # コア平坦底面
                target_offset_z = -depth
            else:
                # 傾斜部分
                t = (dist - r_core) / max(r_poly - r_core, 1e-4)
                # スムーズなコサインカーブでフチをブレンド
                w = 0.5 * (1.0 + math.cos(math.pi * t))
                target_offset_z = -depth * w

            # ローカル座標に逆変換して適用
            new_w_pos = mathutils.Vector((w_pos.x, w_pos.y, w_pos.z + target_offset_z))
            v.co = mat_inv @ new_w_pos
            modified_count += 1

    if modified_count == 0:
        bm.free()
        return False, "窪み範囲内に変形対象となる頂点がありませんでした。3Dカーソルの位置または半径を確認してください。"

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    if auto_export_obj:
        try:
            _export_terrain_obj(terrain_obj)
        except Exception:
            pass

    return True, f"多角柱の窪みを彫り込みました（{modified_count}個の頂点を押し下げ）"


class MYADDON_OT_carve_prism_depression(bpy.types.Operator):
    bl_idname = "myaddon.carve_prism_depression"
    bl_label = "面を多角柱に押し下げる"
    bl_description = "3Dカーソル位置の地面（地形）メッシュの面を、多角柱（3〜12角形）の形状で下方向に押し下げて窪みを作ります"
    bl_options = {'REGISTER', 'UNDO'}

    sides: bpy.props.IntProperty(
        name="角数 (多角柱)",
        description="窪みの形状（3=三角柱, 4=四角柱, 6=六角柱, 8=八角柱...）",
        default=6,
        min=3,
        max=12
    )

    radius: bpy.props.FloatProperty(
        name="半径 (m)",
        description="多角柱の窪みの半径サイズ",
        default=3.0,
        min=0.5,
        max=50.0,
        unit='LENGTH'
    )

    depth: bpy.props.FloatProperty(
        name="押し下げ深さ (m)",
        description="下方向へ押し下げる深さ",
        default=1.5,
        min=0.1,
        max=30.0,
        unit='LENGTH'
    )

    bevel_ratio: bpy.props.FloatProperty(
        name="側面の傾斜割合",
        description="側面（フチ）の傾斜度合い（0.0で垂直な段差、1.0ですり鉢状）",
        default=0.2,
        min=0.0,
        max=1.0,
        subtype='FACTOR'
    )

    rotation_deg: bpy.props.FloatProperty(
        name="回転角度 (度)",
        description="多角形の向き（Z軸回転角）",
        default=0.0,
        min=-180.0,
        max=180.0
    )

    auto_subdivide: bpy.props.BoolProperty(
        name="周囲の面を自動細分化",
        description="粗いグリッド地面でもきれいな多角形のフチが出るように範囲内の面を細分化します",
        default=True
    )

    def execute(self, context):
        # 対象メッシュの取得（選択中メッシュ、または TerrainGround / TERRAIN）
        target_obj = None
        if context.active_object and context.active_object.type == 'MESH':
            target_obj = context.active_object
        elif "TerrainGround" in bpy.data.objects:
            target_obj = bpy.data.objects["TerrainGround"]
        else:
            for obj in bpy.data.objects:
                if obj.type == 'MESH' and (obj.get("object_type") == "TERRAIN" or "terrain" in obj.name.lower()):
                    target_obj = obj
                    break

        if not target_obj:
            self.report({'ERROR'}, "対象の地面メッシュが見つかりません。地形メッシュを選択するか、TerrainGround を作成してください。")
            return {'CANCELLED'}

        # オブジェクトモードにして変形
        if context.mode != 'OBJECT':
            try:
                bpy.ops.object.mode_set(mode='OBJECT')
            except Exception:
                pass

        center_pos = context.scene.cursor.location
        rot_rad = math.radians(self.rotation_deg)

        success, msg = carve_prism_depression_in_terrain(
            terrain_obj=target_obj,
            center_pos=center_pos,
            sides=self.sides,
            radius=self.radius,
            depth=self.depth,
            bevel_ratio=self.bevel_ratio,
            rotation_rad=rot_rad,
            auto_subdivide=self.auto_subdivide,
            auto_export_obj=False
        )

        if success:
            self.report({'INFO'}, f"✅ {msg}")
            return {'FINISHED'}
        else:
            self.report({'WARNING'}, msg)
            return {'CANCELLED'}


# ------------------------------------------
# 登録・解除（単体実行用）
# ------------------------------------------
classes = (
    MYADDON_OT_generate_terrain,
    MYADDON_OT_convert_obj_to_terrain,
    MYADDON_OT_deform_terrain_to_rail,
    MYADDON_OT_enter_terrain_sculpt,
    MYADDON_OT_draw_rail_mode,
    MYADDON_OT_carve_prism_depression,
)

def register():
    for cls in classes:
        try:
            bpy.utils.register_class(cls)
        except ValueError:
            pass

def unregister():
    for cls in reversed(classes):
        try:
            bpy.utils.unregister_class(cls)
        except Exception:
            pass

if __name__ == "__main__":
    register()



