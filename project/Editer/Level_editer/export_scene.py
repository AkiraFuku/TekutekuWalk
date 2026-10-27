import bpy
import math
import bpy_extras
import json
# ==========================================
# 6. シーン出力（エクスポート機能・詳細ログ追加）
# ==========================================
class MYADDON_OT_export_scene(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    bl_idname = "myaddon.myaddon_ot_export_scene"
    bl_label = "シーン出力"
    bl_description = "シーン情報をExportする"
    filename_ext = ".json"
    

    convert_to_game_coords: bpy.props.BoolProperty(
        name="ゲーム座標系に変換 (Blender -> Game)",
        description="Blender座標系(Z-up)からゲーム座標系(Y-up)に変換して出力します",
        default=True,
    )

    def draw(self, context):
        self.layout.prop(self, "convert_to_game_coords")

    def execute(self, context):
        print("--- シーン情報Export開始 (ゲーム座標系変換: " + ("ON" if self.convert_to_game_coords else "OFF") + ") ---")
        self.export_json()
        print("--- シーン情報Export完了 ---")
        if self.convert_to_game_coords:
            self.report({'INFO'}, "シーン情報をゲーム座標系(Y-up: X->X, Y->Z, Z->Y)に変換してExportしました！")
        else:
            self.report({'INFO'}, "シーン情報をExportしました (Blender座標のまま)")
        return {'FINISHED'}   

    def export(self):
        with open(self.filepath, 'wt', encoding='utf-8') as file:
            self.Write_and_print(file, "SCENE")
            for object in bpy.context.scene.objects:
                if object.parent:
                    continue
                if object.name.endswith("_Preview") or object.name.endswith("_preview"):
                    continue
                self.parse_scene_recursive(file, object, 0)
                if object.parent is not None:
                    self.Write_and_print(file, "parent: " + object.parent.name)
                self.Write_and_print(file, "")

    def export_json(self):
        """"JSON形式でファイルに出力"""

        json_object_root = dict()
        json_object_root["name"] = "scene"
        json_object_root["coordinate_system"] = "GAME" if self.convert_to_game_coords else "BLENDER"
        json_object_root["objects"] = list()
        
        # シーンのオブジェクト走査してパック
        for object in bpy.context.scene.objects:
             if (object.parent):
                continue
             if object.name.endswith("_Preview") or object.name.endswith("_preview"):
                continue
             # デフォルトのCubeなど、object_typeが未設定でname=="Cube"の不要オブジェクトを除外
             if object.name == "Cube" and not object.get("object_type"):
                continue
             self.parse_scene_recursive_json(json_object_root["objects"], object, 0)
        #エンコード
        json_text = json.dumps(json_object_root, ensure_ascii=False, cls=json.JSONEncoder, indent=4)
        print(json_text)

        with open(self.filepath, 'wt', encoding='utf-8') as file:
            file.write(json_text)

    def Write_and_print(self, file, text):
        print(text)
        file.write(text + "\n")

    def parse_scene_recursive_json(self, data_parent, object, level):
        json_object = dict()

        if object.get("object_type") in ["ENEMY", "PLAYER_SPAWN", "GOAL"]:
            json_object["type"] = "EMPTY"
        elif object.get("file_name") == "terrain_grid":
            json_object["type"] = "EMPTY"
        else:
            json_object["type"] = object.type
        json_object["name"] = object.name

        # ワールドトランスフォームを取得（親子のズレを防止）
        trans, rot, scale = object.matrix_world.decompose()
        rot = rot.to_euler()
        deg_x = math.degrees(rot.x)
        deg_y = math.degrees(rot.y)
        deg_z = math.degrees(rot.z)

        transform = dict()
        if self.convert_to_game_coords:
            # Blender (X右, Y奥, Z上) -> ゲーム (X右, Y上, Z奥)
            transform["translation"] = [round(trans.x, 4), round(trans.z, 4), round(trans.y, 4)]
            transform["rotation"]    = [round(-deg_x, 4),  round(deg_z, 4),   round(deg_y, 4)]
            transform["scaling"]     = [round(scale.x, 4), round(scale.z, 4), round(scale.y, 4)]
            print(f"  [Export] {object.name}: Blender({trans.x:.2f}, {trans.y:.2f}, {trans.z:.2f}) -> Game({trans.x:.2f}, {trans.z:.2f}, {trans.y:.2f})")
        else:
            transform["translation"] = [round(trans.x, 4), round(trans.y, 4), round(trans.z, 4)]
            transform["rotation"]    = [round(deg_x, 4),   round(deg_y, 4),   round(deg_z, 4)]
            transform["scaling"]     = [round(scale.x, 4), round(scale.y, 4), round(scale.z, 4)]

        json_object["transform"] = transform

        # 各種プロパティの出力
        if "object_type" in object:
            json_object["object_type"] = object["object_type"]
        if "disabled" in object:
            json_object["disabled"] = bool(object["disabled"])
        if "file_name" in object:
            json_object["file_name"] = object["file_name"]
        if "model_dir" in object:
            json_object["model_dir"] = object["model_dir"]
        if "texture" in object:
            json_object["texture"] = object["texture"]
        if "enemy_type" in object:
            json_object["enemy_type"] = object["enemy_type"]
        if "rail_pos" in object:
            json_object["rail_pos"] = list(object["rail_pos"])
        if "loop" in object:
            json_object["loop"] = bool(object["loop"])
        if "event_name" in object:
            json_object["event_name"] = object["event_name"]
        if "fire_mode" in object:
            json_object["fire_mode"] = object["fire_mode"]

        # properties マップ（汎用カスタムプロパティ）
        # "prop_" プレフィックスのカスタムプロパティを自動収集し、
        # ゲーム側 LevelLoader が参照する "properties" キーに出力する。
        # 例: prop_size_x="160.0" → properties["size_x"] = "160.0"
        _known_keys = {
            "object_type", "disabled", "file_name", "model_dir", "texture",
            "enemy_type", "rail_pos", "loop", "event_name", "fire_mode",
            "collider", "collider_center", "collider_size", "behavior_tree",
            "_RNA_UI",
        }
        # 地形専用プロパティの定義
        _terrain_keys = {
            "size_x", "size_y", "divisions_x", "divisions_y",
            "uv_tile", "cell_types", "grass_texture", "road_texture"
        }
        # オブジェクトが TERRAIN（地形）かどうかを厳格に判定
        is_terrain = (
            object.get("object_type") == "TERRAIN" or
            object.get("file_name") == "terrain_grid" or
            object.name.startswith("Terrain")
        )

        properties_map = {}
        for key in object.keys():
            if key in _known_keys:
                continue
            if key.startswith("_"):
                continue
            if key.startswith("prop_"):
                # "prop_size_x" → "size_x" としてゲームに渡す
                prop_key = key[len("prop_"):]
                # 地形専用キーかつ対象が地形でない場合は混入防止のため除外
                if prop_key in _terrain_keys and not is_terrain:
                    continue
                properties_map[prop_key] = str(object[key])
            elif is_terrain and key in _terrain_keys:
                # 地形サイズ・分割数・属性プロパティを確実に properties に出力
                properties_map[key] = str(object[key])
        # カメラ専用プロパティ（描画距離・動作モード等）
        if object.type == 'CAMERA':
            scene = bpy.context.scene
            properties_map["draw_distance"] = str(getattr(scene, "game_camera_draw_distance", 80.0))
            properties_map["camera_mode"] = str(getattr(scene, "game_camera_mode", "AUTO_OFFSET"))
            properties_map["camera_distance"] = str(getattr(scene, "rail_camera_distance", 8.5))
            properties_map["camera_height"] = str(getattr(scene, "rail_camera_height", 2.8))

        # エネミー専用プロパティ（カメラ直前スポーン・感知距離）
        if object.get("object_type") == "ENEMY":
            properties_map["spawn_mode"] = str(object.get("spawn_mode", "TRIGGER_SPAWN"))
            properties_map["spawn_distance"] = str(object.get("spawn_distance", 25.0))

        if properties_map:
            json_object["properties"] = properties_map

        # ── 地形メッシュの頂点位置データ（高低差・棚田・土手形状）の出力 ──
        if is_terrain and object.type == 'MESH' and object.data and object.data.vertices:
            mesh = object.data
            v_positions = []
            for v in mesh.vertices:
                if self.convert_to_game_coords:
                    # Blender ローカル(X右, Y奥, Z上) -> ゲーム ローカル(X右, Y上, Z奥)
                    v_positions.append([round(v.co.x, 4), round(v.co.z, 4), round(v.co.y, 4)])
                else:
                    v_positions.append([round(v.co.x, 4), round(v.co.y, 4), round(v.co.z, 4)])
            json_object["vertex_positions"] = v_positions

            # 三角形ポリゴンのインデックスを出力
            mesh.calc_loop_triangles()
            indices = []
            for tri in mesh.loop_triangles:
                indices.extend([int(tri.vertices[0]), int(tri.vertices[1]), int(tri.vertices[2])])
            json_object["indices"] = indices

        if "collider" in object:
            collider = dict()
            collider["type"] = object["collider"]
            cc = object.get("collider_center", [0, 0, 0])
            cs = object.get("collider_size", [1, 1, 1])
            cc_list = list(cc) if hasattr(cc, "__iter__") else [0, 0, 0]
            cs_list = list(cs) if hasattr(cs, "__iter__") else [1, 1, 1]
            if self.convert_to_game_coords:
                collider["center"] = [cc_list[0], cc_list[2], cc_list[1]]
                collider["size"]   = [cs_list[0], cs_list[2], cs_list[1]]
            else:
                collider["center"] = cc_list
                collider["size"]   = cs_list
            json_object["collider"] = collider

        # レール (CURVE) の制御点出力
        if object.type == "CURVE" and object.data:
            curve = object.data
            rail_points = []
            is_cyclic = False
            saved_types = object.get("interp_types", [])
            for spline in curve.splines:
                if spline.use_cyclic_u:
                    is_cyclic = True
                if spline.type == 'BEZIER':
                    for i, bp in enumerate(spline.bezier_points):
                        co = bp.co
                        hl = bp.handle_left
                        hr = bp.handle_right
                        if self.convert_to_game_coords:
                            pt_co = [co.x, co.z, co.y]
                            pt_hl = [hl.x, hl.z, hl.y]
                            pt_hr = [hr.x, hr.z, hr.y]
                        else:
                            pt_co = [co.x, co.y, co.z]
                            pt_hl = [hl.x, hl.y, hl.z]
                            pt_hr = [hr.x, hr.y, hr.z]
                        # 補間タイプ判定 (saved_types または ハンドル形状)
                        if i < len(saved_types) and saved_types[i]:
                            interp_type = str(saved_types[i]).upper()
                        elif bp.handle_left_type == 'VECTOR' and bp.handle_right_type == 'VECTOR':
                            interp_type = "LINEAR"
                        else:
                            interp_type = "BEZIER"

                        rail_points.append({
                            "co": pt_co,
                            "handle_left": pt_hl,
                            "handle_right": pt_hr,
                            "type": interp_type,
                        })
            if rail_points:
                json_object["rail_points"] = rail_points
                json_object["loop"] = is_cyclic


        # ビヘイビアツリー出力判定（JSON版）
        if "behavior_tree" in object:
            
            tree_name = object["behavior_tree"]
            if tree_name in bpy.data.node_groups:
                tree = bpy.data.node_groups[tree_name]
                if tree.bl_idname == 'BehaviorTreeType':
                    bt_data = self.export_behavior_tree_json(tree)
                    if bt_data:
                        json_object["behavior_tree"] = bt_data
                else:
                    print(f"⚠ {object.name}: '{tree_name}' は BehaviorTreeType ではありません。")
            else:
                print(f"⚠ {object.name}: 指定されたツリー名 '{tree_name}' が存在しません。")
        # 親オブジェクトに登録する
        data_parent.append(json_object)       
        #子供リスト（プレビュー用メッシュ等は除外）
        valid_children = [c for c in object.children if not (c.name.endswith("_Preview") or c.name.endswith("_preview"))]
        if len(valid_children) > 0:
            json_object["children"] = list()
            for child in valid_children:
                self.parse_scene_recursive_json(json_object["children"], child, level+1)
        
      

    def parse_scene_recursive(self, file, object, level):
        indent = ''
        for i in range(level):
            indent += "\t"

        self.Write_and_print(file, indent + object.type )
        trans, rot, scale = object.matrix_local.decompose()
        rot = rot.to_euler()
        rot.x = math.degrees(rot.x)
        rot.y = math.degrees(rot.y)
        rot.z = math.degrees(rot.z)

        self.Write_and_print(file, indent + "Trans(%f, %f, %f)" % (trans.x, trans.y, trans.z))
        self.Write_and_print(file, indent + "Rot(%f, %f, %f)" % (rot.x, rot.y, rot.z))
        self.Write_and_print(file, indent + "Scale(%f, %f, %f)" % (scale.x, scale.y, scale.z))
        self.Write_and_print(file, "" )

        # カスタムプロパティ出力
        if "file_name" in object:
            self.Write_and_print(file, indent + "Name %s" % object["file_name"])

        if "collider" in object:
            self.Write_and_print(file, indent + "Collider %s" % object["collider"])
            temp_str = indent + "CC(%f,%f,%f)" % (object["collider_center"][0], object["collider_center"][1], object["collider_center"][2])
            self.Write_and_print(file, temp_str)
            temp_str = indent + "CS(%f,%f,%f)" % (object["collider_size"][0], object["collider_size"][1], object["collider_size"][2])
            self.Write_and_print(file, temp_str)

        # ビヘイビアツリー出力判定
        if "behavior_tree" in object:
            tree_name = object["behavior_tree"]
            if tree_name in bpy.data.node_groups:
                tree = bpy.data.node_groups[tree_name]
                if tree.bl_idname == 'BehaviorTreeType':
                    self.Write_and_print(file, indent + "BehaviorTree %s" % tree_name)
                    self.export_behavior_tree(file, tree, indent + "\t")
                else:
                    print(f"⚠ {object.name}: '{tree_name}' は BehaviorTreeType ではありません。")
            else:
                print(f"⚠ {object.name}: 指定されたツリー名 '{tree_name}' が存在しません。")

        self.Write_and_print(file, indent + 'End')
        self.Write_and_print(file, '')

        for child in object.children:
            self.parse_scene_recursive(file, child, level + 1)
    def export_behavior_tree_json(self, tree):
        """ビヘイビアツリーを平坦なリスト構造として出力する"""
        root_node = None
        for node in tree.nodes:
            if node.bl_idname == 'BTNode_Root':
                root_node = node
                break
        
        if not root_node:
            print(f"⚠ ツリー '{tree.name}' 内に BTNode_Root が見つかりませんでした。")
            return None

        node_list = []
        node_id_map = {}  # ノードオブジェクト -> ID の辞書

        # 1. ツリーを巡回してすべてのノードにIDを採番＆リスト化
        self.collect_nodes_to_list(root_node, node_list, node_id_map, parent_id=None)

        return {
            "name": tree.name,
            "node_list": node_list
        }

    def collect_nodes_to_list(self, node, node_list, node_id_map, parent_id=None):
        """ノードを巡回してリストに追加する再帰関数"""
        # すでに登録済みの場合はスキップ（循環参照回避）
        if node in node_id_map:
            return node_id_map[node]

        current_id = len(node_list)
        node_id_map[node] = current_id

        # 基本情報の構築
        node_data = {
            "id": current_id,
            "parent_id": parent_id
        }

        # パラメータやプロパティの取得
        if hasattr(node, "get_export_data"):
            data = node.get_export_data()
            node_data["type"] = data.get("type", node.bl_idname)
            params = {k: v for k, v in data.items() if k not in ("type", "condition", "switch")}
            if params:
                node_data["parameters"] = params
            if "condition" in data:
                node_data["condition"] = data["condition"]
            if "switch" in data:
                node_data["switch"] = data["switch"]
        else:
            node_data["type"] = node.bl_idname

        # リストに仮登録（子のIDは後で埋める）
        node_list.append(node_data)

        # 1. Branchノードの分岐処理
        if node.bl_idname == 'BTNode_Branch':
            branch_ids = {}
            for output in node.outputs:
                label = output.name
                child_ids = []
                for link in output.links:
                    cid = self.collect_nodes_to_list(link.to_node, node_list, node_id_map, parent_id=current_id)
                    child_ids.append(cid)
                if child_ids:
                    branch_ids[label] = child_ids
            node_data["branches"] = branch_ids
            return current_id

        # 2. Switchノードの分岐処理
        if node.bl_idname == 'BTNode_Switch':
            case_ids = {}
            for output in node.outputs:
                label = output.name
                child_ids = []
                for link in output.links:
                    cid = self.collect_nodes_to_list(link.to_node, node_list, node_id_map, parent_id=current_id)
                    child_ids.append(cid)
                if child_ids:
                    case_ids[label] = child_ids
            node_data["cases"] = case_ids
            return current_id

        # 3. 通常ノード（Y座標降順で子を取得）
        children_nodes = []
        for output in node.outputs:
            for link in output.links:
                children_nodes.append(link.to_node)
        children_nodes.sort(key=lambda n: n.location.y, reverse=True)

        child_ids = []
        for child in children_nodes:
            cid = self.collect_nodes_to_list(child, node_list, node_id_map, parent_id=current_id)
            child_ids.append(cid)

        node_data["child_ids"] = child_ids
        return current_id
   

    def export_behavior_tree(self, file, tree, indent):
        root_node = None
        for node in tree.nodes:
            if node.bl_idname == 'BTNode_Root':
                root_node = node
                break
        
        if root_node:
            self.export_bt_node_recursive(file, root_node, indent)
        else:
            print(f"⚠ ツリー '{tree.name}' 内に BTNode_Root が見つかりませんでした。")

    def export_bt_node_recursive(self, file, node, indent):
        import json as _json
        if hasattr(node, "get_export_data"):
            data = node.get_export_data()
            type_name = data.get("type", node.bl_idname)
            params = []
            for k, v in data.items():
                if k not in ("type", "condition"):
                    params.append(f"{k}:{v}")
            param_str = " ".join(params)
            if param_str:
                self.Write_and_print(file, indent + f"Node {type_name} {param_str}")
            else:
                self.Write_and_print(file, indent + f"Node {type_name}")

            if "condition" in data:
                cond = data["condition"]
                if isinstance(cond, dict):
                    self.Write_and_print(file, indent + f"Condition {_json.dumps(cond, ensure_ascii=False)}")
                else:
                    self.Write_and_print(file, indent + f"Condition {cond}")
        else:
            self.Write_and_print(file, indent + f"Node {node.bl_idname}")

        if node.bl_idname == 'BTNode_Branch':
            for output in node.outputs:
                branch_label = output.name
                for link in output.links:
                    self.Write_and_print(file, indent + f"Branch_{branch_label}")
                    self.export_bt_node_recursive(file, link.to_node, indent + "\t")
            return

        if node.bl_idname == 'BTNode_Switch':
            if hasattr(node, "get_export_data"):
                data = node.get_export_data()
                if "switch" in data:
                    sw = data["switch"]
                    import json as _json2
                    self.Write_and_print(file, indent + f"Switch {_json2.dumps(sw, ensure_ascii=False)}")
            for output in node.outputs:
                case_label = output.name
                for link in output.links:
                    self.Write_and_print(file, indent + f"Case_{case_label}")
                    self.export_bt_node_recursive(file, link.to_node, indent + "\t")
            return

        children = []
        for output in node.outputs:
            for link in output.links:
                children.append(link.to_node)

        children.sort(key=lambda n: n.location.y, reverse=True)

        for child in children:
            self.export_bt_node_recursive(file, child, indent + "\t")