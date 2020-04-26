/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * Copyright (C) 2020 Gustavo Sousa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"

#include "pixmaps/specialization.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 2.0
#define FONT_HEIGHT 0.8
#define TEXT_MARGIN 1.0

#define NUM_CONNECTIONS 9

typedef struct _Specialization Specialization;

typedef enum {
  SPEC_DISJOINT,
  SPEC_OVERLAPPING,
  SPEC_CATEGORY
} SpecializationType;

struct _Specialization {
  Element element;

  SpecializationType type;

  DiaFont *font;
  real font_height;

  real line_width;
  Color line_color;

  ConnectionPoint connections[NUM_CONNECTIONS];
};

static real specialization_distance_from(Specialization *specialization, Point *point);
static void specialization_select(Specialization *specialization, Point *clicked_point,
                             DiaRenderer *interactive_renderer);
static ObjectChange* specialization_move_handle(Specialization *specialization, Handle *handle,
                                           Point *to, ConnectionPoint *cp,
                                           HandleMoveReason reason,
                                           ModifierKeys modifiers);
static ObjectChange* specialization_move(Specialization *specialization, Point *to);
static void specialization_draw(Specialization *specialization, DiaRenderer *renderer);
static void specialization_update_data(Specialization *specialization);
static DiaObject *specialization_create(Point *startpoint,
                                   void *user_data,
                                   Handle **handle1,
                                   Handle **handle2);
static void specialization_destroy(Specialization *specialization);
static DiaObject *specialization_copy(Specialization *specialization);
static PropDescription *
specialization_describe_props(Specialization *specialization);
static void
specialization_get_props(Specialization *specialization, GPtrArray *props);
static void
specialization_set_props(Specialization *specialization, GPtrArray *props);

static void specialization_save(Specialization *specialization, ObjectNode obj_node,
                           DiaContext *ctx);
static DiaObject *specialization_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps specialization_type_ops =
    {
        (CreateFunc) specialization_create,
        (LoadFunc)   specialization_load,
        (SaveFunc)   specialization_save
    };

static ObjectOps specialization_ops = {
    (DestroyFunc)         specialization_destroy,
    (DrawFunc)            specialization_draw,
    (DistanceFunc)        specialization_distance_from,
    (SelectFunc)          specialization_select,
    (CopyFunc)            specialization_copy,
    (MoveFunc)            specialization_move,
    (MoveHandleFunc)      specialization_move_handle,
    (GetPropertiesFunc)   object_create_props_dialog,
    (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
    (ObjectMenuFunc)      NULL,
    (DescribePropsFunc)   specialization_describe_props,
    (GetPropsFunc)        specialization_get_props,
    (SetPropsFunc)        specialization_set_props,
    (TextEditFunc) 0,
    (ApplyPropertiesListFunc) object_apply_props,
};

static PropEnumData prop_specialization_direction_data[] = {
    { N_("Disjoint"), SPEC_DISJOINT },
    { N_("Overlapping"), SPEC_OVERLAPPING },
    { N_("Category"), SPEC_CATEGORY },
    { NULL, 0 }
};

static PropDescription specialization_props[] = {
    ELEMENT_COMMON_PROPERTIES,
    {"type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
     N_("Type:"), NULL, prop_specialization_direction_data},
    PROP_STD_TEXT_FONT,
    PROP_STD_TEXT_HEIGHT,
    PROP_DESC_END
};

static PropOffset specialization_offsets[] = {
    ELEMENT_COMMON_PROPERTIES_OFFSETS,
    { "type", PROP_TYPE_ENUM, offsetof(Specialization, type) },
    { "text_font", PROP_TYPE_FONT, offsetof(Specialization, font) },
    { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Specialization, font_height) },
    { NULL, 0, 0}
};

DiaObjectType specialization_type = {
    "ER - Specialization/Generalization",    /* name */
    0,                                      /* version */
    specialization_xpm,                     /* pixmap */
    &specialization_type_ops,               /* ops */
    NULL,
    0,
    specialization_props,                   /* props */
    specialization_offsets                  /* offsets */
};


static PropDescription *
specialization_describe_props(Specialization *specialization)
{
  if (specialization_props[0].quark == 0)
    prop_desc_list_calculate_quarks(specialization_props);
  return specialization_props;
}

static void
specialization_get_props(Specialization *specialization, GPtrArray *props)
{
  object_get_props_from_offsets(&specialization->element.object,
                                specialization_offsets, props);
}

static void
specialization_set_props(Specialization *specialization, GPtrArray *props)
{
  object_set_props_from_offsets(&specialization->element.object,
                                specialization_offsets, props);
  specialization_update_data(specialization);
}

static const char *
specialization_get_label(Specialization *specialization)
{
  switch (specialization->type) {
  case SPEC_OVERLAPPING:
    return "o";
  case SPEC_CATEGORY:
    return "U";
  case SPEC_DISJOINT:
  default:
    return "d";
  }
}

static real
specialization_distance_from(Specialization *specialization, Point *point)
{
  Element *elem = &specialization->element;
  Point center;

  center.x = elem->corner.x + elem->width / 2;
  center.y = elem->corner.y + elem->height / 2;

  return distance_ellipse_point(&center, elem->width, elem->height,
                                specialization->line_width, point);
}

static void
specialization_select(Specialization *specialization, Point *clicked_point,
                 DiaRenderer *interactive_renderer)
{
  element_update_handles(&specialization->element);
}

static ObjectChange*
specialization_move_handle(Specialization *specialization, Handle *handle,
                      Point *to, ConnectionPoint *cp,
                      HandleMoveReason reason, ModifierKeys modifiers)
{
  element_move_handle(&specialization->element, handle->id, to, cp, reason, modifiers);
  specialization_update_data(specialization);

  return NULL;
}

static ObjectChange*
specialization_move(Specialization *specialization, Point *to)
{
  specialization->element.corner = *to;
  specialization_update_data(specialization);

  return NULL;
}

static void
specialization_draw (Specialization *specialization, DiaRenderer *renderer)
{
  Point center;
  Point p;
  Element *elem;
  const char *label;

  elem = &specialization->element;

  center.x = elem->corner.x + elem->width / 2;
  center.y = elem->corner.y + elem->height / 2;

  dia_renderer_set_fillstyle (renderer, FILLSTYLE_SOLID);
  dia_renderer_set_linewidth(renderer, specialization->line_width);
  dia_renderer_set_linejoin(renderer, LINEJOIN_MITER);

  dia_renderer_draw_ellipse (renderer,
                             &center,
                             elem->width, elem->height,
                             NULL, &specialization->line_color);

  dia_renderer_set_font(renderer, specialization->font, specialization->font_height);
  label = specialization_get_label(specialization);

  p.x = elem->corner.x + elem->width / 2.0;
  p.y = elem->corner.y + (elem->height - specialization->font_height) / 2.0 +
      dia_font_ascent (label, specialization->font, specialization->font_height);

  dia_renderer_set_font (renderer, specialization->font, specialization->font_height);
  dia_renderer_draw_string (renderer,
                            label,
                            &p,
                            ALIGN_CENTER,
                            &color_black);

}
static void
specialization_update_connections(Specialization *specialization)
{
  Element *elem = &specialization->element;
  Point center;
  static const double COS_45 = cos(45 * M_PI / 180.0);

  center.x = elem->corner.x + elem->width / 2;
  center.y = elem->corner.y + elem->height / 2;

  connpoint_update(&specialization->connections[0],
                   elem->corner.x,
                   elem->corner.y + elem->height / 2.0,
                   DIR_WEST);

  connpoint_update(&specialization->connections[1],
                   center.x - COS_45 * (elem->width / 2),
                   center.y - COS_45 * (elem->height / 2),
                   DIR_NORTHWEST);

  connpoint_update(&specialization->connections[2],
                   elem->corner.x + elem->width / 2.0,
                   elem->corner.y,
                   DIR_NORTH);

  connpoint_update(&specialization->connections[3],
                   center.x + COS_45 * (elem->width / 2),
                   center.y - COS_45 * (elem->height / 2),
                   DIR_NORTHEAST);

  connpoint_update(&specialization->connections[4],
                   elem->corner.x + elem->width,
                   elem->corner.y + elem->height / 2.0,
                   DIR_EAST);

  connpoint_update(&specialization->connections[5],
                   center.x + COS_45 * (elem->width / 2),
                   center.y + COS_45 * (elem->height / 2),
                   DIR_SOUTHEAST);

  connpoint_update(&specialization->connections[6],
                   elem->corner.x + elem->width / 2.0,
                   elem->corner.y + elem->height,
                   DIR_SOUTH);

  connpoint_update(&specialization->connections[7],
                   center.x - COS_45 * (elem->width / 2),
                   center.y + COS_45 * (elem->height / 2),
                   DIR_SOUTHWEST);

  connpoint_update(&specialization->connections[8],
                   elem->corner.x + elem->width / 2.0,
                   elem->corner.y + elem->height / 2.0,
                   DIR_ALL);

}

static void
specialization_update_data(Specialization *specialization)
{
  Element *elem = &specialization->element;
  DiaObject *obj = &elem->object;
  ElementBBExtras *extra = &elem->extra_spacing;
  const char *label = specialization_get_label(specialization);

  real label_width = dia_font_string_width(label, specialization->font, specialization->font_height);

  elem->width = label_width + TEXT_MARGIN;
  elem->height = elem->width;

  specialization_update_connections(specialization);

  extra->border_trans = specialization->line_width / 2.0;
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);

}

static DiaObject *
specialization_create(Point *startpoint,
                 void *user_data,
                 Handle **handle1,
                 Handle **handle2)
{
  Specialization *specialization;
  Element *elem;
  DiaObject *obj;
  int i;

  specialization = g_malloc0(sizeof(Specialization));
  elem = &specialization->element;
  obj = &elem->object;

  obj->type = &specialization_type;
  obj->ops = &specialization_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  specialization->line_width = attributes_get_default_linewidth();
  specialization->line_color = attributes_get_foreground();

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i = 0; i < NUM_CONNECTIONS; i++) {
    obj->connections[i] = &specialization->connections[i];
    specialization->connections[i].object = obj;
    specialization->connections[i].connected = NULL;
  }
  specialization->connections[8].flags = CP_FLAGS_MAIN;

  specialization->type = SPEC_DISJOINT;
  specialization->font = dia_font_new_from_style(DIA_FONT_MONOSPACE, FONT_HEIGHT);
  specialization->font_height = FONT_HEIGHT;
  specialization_update_data(specialization);

  for (i = 0; i < 8; i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return &specialization->element.object;
}

static void
specialization_destroy(Specialization *specialization)
{
  g_object_unref(specialization->font);
  element_destroy(&specialization->element);
}

static DiaObject *
specialization_copy(Specialization *specialization)
{
  int i;
  Specialization *newspec;
  Element *elem, *newelem;
  DiaObject *newobj;

  elem = &specialization->element;

  newspec = g_malloc0(sizeof(Specialization));
  newelem = &newspec->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newspec->type = specialization->type;
  newspec->font = g_object_ref (specialization->font);
  newspec->font_height = specialization->font_height;
  newspec->line_width = specialization->line_width;
  newspec->line_color = specialization->line_color;

  for (i=0;i<NUM_CONNECTIONS;i++) {
    newobj->connections[i] = &newspec->connections[i];
    newspec->connections[i].object = newobj;
    newspec->connections[i].connected = NULL;
    newspec->connections[i].pos = specialization->connections[i].pos;
    newspec->connections[i].flags = specialization->connections[i].flags;
  }

  return &newspec->element.object;
}


static void
specialization_save(Specialization *specialization, ObjectNode obj_node,
               DiaContext *ctx)
{
  element_save(&specialization->element, obj_node, ctx);

  data_add_enum(new_attribute(obj_node, "type"),
                   specialization->type, ctx);
  data_add_font (new_attribute (obj_node, "font"),
                 specialization->font, ctx);
  data_add_real(new_attribute(obj_node, "font_height"),
                specialization->font_height, ctx);
}

static DiaObject *
specialization_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Specialization *specialization;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  specialization = g_malloc0(sizeof(Specialization));
  elem = &specialization->element;
  obj = &elem->object;

  obj->type = &specialization_type;
  obj->ops = &specialization_ops;

  element_load(elem, obj_node, ctx);

  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
    specialization->type = data_enum(attribute_first_data(attr), ctx);

  specialization->font = NULL;
  attr = object_find_attribute(obj_node, "font");
  if (attr != NULL)
    specialization->font = data_font(attribute_first_data(attr), ctx);

  specialization->font_height = FONT_HEIGHT;
  attr = object_find_attribute(obj_node, "font_height");
  if (attr != NULL)
    specialization->font_height = data_real(attribute_first_data(attr), ctx);

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i = 0; i < NUM_CONNECTIONS; i++) {
    obj->connections[i] = &specialization->connections[i];
    specialization->connections[i].object = obj;
    specialization->connections[i].connected = NULL;
  }
  specialization->connections[8].flags = CP_FLAGS_MAIN;

  if (specialization->font == NULL)
    specialization->font = dia_font_new_from_style(DIA_FONT_MONOSPACE, specialization->font_height);

  specialization->line_width = attributes_get_default_linewidth();
  specialization->line_color = attributes_get_foreground();
  specialization_update_data(specialization);

  for (i = 0; i < 8; i++)
    obj->handles[i]->type = HANDLE_NON_MOVABLE;

  return &specialization->element.object;
}