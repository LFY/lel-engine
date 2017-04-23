'''
LEL game engine data export from Blender scene
Lingfeng Yang (LFY@ github)

based on:

Dropper
by Joseph Hocking www.newarteest.com
MIT license

written in Blender 2.77
open in Text Editor view, then Run Script

Saves info about objects in the scene:
name, position, rotation, scale, custom properties
'''

import bpy

# favor the accelerated C implementation, fallback to Python implementation
try:
    import xml.etree.cElementTree as et
except ImportError:
    import xml.etree.ElementTree as et
# http://eli.thegreenplace.net/2012/03/15/processing-xml-in-python-with-elementtree

def write_data(context, filepath, format):
    print("writing scene data...")
    
    data = { 'OPT_LEL':build_lel(), }[format]
    
    f = open(filepath, 'w', encoding='utf-8')
    f.write(data)
    f.close()

    return {'FINISHED'}

def do_prop(propname, prop):
    instanceTag = "int"
    if isinstance(prop, float):
        instanceTag = "float"
    if isinstance(prop, str):
        instanceTag = "str"

    if instanceTag == "str":
        res = "prop %s %s \"%s\"" % (instanceTag, propname, prop)
    else:
        res = "prop %s %s %s" % (instanceTag, propname, prop)
    return res

def do_custom_props(obj, objtag, objname):
     # custom properties
     res = ""
     res += "set %s %s prop %s %s \"%s\"\n" % (objtag, objname, "str", "__entity_name__", obj.name)
     if obj.parent:
         res += "set %s %s prop %s %s \"%s\"\n" % (objtag, objname, "str", "__entity_parent__", obj.parent.name)
     if len(obj.keys()) > 1:
         for k in obj.keys():
             if k not in "_RNA_UI":
                 res += "set %s %s %s\n" % (objtag, objname, do_prop(k, obj[k]))
     return res

def do_global_prop(obj):
    res = ""
    for k in obj.keys():
        if k not in "_RNA_UI":
            res += "set %s\n" % do_prop(k, obj[k])
    return res

def build_lel():
    dict = {}
    dict["entities"] = []
  
    entities = {}
    entity_ctr = 0
    res = ""

    wanted_models = {}
    scene = bpy.data.scenes["Scene"];
    scene.frame_set(0);

    for obj in bpy.data.objects:
        if obj.name in ["__global_props__"]:
            res += do_global_prop(obj);
        elif "Camera" in obj.data.name:
            my_name = obj.name
            res += "define camera %s %d \n" % (my_name, entity_ctr)
            camera_fov = bpy.data.cameras[my_name].angle;
            camera_nearclip = bpy.data.cameras[my_name].clip_start;
            camera_farclip = bpy.data.cameras[my_name].clip_end;
            aspect = float(scene.render.resolution_x) / float(scene.render.resolution_y);

            res += "set camera %s proj %f %f %f %f\n" % (my_name, camera_fov * 180 / 3.14159265358979323, aspect, camera_nearclip, camera_farclip)
            res += "set camera %s frame %f %f %f %f %f %f %f %f %f\n" % (my_name, obj.location.x, obj.location.y, obj.location.z, -obj.matrix_world[0][2], -obj.matrix_world[1][2], -obj.matrix_world[2][2], obj.matrix_world[0][1], obj.matrix_world[1][1], obj.matrix_world[2][1])
            # custom properties
            res += do_custom_props(obj, "camera", my_name)
            res += "\n"
            entities[my_name] = entity_ctr
            entity_ctr += 1
        else:
            if obj.data.name not in wanted_models.keys():
                res += "define model %s\n" % (obj.data.name)
                wanted_models[obj.data.name] = True;
            res += "define entity %s %d\n" % (obj.data.name, entity_ctr)
            res += "set entity %d frame %f %f %f %f %f %f %f %f %f\n" % (entity_ctr, obj.location.x, obj.location.y, obj.location.z, -obj.matrix_world[0][2], -obj.matrix_world[1][2], -obj.matrix_world[2][2], obj.matrix_world[0][1], obj.matrix_world[1][1], obj.matrix_world[2][1]);
            res += "set entity %d scale %f %f %f\n" % (entity_ctr, obj.scale.x, obj.scale.y, obj.scale.z);
            res += do_custom_props(obj, "entity", entity_ctr)
            res += "\n"
            entities[obj.name] = entity_ctr
            entity_ctr += 1

    # now do animations
    for i in range(1, 2501):
        scene.frame_set(i)
        for obj in bpy.data.objects:
            if obj.get("animated", 0) == 1:
                res += "set entityanim %d %d %f %f %f %f %f %f %f %f %f %f %f %f\n" % (i, entities[obj.name], obj.location.x, obj.location.y, obj.location.z, -obj.matrix_world[0][2], -obj.matrix_world[1][2], -obj.matrix_world[2][2], obj.matrix_world[0][1], obj.matrix_world[1][1], obj.matrix_world[2][1], obj.scale.x, obj.scale.y, obj.scale.z)
        
    scene.frame_set(0)

    return res

# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

class Dropper(Operator, ExportHelper):
    """Dropper - Saves info about objects in the scene."""
    bl_idname = "dropper.scene_text"
    bl_label = "Export Scene Data"

    # ExportHelper mixin class uses this
    filename_ext = ".esys"

    filter_glob = StringProperty(
            default="*.esys",
            options={'HIDDEN'},
            maxlen=255,  # Max internal buffer length, longer would be clamped.
            )

    # options menu next to the file selector
    data_format = EnumProperty(
            name="Data Format",
            description="Choose the data format",
            items=(
                   ('OPT_LEL', "LEL", "LEL \"Markup\" Language hehehehehehe"),
                   ),
            default='OPT_LEL',
            )

    def execute(self, context):
        return write_data(context, self.filepath, self.data_format)


def menu_func_export(self, context):
    self.layout.operator(Dropper.bl_idname, text="Dropper")


def register():
    bpy.utils.register_class(Dropper)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(Dropper)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()

    # test call
    bpy.ops.dropper.scene_text('INVOKE_DEFAULT')

