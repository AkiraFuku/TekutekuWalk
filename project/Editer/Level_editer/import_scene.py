import bpy
import bpy_extras
import json
import math
import mathutils
import os


# ==========================================
# レールベジェ曲線計算 & スナップユーティリティ
# ==========================================
def evaluate_bezier(p0, p1, p2, p3, t):
    """3次ベジェ曲線の補間点を計算 (mathutils.Vector)"""
    u = 1.0 - t
    uu = u * u
    uuu = uu * u
    tt = t * t
    ttt = tt * t
    return uuu * p0 + 3.0 * uu * t * p1 + 3.0 * u * tt * p2 + ttt * p3


def get_rail_world_position(curve_obj, global_t):
    """
    CURVE オブジェクトのベジェスプラインから、進行度 global_t におけるワールド座標を計算
    """
    if not curve_obj or curve_obj.type != 'CURVE':
        return None
    curve_data = curve_obj.data
    if not curve_data.splines:
        return None
    spline = curve_data.splines[0]
    points = spline.bezier_points
    num_pts = len(points)
    if num_pts == 0:
        return None
    if num_pts == 1:
        return curve_obj.matrix_world @ points[0].co

    is_loop = spline.use_cyclic_u
    max_t = float(num_pts) if is_loop else float(num_pts - 1)

    if is_loop:
        global_t = global_t % max_t
        if global_t < 0.0:
            global_t += max_t
    else:
        if global_t <= 0.0:
            return curve_obj.matrix_world @ points[0].co
        if global_t >= max_t:
            return curve_obj.matrix_world @ points[-1].co

    idx1 = int(global_t)
    if is_loop:
        idx2 = (idx1 + 1) % num_pts
        t = global_t - float(idx1)
    else:
        if idx1 >= num_pts - 1:
            idx1 = num_pts - 2
            idx2 = num_pts - 1
            t = 1.0
        else:
            idx2 = idx1 + 1
            t = global_t - float(idx1)

    p1 = points[idx1]
    p2 = points[idx2]

    mat = curve_obj.matrix_world
    p0_w = mat @ p1.co
    p1_w = mat @ p1.handle_right
    p2_w = mat @ p2.handle_left
    p3_w = mat @ p2.co

    return evaluate_bezier(p0_w, p1_w, p2_w, p3_w, t)


def find_target_rail_object(target_name=None):
    """対象となるレール（CURVE）オブジェクトを検索"""
    if target_name and target_name in bpy.data.objects:
        obj = bpy.data.objects[target_name]
        if obj.type == 'CURVE':
            return obj

    if "StageRail" in bpy.data.objects and bpy.data.objects["StageRail"].type == 'CURVE':
        return bpy.data.objects["StageRail"]

    for obj in bpy.data.objects:
        if obj.type == 'CURVE' and obj.get("object_type") == "STAGE_RAIL":
            return obj

    for obj in bpy.data.objects:
        if obj.type == 'CURVE':
            return obj

    return None


def apply_rail_position_to_object(obj, target_rail=None):
    """
    オブジェクトの rail_pos = [t, y] プロパティに基づいて、
    レール上の計算座標（+高さオフセット）にオブジェクトの位置をスナップ配置
    """
    if "rail_pos" not in obj:
        return False

    rp = obj["rail_pos"]
    try:
        t = float(rp[0])
        height_offset = float(rp[1]) if len(rp) > 1 else 0.0
    except (ValueError, TypeError, IndexError):
        return False

    rail = target_rail or find_target_rail_object(obj.get("rail_target"))
    if not rail:
        return False

    rail_world_pos = get_rail_world_position(rail, t)
    if rail_world_pos is None:
        return False

    # BlenderではゲームのY軸（高さ）はZ軸
    final_world_pos = mathutils.Vector((
        rail_world_pos.x,
        rail_world_pos.y,
        rail_world_pos.z + height_offset
    ))

    if obj.parent:
        obj.location = obj.parent.matrix_world.inverted() @ final_world_pos
    else:
        obj.location = final_world_pos

    print(f"  [RailSnap] {obj.name}: rail_pos=[{t}, {height_offset}] -> Location: ({final_world_pos.x:.2f}, {final_world_pos.y:.2f}, {final_world_pos.z:.2f})")
    return True


def snap_all_objects_to_rail():
    """シーン内の rail_pos を持つ全オブジェクトをレール上に配置"""
    count = 0
    for obj in bpy.data.objects:
        if "rail_pos" in obj:
            if apply_rail_position_to_object(obj):
                count += 1
    return count


# ==========================================
# プレビューモデル（メッシュ）読み込み & キャッシュ
# ==========================================
_preview_mesh_cache = {}

def get_project_root():
    """プロジェクトのルートディレクトリを取得"""
    try:
        cur_file = os.path.abspath(__file__)
        level_editer_dir = os.path.dirname(cur_file)
        editer_dir = os.path.dirname(level_editer_dir)
        cand = os.path.dirname(editer_dir)
        if os.path.exists(os.path.join(cand, "resources")):
            return cand
    except Exception:
        pass

    try:
        blend_path = bpy.data.filepath
        if blend_path:
            cand = os.path.dirname(blend_path)
            for _ in range(5):
                if os.path.exists(os.path.join(cand, "resources")):
                    return cand
                cand = os.path.dirname(cand)
    except Exception:
        pass

    fixed_path = r"c:\Users\k024g\OneDrive\デスクトップ\study\3nenn\AL\Project\project"
    if os.path.exists(fixed_path):
        return fixed_path

    return ""


def create_texture_material(mat_name, texture_path):
    """テクスチャ画像を貼った Principled BSDF マテリアルを作成・取得"""
    if mat_name in bpy.data.materials:
        return bpy.data.materials[mat_name]

    mat = bpy.data.materials.new(name=mat_name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links

    bsdf = nodes.get("Principled BSDF")
    if bsdf and os.path.exists(texture_path):
        try:
            img = bpy.data.images.load(texture_path, check_existing=True)
            tex_node = nodes.new(type="ShaderNodeTexImage")
            tex_node.image = img
            links.new(tex_node.outputs["Color"], bsdf.inputs["Base Color"])
        except Exception as e:
            print(f"Error loading texture {texture_path}: {e}")

    return mat


def load_obj_mesh_converted(filepath, tex_path=None, mesh_name=None, convert_coords=True):
    """
    OBJファイルを読み込み、ゲーム座標系 (X右, Y上, Z奥) から
    Blender座標系 (X右, Y奥, Z上) へ頂点軸変換 & 法線補正したメッシュデータを生成。
    """
    if not os.path.exists(filepath):
        return None

    prev_selected = [o for o in bpy.context.selected_objects]
    prev_active = bpy.context.view_layer.objects.active

    imported_objects = []
    try:
        if hasattr(bpy.ops.wm, "obj_import"):
            # 軸変換なし（生データそのまま）でインポート
            try:
                bpy.ops.wm.obj_import(filepath=filepath, forward_axis='Y', up_axis='Z')
            except Exception:
                bpy.ops.wm.obj_import(filepath=filepath)
        elif hasattr(bpy.ops.import_scene, "obj"):
            try:
                bpy.ops.import_scene.obj(filepath=filepath, axis_forward='Y', axis_up='Z')
            except Exception:
                bpy.ops.import_scene.obj(filepath=filepath)
        imported_objects = [o for o in bpy.context.selected_objects if o not in prev_selected]
    except Exception as e:
        print(f"Error importing OBJ {filepath}: {e}")
        return None

    if not imported_objects:
        return None

    mesh_obj = None
    for io in imported_objects:
        if io.type == 'MESH':
            mesh_obj = io
            break

    if not mesh_obj:
        for io in imported_objects:
            bpy.data.objects.remove(io, do_unlink=True)
        return None

    mesh_data = mesh_obj.data.copy()
    if mesh_name:
        mesh_data.name = mesh_name

    # ゲーム座標系 (X右, Y上, Z奥) -> Blender座標系 (X右, Y奥, Z上) への頂点座標変換
    if convert_coords:
        for v in mesh_data.vertices:
            x, y, z = v.co.x, v.co.y, v.co.z
            v.co.x = x
            v.co.y = z
            v.co.z = y
        # YとZのスワップで面の巻き順が裏返るため法線を反転
        mesh_data.flip_normals()

    mesh_data.update()

    if tex_path and os.path.exists(tex_path):
        mat = create_texture_material(f"Mat_{mesh_data.name}", tex_path)
        if not mesh_data.materials:
            mesh_data.materials.append(mat)
        else:
            mesh_data.materials[0] = mat

    for io in imported_objects:
        bpy.data.objects.remove(io, do_unlink=True)

    for o in prev_selected:
        try:
            o.select_set(True)
        except Exception:
            pass
    if prev_active and prev_active.name in bpy.data.objects:
        bpy.context.view_layer.objects.active = prev_active

    return mesh_data


def get_or_load_preview_mesh(obj_type, enemy_type=None, force_reload=False):
    """ENEMY, PLAYER_SPAWN, GOAL 用のプレビューメッシュを取得（ゲーム座標 -> Blender座標変換済み・キャッシュ付き）"""
    global _preview_mesh_cache

    cache_key = obj_type
    mesh_name = f"Mesh_Preview_{obj_type}"

    if obj_type == "ENEMY":
        ekey = enemy_type if enemy_type else "Normal"
        cache_key = f"ENEMY_{ekey}"
        mesh_name = f"Mesh_Preview_ENEMY_{ekey}"

    if force_reload:
        _preview_mesh_cache.pop(cache_key, None)
        if mesh_name in bpy.data.meshes:
            bpy.data.meshes.remove(bpy.data.meshes[mesh_name], do_unlink=True)

    if not force_reload and cache_key in _preview_mesh_cache:
        mesh = _preview_mesh_cache[cache_key]
        if mesh and mesh.name in bpy.data.meshes:
            return mesh

    if not force_reload and mesh_name in bpy.data.meshes:
        _preview_mesh_cache[cache_key] = bpy.data.meshes[mesh_name]
        return bpy.data.meshes[mesh_name]

    root = get_project_root()

    # 敵タイプに応じたテクスチャ選択
    enemy_tex = "taru.png"
    if obj_type == "ENEMY":
        ekey = (enemy_type or "Normal").lower()
        if "chase" in ekey:
            enemy_tex = "taru3.png"
        elif "bound" in ekey:
            enemy_tex = "taru2.png"
        else:
            enemy_tex = "taru.png"

    model_configs = {
        "ENEMY": {
            "obj_rel": os.path.join("resources", "taru", "taru.obj"),
            "tex_rel": os.path.join("resources", "taru", enemy_tex),
        },
        "PLAYER_SPAWN": {
            "obj_rel": os.path.join("resources", "player", "player.obj"),
            "tex_rel": os.path.join("resources", "player", "player.png"),
        },
        "GOAL": {
            "obj_rel": os.path.join("resources", "Goal", "goal.obj"),
            "tex_rel": os.path.join("resources", "Goal", "goal.png"),
        },
    }

    cfg = model_configs.get(obj_type)
    if not cfg:
        return None

    obj_path = os.path.join(root, cfg["obj_rel"]) if root else ""
    tex_path = os.path.join(root, cfg["tex_rel"]) if root else ""

    if os.path.exists(obj_path):
        mesh_data = load_obj_mesh_converted(obj_path, tex_path, mesh_name=mesh_name, convert_coords=True)
        if mesh_data:
            _preview_mesh_cache[cache_key] = mesh_data
            return mesh_data

    # フォールバック
    mesh_data = bpy.data.meshes.new(mesh_name)
    verts = [(-0.5, -0.5, -0.5), (0.5, -0.5, -0.5), (0.5, 0.5, -0.5), (-0.5, 0.5, -0.5),
             (-0.5, -0.5,  0.5), (0.5, -0.5,  0.5), (0.5, 0.5,  0.5), (-0.5, 0.5,  0.5)]
    faces = [
        (0, 3, 2, 1),  # 底面 (Z-, 法線 -Z)
        (4, 5, 6, 7),  # 上面 (Z+, 法線 +Z)
        (0, 1, 5, 4),  # 前面 (Y-, 法線 -Y)
        (2, 3, 7, 6),  # 後面 (Y+, 法線 +Y)
        (1, 2, 6, 5),  # 右面 (X+, 法線 +X)
        (0, 4, 7, 3),  # 左面 (X-, 法線 -X)
    ]
    mesh_data.from_pydata(verts, [], faces)
    mesh_data.validate(verbose=False)
    mesh_data.update(calc_edges=True)
    _preview_mesh_cache[obj_type] = mesh_data
    return mesh_data


def create_terrain_grid_mesh(mesh_name, size_x, size_y, div_x, div_y, texture_path="", uv_tile=4.0, cell_types_str=""):
    """terrain_grid 用のメッシュ（面・UV・テクスチャ付き）を直接生成。cell_types があれば落とし穴や縁三角を再現"""
    # 既存の同名メッシュは削除してから再生成（重複防止）
    if mesh_name in bpy.data.meshes:
        bpy.data.meshes.remove(bpy.data.meshes[mesh_name], do_unlink=True)

    mesh = bpy.data.meshes.new(mesh_name)
    div_x = max(int(div_x), 1)
    div_y = max(int(div_y), 1)
    step_x = size_x / div_x
    step_y = size_y / div_y
    start_x = -size_x * 0.5
    start_y = -size_y * 0.5

    # 頂点生成（ローカル原点中心: Z=0）
    verts = []
    for iy in range(div_y + 1):
        for ix in range(div_x + 1):
            verts.append((start_x + ix * step_x, start_y + iy * step_y, 0.0))

    # cell_types のパース
    cell_types = []
    if cell_types_str:
        for val_s in cell_types_str.split(","):
            val_s = val_s.strip()
            if val_s:
                try:
                    cell_types.append(int(val_s))
                except ValueError:
                    cell_types.append(0)

    # 面生成
    # 頂点インデックス:
    #   i0=(ix,   iy  )  i1=(ix+1, iy  )
    #   i2=(ix,   iy+1)  i3=(ix+1, iy+1)
    # 反時計回り: i0(左下) -> i1(右下) -> i3(右上) -> i2(左上) で法線が Z+ 上向き
    faces = []
    for iy in range(div_y):
        for ix in range(div_x):
            c_idx = iy * div_x + ix
            ctype = cell_types[c_idx] if c_idx < len(cell_types) else 0

            # 2: 完全な穴 (HOLE) は面を生成しない
            if ctype == 2:
                continue

            i0 = iy * (div_x + 1) + ix
            i1 = i0 + 1
            i2 = i0 + (div_x + 1)
            i3 = i2 + 1

            if ctype == 3:
                # 3: EDGE_TRI0 (tri0: i0 -> i1 -> i2 の片方三角ポリゴン)
                faces.append((i0, i1, i2))
            elif ctype == 4:
                # 4: EDGE_TRI1 (tri1: i1 -> i3 -> i2 の片方三角ポリゴン)
                faces.append((i1, i3, i2))
            else:
                # 0: GROUND または 1: ROAD (四角形面)
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

    # 法線を確実に更新し、上面（Z+ > 0）を表向きにする
    mesh.update(calc_edges=True)
    if mesh.polygons and mesh.polygons[0].normal.z < 0:
        mesh.flip_normals()
        mesh.update()

    # テクスチャマテリアル適用
    if texture_path:
        root = get_project_root()
        tex_full = os.path.join(root, texture_path) if (root and not os.path.isabs(texture_path)) else texture_path
        mat = create_texture_material(f"Mat_{mesh_name}", tex_full)
        if mat:
            mesh.materials.append(mat)

    return mesh


def create_terrain_from_vertex_data(mesh_name, vertex_positions, indices, texture_path="", uv_tile=4.0, convert_coords=True):
    """
    JSON内の vertex_positions（高低差・坂道・土手）と indices から、
    起伏のある地形メッシュを直接かつ完全に復元する。
    """
    if mesh_name in bpy.data.meshes:
        bpy.data.meshes.remove(bpy.data.meshes[mesh_name], do_unlink=True)

    mesh = bpy.data.meshes.new(mesh_name)
    verts = []
    for vp in vertex_positions:
        if convert_coords:
            # ゲーム (X右, Y上, Z奥) -> Blender (X右, Y奥, Z上)
            verts.append((float(vp[0]), float(vp[2]), float(vp[1])))
        else:
            verts.append((float(vp[0]), float(vp[1]), float(vp[2])))

    faces = []
    if indices and len(indices) >= 3:
        for i in range(0, len(indices) - 2, 3):
            # DirectX (時計回り) -> Blender (反時計回り) で法線を上向きにするため 0, 2, 1
            if convert_coords:
                faces.append((int(indices[i]), int(indices[i + 2]), int(indices[i + 1])))
            else:
                faces.append((int(indices[i]), int(indices[i + 1]), int(indices[i + 2])))

    mesh.from_pydata(verts, [], faces)
    mesh.validate(verbose=False)
    mesh.update(calc_edges=True)

    # UV 座標の自動生成（平面投影 X-Y）
    if verts:
        min_x = min(v[0] for v in verts)
        max_x = max(v[0] for v in verts)
        min_y = min(v[1] for v in verts)
        max_y = max(v[1] for v in verts)
        span_x = max(max_x - min_x, 1.0)
        span_y = max(max_y - min_y, 1.0)

        uv_layer = mesh.uv_layers.new(name="UVMap")
        for poly in mesh.polygons:
            for loop_idx in poly.loop_indices:
                vert_idx = mesh.loops[loop_idx].vertex_index
                vx, vy, _ = verts[vert_idx]
                u = ((vx - min_x) / span_x) * uv_tile
                v = ((vy - min_y) / span_y) * uv_tile
                uv_layer.data[loop_idx].uv = (u, v)

    # 法線チェック
    if mesh.polygons and mesh.polygons[0].normal.z < 0:
        mesh.flip_normals()
        mesh.update()

    # テクスチャマテリアル適用
    if texture_path:
        root = get_project_root()
        tex_full = os.path.join(root, texture_path) if (root and not os.path.isabs(texture_path)) else texture_path
        mat = create_texture_material(f"Mat_{mesh_name}", tex_full)
        if mat:
            mesh.materials.append(mat)

    return mesh



# ==========================================
# JSON ステージデータ インポーター
# ==========================================

def _import_object_recursive(objects_json, parent=None, clear_existing=True, convert_coords=True, target_collection=None):
    """JSON オブジェクトリストを再帰的に Blender オブジェクトへ変換"""
    col = target_collection or bpy.context.collection

    for obj_data in objects_json:
        name        = obj_data.get("name", "Unnamed")
        type_str    = obj_data.get("type", "MESH")
        obj_type    = obj_data.get("object_type", "")
        transform   = obj_data.get("transform", {})
        file_name   = obj_data.get("file_name", "")
        model_dir   = obj_data.get("model_dir", "")
        texture     = obj_data.get("texture", "")
        disabled    = obj_data.get("disabled", False)
        rail_pos    = obj_data.get("rail_pos", None)
        enemy_type  = obj_data.get("enemy_type", "")
        loop        = obj_data.get("loop", False)
        rail_points = obj_data.get("rail_points", [])
        collider    = obj_data.get("collider", {})
        event_name  = obj_data.get("event_name", "")
        fire_mode   = obj_data.get("fire_mode", "")
        properties  = obj_data.get("properties", {})
        children    = obj_data.get("children", [])

        # デフォルトの不要オブジェクト（Cube, Light, Camera で object_type なし）を除外
        if not obj_type and name in ["Cube", "Light", "Camera"] and type_str in ["MESH", "LIGHT", "CAMERA"]:
            print(f"[import_scene] デフォルト不要オブジェクトをスキップ: {name} ({type_str})")
            continue

        tr_raw = transform.get("translation", [0, 0, 0])
        sc_raw = transform.get("scaling",     [1, 1, 1])
        ro_raw = transform.get("rotation",    [0, 0, 0])

        if convert_coords:
            # ゲーム (X右, Y上, Z奥) -> Blender (X右, Y奥, Z上)
            loc   = (tr_raw[0], tr_raw[2], tr_raw[1])
            scale = (sc_raw[0], sc_raw[2], sc_raw[1])
            rot_rad = (
                -math.radians(ro_raw[0]),
                 math.radians(ro_raw[2]),
                 math.radians(ro_raw[1]),
            )
        else:
            loc   = (tr_raw[0], tr_raw[1], tr_raw[2])
            scale = (sc_raw[0], sc_raw[1], sc_raw[2])
            rot_rad = (
                math.radians(ro_raw[0]),
                math.radians(ro_raw[1]),
                math.radians(ro_raw[2]),
            )

        # 既存オブジェクトの削除
        if clear_existing and name in bpy.data.objects:
            bpy.data.objects.remove(bpy.data.objects[name], do_unlink=True)

        blender_obj = None

        # 1. CURVE (レール)
        if type_str == "CURVE" and rail_points:
            curve_data = bpy.data.curves.new(name=name, type="CURVE")
            curve_data.dimensions = "3D"
            curve_data.resolution_u = 12
            spline = curve_data.splines.new("BEZIER")
            spline.use_cyclic_u = loop
            if len(rail_points) > 1:
                spline.bezier_points.add(len(rail_points) - 1)
            interp_types = []
            for i, rp in enumerate(rail_points):
                co = rp.get("co", [0, 0, 0])
                hl = rp.get("handle_left",  co)
                hr = rp.get("handle_right", co)
                pt_type = rp.get("type", "BEZIER")
                interp_types.append(pt_type)
                bpt = spline.bezier_points[i]

                # 先に FREE に設定して Blender によるハンドルの自動再計算・破壊を防止
                bpt.handle_left_type  = "FREE"
                bpt.handle_right_type = "FREE"

                if convert_coords:
                    # ゲーム (X右, Y上, Z奥) -> Blender (X右, Y奥, Z上)
                    bpt.co           = (co[0], co[2], co[1])
                    bpt.handle_left  = (hl[0], hl[2], hl[1])
                    bpt.handle_right = (hr[0], hr[2], hr[1])
                else:
                    bpt.co           = (co[0], co[1], co[2])
                    bpt.handle_left  = (hl[0], hl[1], hl[2])
                    bpt.handle_right = (hr[0], hr[1], hr[2])

                # ハンドルタイプの設定（LINEARの場合はVECTOR、それ以外はFREEで精密座標を保持）
                if pt_type == "LINEAR":
                    bpt.handle_left_type  = "VECTOR"
                    bpt.handle_right_type = "VECTOR"
                else:
                    bpt.handle_left_type  = "FREE"
                    bpt.handle_right_type = "FREE"

            blender_obj = bpy.data.objects.new(name, curve_data)
            blender_obj["interp_types"] = interp_types
            col.objects.link(blender_obj)

        # 2. 地形（TERRAIN / terrain_grid / 高低差 vertex_positions 付きメッシュ）
        elif (obj_type == "TERRAIN" or file_name == "terrain_grid" or name.startswith("Terrain")) and "vertex_positions" in obj_data and obj_data["vertex_positions"]:
            # 高低差・坂道・土手形状の頂点データから完全復元！
            v_pos = obj_data["vertex_positions"]
            idx = obj_data.get("indices", [])
            uv = float(properties.get("uv_tile", 4.0))
            tex = texture or properties.get("grass_texture", "resources/Stagemap/863603.png")

            t_mesh = create_terrain_from_vertex_data(f"Mesh_{name}", v_pos, idx, texture_path=tex, uv_tile=uv, convert_coords=convert_coords)
            blender_obj = bpy.data.objects.new(name, t_mesh)
            blender_obj.show_wire = False
            col.objects.link(blender_obj)
            print(f"[import_scene] 起伏地形メッシュを頂点復元: {name} (頂点数={len(v_pos)}, 面数={len(idx)//3})")

        # 3. 平坦地形グリッド (terrain_grid フォールバック)
        elif file_name == "terrain_grid" or (obj_type == "TERRAIN" and "size_x" in properties):
            sx = float(properties.get("size_x", 150.0))
            sy = float(properties.get("size_y", 150.0))
            dx = int(float(properties.get("divisions_x", 20)))
            dy = int(float(properties.get("divisions_y", 20)))
            uv = float(properties.get("uv_tile", 4.0))
            cell_types = properties.get("cell_types", "")

            t_mesh = create_terrain_grid_mesh(f"Mesh_{name}", sx, sy, dx, dy, texture, uv, cell_types_str=cell_types)
            blender_obj = bpy.data.objects.new(name, t_mesh)
            blender_obj.show_wire = True
            col.objects.link(blender_obj)
            print(f"[import_scene] 地形グリッドメッシュを生成: {name} ({sx}x{sy}m, div={dx}x{dy}, tex={texture})")

        elif type_str == "EMPTY":
            # ENEMY, PLAYER_SPAWN, GOAL は 3D座標(Empty)ではなくモデルメッシュで描画
            preview_mesh = None
            if obj_type in ["ENEMY", "PLAYER_SPAWN", "GOAL"]:
                preview_mesh = get_or_load_preview_mesh(obj_type, enemy_type=enemy_type)

            if preview_mesh:
                blender_obj = bpy.data.objects.new(name, preview_mesh)
            else:
                blender_obj = bpy.data.objects.new(name, None)
                blender_obj.empty_display_type = 'PLAIN_AXES'
            col.objects.link(blender_obj)

        else:
            # MESH オブジェクト (file_name がある場合はモデルを読み込む)
            loaded_mesh = None
            if file_name and file_name != "terrain_grid":
                root = get_project_root()
                possible_paths = []
                if model_dir:
                    possible_paths.append(os.path.join(root, model_dir, file_name))
                    possible_paths.append(os.path.join(root, model_dir, file_name + ".obj"))
                possible_paths.append(os.path.join(root, "resources", "Stagemap", file_name))
                possible_paths.append(os.path.join(root, "resources", "Stagemap", file_name + ".obj"))
                possible_paths.append(os.path.join(root, "resources", file_name))
                possible_paths.append(os.path.join(root, "resources", file_name + ".obj"))
                possible_paths.append(os.path.join(root, file_name))
                possible_paths.append(os.path.join(root, file_name + ".obj"))

                for p in possible_paths:
                    if os.path.exists(p) and p.lower().endswith(".obj"):
                        tex_full = os.path.join(root, texture) if (texture and root) else ""
                        loaded_mesh = load_obj_mesh_converted(p, tex_full, mesh_name=f"Mesh_{name}", convert_coords=convert_coords)
                        if loaded_mesh:
                            break

            if loaded_mesh:
                blender_obj = bpy.data.objects.new(name, loaded_mesh)
            elif "size_x" in properties:
                # OBJファイルが見つからないが properties にサイズ情報がある場合、地形グリッドを生成
                sx = float(properties.get("size_x", 100.0))
                sy = float(properties.get("size_y", 100.0))
                dx = int(float(properties.get("divisions_x", 20)))
                dy = int(float(properties.get("divisions_y", 20)))
                uv = float(properties.get("uv_tile", 4.0))
                cell_types = properties.get("cell_types", "")
                t_mesh = create_terrain_grid_mesh(f"Mesh_{name}", sx, sy, dx, dy, texture, uv, cell_types_str=cell_types)
                blender_obj = bpy.data.objects.new(name, t_mesh)
                blender_obj.show_wire = True
            else:
                mesh_data = bpy.data.meshes.new(name)
                # 立方体メッシュ生成 (サイズ 1x1x1)
                verts = [
                    (-0.5, -0.5, -0.5), (0.5, -0.5, -0.5), (0.5, 0.5, -0.5), (-0.5, 0.5, -0.5),
                    (-0.5, -0.5,  0.5), (0.5, -0.5,  0.5), (0.5, 0.5,  0.5), (-0.5, 0.5,  0.5)
                ]
                faces = [
                    (0, 3, 2, 1),  # 底面 (Z-, 法線 -Z)
                    (4, 5, 6, 7),  # 上面 (Z+, 法線 +Z)
                    (0, 1, 5, 4),  # 前面 (Y-, 法線 -Y)
                    (2, 3, 7, 6),  # 後面 (Y+, 法線 +Y)
                    (1, 2, 6, 5),  # 右面 (X+, 法線 +X)
                    (0, 4, 7, 3),  # 左面 (X-, 法線 -X)
                ]
                mesh_data.from_pydata(verts, [], faces)
                mesh_data.validate(verbose=False)
                mesh_data.update(calc_edges=True)

                # プレハブ・テクスチャ解決
                eff_tex = texture
                if not eff_tex:
                    if "OneWay" in name or "oneway" in name:
                        eff_tex = "resources/grass.png"
                    elif "Stair" in name or "stair" in name:
                        eff_tex = "resources/grass.png"
                    elif obj_type == "BLOCK":
                        eff_tex = "resources/grass.png"

                if eff_tex:
                    root = get_project_root()
                    tex_full = os.path.join(root, eff_tex) if (eff_tex and root and not os.path.isabs(eff_tex)) else eff_tex
                    if os.path.exists(tex_full):
                        mat = create_texture_material(f"Mat_{name}", tex_full)
                        if mat:
                            mesh_data.materials.append(mat)

                blender_obj = bpy.data.objects.new(name, mesh_data)
            col.objects.link(blender_obj)

        if blender_obj:
            blender_obj.location       = loc
            blender_obj.rotation_euler = rot_rad
            blender_obj.scale          = scale

            # カスタムプロパティ復元
            if obj_type:   blender_obj["object_type"] = obj_type
            if file_name:  blender_obj["file_name"]   = file_name
            if model_dir:  blender_obj["model_dir"]   = model_dir
            if texture:    blender_obj["texture"]     = texture
            if disabled:   blender_obj["disabled"]    = disabled
            if enemy_type: blender_obj["enemy_type"]  = enemy_type
            if loop:       blender_obj["loop"]        = loop
            if event_name: blender_obj["event_name"]  = event_name
            if fire_mode:  blender_obj["fire_mode"]   = fire_mode
            if rail_pos is not None: blender_obj["rail_pos"] = rail_pos

            # コライダー復元
            if collider:
                blender_obj["collider"] = collider.get("type", "BOX")
                cc = collider.get("center", [0, 0, 0])
                cs = collider.get("size", [1, 1, 1])
                if convert_coords:
                    blender_obj["collider_center"] = [cc[0], cc[2], cc[1]]
                    blender_obj["collider_size"]   = [cs[0], cs[2], cs[1]]
                else:
                    blender_obj["collider_center"] = cc
                    blender_obj["collider_size"]   = cs

            for k, v in properties.items():
                # terrain_grid の properties（size_x 等）は prop_ プレフィックス付きで
                # 保存する（エクスポート時に export_scene.py が prop_* を収集するため）。
                # それ以外の汎用プロパティはキー名をそのまま使う。
                if file_name == "terrain_grid" and obj_type == "TERRAIN":
                    blender_obj["prop_" + k] = str(v)
                else:
                    blender_obj[k] = v

            if parent:
                blender_obj.parent = parent

            if children:
                _import_object_recursive(children, parent=blender_obj,
                                         clear_existing=clear_existing,
                                         convert_coords=convert_coords,
                                         target_collection=col)


class MYADDON_OT_import_scene(bpy.types.Operator, bpy_extras.io_utils.ImportHelper):
    bl_idname    = "myaddon.import_scene"
    bl_label     = "シーンロード"
    bl_description = "ステージ JSON ファイルを読み込んでシーンに配置します"

    filename_ext = ".json"
    filter_glob: bpy.props.StringProperty(default="*.json", options={"HIDDEN"})

    clear_scene: bpy.props.BoolProperty(
        name="シーンをクリーンにして上書き",
        description="対象コレクションおよびシーン内の既存ステージオブジェクトを一掃して読み込みます",
        default=True,
    )

    convert_from_game_coords: bpy.props.BoolProperty(
        name="ゲーム座標系から変換 (Game -> Blender)",
        description="ゲーム座標系(X右, Y上, Z奥)からBlender座標系(X右, Y奥, Z上)に変換して配置します",
        default=True,
    )

    snap_to_rails: bpy.props.BoolProperty(
        name="rail_pos をレール上に自動配置",
        description="rail_pos を持つオブジェクト（Player, Enemy, Goal等）をベジェレール上の位置へ自動配置します",
        default=False,
    )

    def execute(self, context):
        try:
            with open(self.filepath, "rt", encoding="utf-8") as f:
                data = json.load(f)
        except Exception as e:
            self.report({"ERROR"}, f"JSON 読み込み失敗: {e}")
            return {"CANCELLED"}

        objects = data.get("objects", [])
        if not objects:
            self.report({"WARNING"}, "オブジェクトが見つかりませんでした")
            return {"FINISHED"}

        # プレビューメッシュのキャッシュをクリアして再変換を保証
        global _preview_mesh_cache
        _preview_mesh_cache.clear()
        for ot in ["ENEMY", "PLAYER_SPAWN", "GOAL"]:
            m_name = f"Mesh_Preview_{ot}"
            if m_name in bpy.data.meshes:
                bpy.data.meshes.remove(bpy.data.meshes[m_name], do_unlink=True)

        # JSON ファイル名（拡張子なし）からシーンごとのコレクション名を取得（例: "stage3"）
        scene_col_name = os.path.splitext(os.path.basename(self.filepath))[0]
        if not scene_col_name:
            scene_col_name = data.get("name", "Scene")

        # コレクションを取得または新規作成してシーンにリンク
        if scene_col_name in bpy.data.collections:
            target_col = bpy.data.collections[scene_col_name]
        else:
            target_col = bpy.data.collections.new(scene_col_name)
            # 現在のシーンのマスターコレクション配下に接続
            if target_col.name not in context.scene.collection.children:
                context.scene.collection.children.link(target_col)

        # クリーンロード処理（対象コレクション内を一掃）
        if self.clear_scene:
            if target_col and target_col.objects:
                for obj in list(target_col.objects):
                    bpy.data.objects.remove(obj, do_unlink=True)
            # さらに「AI_Generated_Stage」などの自動生成コレクションもクリーンアップ
            if "AI_Generated_Stage" in bpy.data.collections:
                gen_col = bpy.data.collections["AI_Generated_Stage"]
                for obj in list(gen_col.objects):
                    bpy.data.objects.remove(obj, do_unlink=True)
            # デフォルトの孤立Cube, Light, Cameraも除去
            for obj in list(context.scene.objects):
                if obj.name in ["Cube", "Light", "Camera"] and not obj.get("object_type"):
                    bpy.data.objects.remove(obj, do_unlink=True)

        # 座標系判定（JSON内に明記があればそれを優先）
        coord_sys = data.get("coordinate_system", "")
        if coord_sys == "BLENDER":
            should_convert = False
        elif coord_sys == "GAME":
            should_convert = True
        else:
            should_convert = self.convert_from_game_coords

        _import_object_recursive(objects, parent=None, clear_existing=self.clear_scene,
                                 convert_coords=should_convert, target_collection=target_col)

        # rail_pos を持つオブジェクトをベジェレール上に自動スナップ（必要な場合のみ）
        snapped_count = 0
        if self.snap_to_rails:
            snapped_count = snap_all_objects_to_rail()

        # 未使用となった孤立データブロックのクリーンアップ
        for m in list(bpy.data.meshes):
            if m.users == 0:
                bpy.data.meshes.remove(m, do_unlink=True)
        for c in list(bpy.data.curves):
            if c.users == 0:
                bpy.data.curves.remove(c, do_unlink=True)

        msg = f"コレクション '{scene_col_name}' に {len(objects)} 個のオブジェクトを配置しました"
        if snapped_count > 0:
            msg += f" (レール自動配置: {snapped_count}個)"
        msg += f" (ゲーム座標変換: {'ON' if should_convert else 'OFF'})"
        self.report({"INFO"}, msg)
        return {"FINISHED"}

    def draw(self, context):
        self.layout.prop(self, "clear_scene")
        self.layout.prop(self, "convert_from_game_coords")
        self.layout.prop(self, "snap_to_rails")
