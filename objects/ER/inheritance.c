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
#include <lib/attributes.h>

#include "intl.h"
#include "object.h"
#include "connection.h"
#include "diarenderer.h"
#include "arrows.h"
#include "properties.h"
#include "create.h"

#include "pixmaps/inheritance.xpm"


#define INHERITANCE_ARROW_WIDTH 0.8

/*!
 * \brief ER - Inheritance: a _Connection used for linking Entity specialization constraint to entities
 *
 * The ER - Inheritance object implements a connection between a specialization/generalization
 * contraint in the Enhanced Entity-Relationship notation used in "Fundamentals of database systems"
 * by Elmasri, Ramez, and Sham Navathe.
 *
 * \extends _Connection
 * \ingroup ER
 */
typedef struct _Inheritance {
  Connection connection;

  double line_width;
} Inheritance;


static ObjectChange* inheritance_move_handle(Inheritance *inheritance, Handle *handle,
                                             Point *to, ConnectionPoint *cp,
                                             HandleMoveReason reason,
                                             ModifierKeys modifiers);
static ObjectChange* inheritance_move(Inheritance *inheritance, Point *to);
static void inheritance_select(Inheritance *inheritance, Point *clicked_point,
                               DiaRenderer *interactive_renderer);
static Point inheritance_calc_arrow_center(Point *from, Point *to);
static void inheritance_draw(Inheritance *inheritance, DiaRenderer *renderer);
static DiaObject *inheritance_create(Point *startpoint,
                              void *user_data,
                              Handle **handle1,
                              Handle **handle2);
static real inheritance_distance_from(Inheritance *inheritance, Point *point);
static void inheritance_update_data(Inheritance *inheritance);
static void inheritance_destroy(Inheritance *inheritance);
static DiaObject *inheritance_copy(Inheritance *inheritance);

static void inheritance_set_props(Inheritance *inheritance, GPtrArray *props);

static void inheritance_save(Inheritance *inheritance, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *inheritance_load(ObjectNode obj_node, int version, DiaContext *ctx);
static gboolean inheritance_transform(Inheritance *inheritance, const DiaMatrix *m);

static ObjectTypeOps inheritance_type_ops =
    {
        (CreateFunc) inheritance_create,
        (LoadFunc)   inheritance_load,
        (SaveFunc)   inheritance_save,
        (GetDefaultsFunc)   NULL,
        (ApplyDefaultsFunc) NULL
    };


static PropDescription inheritance_props[] = {
    OBJECT_COMMON_PROPERTIES,
    PROP_DESC_END
};

static PropOffset inheritance_offsets[] = {
    OBJECT_COMMON_PROPERTIES_OFFSETS,
    { NULL, 0, 0 }
};

DiaObjectType inheritance_type =
{
    "ER - Inheritance",  /* name */
    0,                  /* version */
    inheritance_xpm,            /* pixmap */
    &inheritance_type_ops,      /* ops */
    NULL,
    0,
    inheritance_props,
    inheritance_offsets
};

static ObjectOps inheritance_ops = {
    (DestroyFunc)         inheritance_destroy,
    (DrawFunc)            inheritance_draw,
    (DistanceFunc)        inheritance_distance_from,
    (SelectFunc)          inheritance_select,
    (CopyFunc)            inheritance_copy,
    (MoveFunc)            inheritance_move,
    (MoveHandleFunc)      inheritance_move_handle,
    (GetPropertiesFunc)   object_create_props_dialog,
    (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
    (ObjectMenuFunc)      NULL,
    (DescribePropsFunc)   object_describe_props,
    (GetPropsFunc)        object_get_props,
    (SetPropsFunc)        inheritance_set_props,
    (TextEditFunc) NULL,
    (ApplyPropertiesListFunc) object_apply_props,
    (TransformFunc)       inheritance_transform,
};


static void
inheritance_set_props (Inheritance *inheritance, GPtrArray *props)
{
  object_set_props_from_offsets (DIA_OBJECT (inheritance),
                                 inheritance_offsets, props);
  inheritance_update_data (inheritance);
}

static gboolean
inheritance_transform(Inheritance *inheritance, const DiaMatrix *m)
{
  int i;

  g_return_val_if_fail (m != NULL, FALSE);

  for (i = 0; i < 2; i++)
    transform_point (&inheritance->connection.endpoints[i], m);

  inheritance_update_data(inheritance);
  return TRUE;
}

static Point
inheritance_calc_arrow_center(Point *from, Point *to) {
  Point center;

  center.x = from->x + (to->x - from->x) / 3.0;
  center.y = from->y + (to->y - from->y) / 3.0;

  return center;
}

static real
inheritance_distance_from(Inheritance *inheritance, Point *point)
{
  Point *endpoints;
  Point center;
  real linedist, arrowdist;

  endpoints = inheritance->connection.endpoints;

  center = inheritance_calc_arrow_center(&endpoints[0], &endpoints[1]);

  linedist = distance_line_point(&endpoints[0], &endpoints[1], inheritance->line_width, point);
  arrowdist = distance_ellipse_point(&center,
                                     INHERITANCE_ARROW_WIDTH + 1.0, INHERITANCE_ARROW_WIDTH + 1.0,
                                     inheritance->line_width,
                                     point);

  return linedist > arrowdist ? arrowdist : linedist;
}

static void
inheritance_select(Inheritance *inheritance, Point *clicked_point,
                   DiaRenderer *interactive_renderer)
{
  connection_update_handles(&inheritance->connection);
}

static ObjectChange*
inheritance_move_handle(Inheritance *inheritance, Handle *handle,
                        Point *to, ConnectionPoint *cp,
                        HandleMoveReason reason, ModifierKeys modifiers)
{
  connection_move_handle(&inheritance->connection, handle->id, to, cp, reason, modifiers);
  connection_adjust_for_autogap(&inheritance->connection);

  inheritance_update_data(inheritance);

  return NULL;
}

static ObjectChange*
inheritance_move(Inheritance *inheritance, Point *to)
{
  Point delta;
  Point *endpoints = inheritance->connection.endpoints;

  delta = endpoints[1];
  point_sub(&delta, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &delta);

  inheritance_update_data(inheritance);

  return NULL;
}

static void
inheritance_draw (Inheritance *inheritance, DiaRenderer *renderer)
{
  Point *from, *to;
  Point delta, center;
  double line_slope, line_len;
  real angle, angle_start, angle_end;

  Connection *conn = &inheritance->connection;

  from = &conn->endpoints[0];
  to = &conn->endpoints[1];

  delta = *to;
  point_sub(&delta, from);

  line_slope = delta.y / delta.x;
  line_len = sqrt(pow(delta.x, 2) + pow(delta.y, 2));

  dia_renderer_set_linewidth(renderer, inheritance->line_width);
  dia_renderer_set_linestyle(renderer, LINESTYLE_SOLID, 0.0);
  dia_renderer_set_linejoin(renderer, LINEJOIN_MITER);
  dia_renderer_set_linecaps(renderer, LINECAPS_BUTT);

  if (line_len > 2.0) {
    center = inheritance_calc_arrow_center(from, to);

    angle = -(atan(line_slope) * 180 / M_PI);

    angle_start = MIN(angle - 90, angle + 90);
    angle_end = MAX(angle - 90, angle + 90);

    if (delta.x < 0) {
      angle_start += 180;
      angle_end += 180;
    }

    dia_renderer_draw_arc(renderer, &center,
                         INHERITANCE_ARROW_WIDTH, INHERITANCE_ARROW_WIDTH,
                         angle_start, angle_end,
                         &color_black);
  }

  dia_renderer_draw_line(renderer,
                         from, to,
                         &color_black);
}

static DiaObject *
inheritance_create(Point *startpoint,
            void *user_data,
            Handle **handle1,
            Handle **handle2)
{
  Inheritance *inheritance;
  Connection *conn;
  DiaObject *obj;
  Point initial_len = {1.0, 1.0 };

  inheritance = g_malloc0(sizeof(Inheritance));

  conn = &inheritance->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &initial_len);

  obj = &conn->object;
  obj->type = &inheritance_type;
  obj->ops = &inheritance_ops;

  connection_init(conn, 2, 0);
  inheritance->line_width = attributes_get_default_linewidth();

  inheritance_update_data(inheritance);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &inheritance->connection.object;
}

static void
inheritance_destroy(Inheritance *inheritance)
{
  connection_destroy(&inheritance->connection);
}

static DiaObject *
inheritance_copy(Inheritance *inheritance)
{
  Inheritance *newinheritance = g_malloc0(sizeof(Inheritance));

  connection_copy(&inheritance->connection, &newinheritance->connection);
  newinheritance->line_width = inheritance->line_width;
  inheritance_update_data(newinheritance);

  return &newinheritance->connection.object;
}

static void
inheritance_update_bbox(Inheritance *inheritance) {
  Connection *conn = &inheritance->connection;
  DiaObject *obj = &conn->object;
  Point center;
  DiaRectangle bbox;

  connection_update_boundingbox(conn);

  center = inheritance_calc_arrow_center(&conn->endpoints[0], &conn->endpoints[1]);
  bbox.bottom = center.y + INHERITANCE_ARROW_WIDTH;
  bbox.top = center.y - INHERITANCE_ARROW_WIDTH;
  bbox.left = center.x - INHERITANCE_ARROW_WIDTH;
  bbox.right = center.x + INHERITANCE_ARROW_WIDTH;

  rectangle_union (&obj->bounding_box, &bbox);
}

static void
inheritance_update_data(Inheritance *inheritance)
{
  Connection *conn = &inheritance->connection;
  DiaObject *obj = &conn->object;
  LineBBExtras *extra = &conn->extra_spacing;

  extra->start_trans =
  extra->end_trans   =
  extra->start_long  =
  extra->end_long    = inheritance->line_width / 2.0;

  if (connpoint_is_autogap(inheritance->connection.endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(inheritance->connection.endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }

  inheritance_update_bbox(inheritance);

  obj->position = conn->endpoints[0];
  connection_update_handles(conn);
}


static void
inheritance_save(Inheritance *inheritance, ObjectNode obj_node, DiaContext *ctx)
{
  connection_save(&inheritance->connection, obj_node, ctx);
}

static DiaObject *
inheritance_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Inheritance *inheritance;
  Connection *conn;
  DiaObject *obj;

  inheritance = g_malloc0(sizeof(Inheritance));

  conn = &inheritance->connection;
  obj = &conn->object;

  obj->type = &inheritance_type;
  obj->ops = &inheritance_ops;

  connection_load(conn, obj_node, ctx);
  connection_init(conn, 2, 0);

  inheritance->line_width = attributes_get_default_linewidth();

  inheritance_update_data(inheritance);

  return &inheritance->connection.object;
}
