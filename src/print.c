#include <ncurses.h>

#include "print.h"
#include "vector.h"
#include "term_shapes.h"

/*
 * translate an x and y value based on the window dimensions and some magic
 * numbers so an object described with a "radius" approximately 1 will be
 * centered in the center of the screen and entirely fit on the screen
 */
static
void
movexy(double *x, double *y)
{
	int winx, winy;
	double movex, movey;

	getmaxyx(stdscr, winy, winx);

	movex = (int) ((*x * SCALE * winy) + (0.5 * winx));
	movey = (int) (-(*y * SCALE * .5 * winy) + (0.5 * winy));

	*x = movex;
	*y = movey;
}

/*
 * prints the edges by calculating the normal vector from two given points, and
 * then prints points along the edge by multiplying the normal
 */
static
void
print_edges(struct shape *s)
{
	char occlude_val;
	ssize_t fronts_index, behinds_index;
	int i, k;
	double x0, y0, z0, v_len, x, y, z, movex, movey;
	point3 v, u;

	fronts_index = 0;
	behinds_index = 0;

	/* iterates over the edges */
	for (i = s->num_e - 1; i >= 0; --i) {
		v = s->vertices[s->edges[i].edge[0]];
		x0 = v.x;
		y0 = v.y;
		z0 = v.z;

		/* v is the vector given by two points */
		v = vector3_sub(s->vertices[s->edges[i].edge[1]],
				s->vertices[s->edges[i].edge[0]]);
		v_len = vector3_mag(v);

		/* u is the unit vector from v */
		u = vector3_unit(v);

		/*
		 * prints points along the edge
		 *
		 * e_density is a natural number directly corresponding to the
		 * number of points printed along the edge
		 */
		for (k = 0; k <= s->e_density; ++k) {
			x = x0 + ((k / (double) s->e_density * v_len) * u.x);
			y = y0 + ((k / (double) s->e_density * v_len) * u.y);
			z = z0 + ((k / (double) s->e_density * v_len) * u.z);

			/*
			 * if the occlusion flag is set and a point shouldn't
			 * be occluded, the rest of the loop prints the point
			 */
			occlude_val = occlude_point(s, (point3) {x, y, z}, s->edges[i]);
			if (s->occlusion == CONVEX && occlude_val) {
				continue;
			}

			movex = x;
			movey = y;
			movexy(&movex, &movey);

			/*
			 * renders the rear and front symbols based on whether
			 * the point is detected to be "behind" or "in front"
			 */
#if USE_NCURSES
			if (occlude_val == 1) {
				s->behinds[behinds_index].x = movex;
				s->behinds[behinds_index].y = movey;
				behinds_index++;
			} else {
				s->fronts[fronts_index].x = movex;
				s->fronts[fronts_index].y = movey;
				fronts_index++;
			}
#endif
		}
	}

#if USE_NCURSES
	/* print all the points behind */
	if (s->occlusion != CONVEX) {
		for (ssize_t j = 0; j < behinds_index - 1; ++j) {
			mvprintw(s->behinds[j].y,
				 s->behinds[j].x,
				 "%c", s->rear_symbol);
		}
	}

	/* print all the points in front */
	for (ssize_t j = 0; j < fronts_index - 1; ++j) {
		mvprintw(s->fronts[j].y,
			 s->fronts[j].x,
			 "%c", s->front_symbol);
	}
#endif
}

/*
 * prints the vertices with their index (to make connecting edges easier)
 *
 * vertices are printed backwards so that lower numbered indices are printed in
 * front of the higher number indices if they overlap
 */
static
void
print_vertices(struct shape *s)
{
	int i;
	double x, y, z;

	struct edge edge;

	/* specifically invalid edge */
	edge.edge[0] = -1;
	edge.edge[1] = -1;

	for (i = s->num_v - 1; i >= 0; --i) {
		x = s->vertices[i].x;
		y = s->vertices[i].y;
		z = s->vertices[i].z;

		if (s->occlusion &&
			occlude_point(s, (point3) {x, y, z}, edge)) {
			continue;
		}

		movexy(&x, &y);
#if USE_NCURSES
		mvprintw(y, x, "%i", i);
#endif
	}
}

/*
 * only prints edges if edges are stored in the shape struct
 */
void
print_shape(struct shape *s)
{
	if (s->print_edges && s->num_e) {
		print_edges(s);
	}

	if (s->print_vertices) {
		print_vertices(s);
	}
}