/*
 *  This file is part of DashGL.com - Gtk - Brickout Tutorial
 *  Copyright (C) 2017 Benjamin Collins
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License version 2
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <gtk/gtk.h>
#include "lib/dashgl.h"

static void on_realize(GtkGLArea *area);
static void on_render(GtkGLArea *area, GdkGLContext *conext);

GLuint program, num_segments;
GLuint vao, vbo_triangle;
GLint attribute_coord2d, uniform_mvp;

int main(int argc, char *argv[]) {
	
	GtkWidget *window;
	GtkWidget *glArea;

	gtk_init(&argc, &argv);
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "DashGL - Brickout");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	gtk_window_set_type_hint(GTK_WINDOW(window), 5);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	glArea = gtk_gl_area_new();
	gtk_widget_set_vexpand(glArea, TRUE);
	gtk_widget_set_hexpand(glArea, TRUE);
	g_signal_connect(glArea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glArea, "render", G_CALLBACK(on_render), NULL);
	gtk_container_add(GTK_CONTAINER(window), glArea);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;

}

static void on_realize(GtkGLArea *area) {

	int i;
	float angle, nextAngle;

	g_print("on_realize\n");
	
	gtk_gl_area_make_current(area);
	if (gtk_gl_area_get_error (area) != NULL) {
		fprintf(stderr, "Unknown error\n");
		return;
	}

	g_print("00\n");

	glewExperimental = GL_TRUE;
	glewInit();

	g_print("01\n");

	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	gtk_gl_area_set_has_depth_buffer(area, TRUE);
	
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	g_print("02\n");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	num_segments = 100;
	
	/*
	GLfloat triangle_vertices[] = {
		-0.5f, -0.5f,
		-0.5f,  0.5f,
		 0.5f,  0.5f,

		 0.5f,  0.5f,
		 0.5f, -0.5f,
		-0.5f, -0.5f
	};
	*/

	GLfloat *triangle_vertices;
	triangle_vertices = malloc(6 * num_segments * sizeof(GLfloat));
	
	for(i = 0; i < num_segments; i++) {

		angle = i * 2.0f * M_PI / (num_segments - 1);	
		nextAngle = (i+1) * 2.0f * M_PI / (num_segments - 1);

		triangle_vertices[i*6 + 0] = cos(angle) * 20;
		triangle_vertices[i*6 + 1] = sin(angle) * 20;
		
		triangle_vertices[i*6 + 2] = cos(nextAngle) * 20;
		triangle_vertices[i*6 + 3] = sin(nextAngle) * 20;
	
		triangle_vertices[i*6 + 4] = 0.0f;
		triangle_vertices[i*6 + 5] = 0.0f;

	}

	glGenBuffers(1, &vbo_triangle);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    6 * num_segments * sizeof(GLfloat),
	    triangle_vertices,
	    GL_STATIC_DRAW
	);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glDisableVertexAttribArray(0);

	program = dash_create_program("sdr/vertex.glsl", "sdr/fragment.glsl");
	if(program == 0) {
		fprintf(stderr, "Program creation error\n");
		exit(1);
	}

	const char *attribute_name = "coord2d";
	attribute_coord2d = glGetAttribLocation(program, attribute_name);
	if(attribute_coord2d == -1) {
	    fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
	    return;
	}
	
	GLint uniform_ortho;
	mat4 ortho;
	mat4_orthographic(0, 640, 480, 0, ortho);

	const char *uniform_name;
	uniform_name = "ortho";
	uniform_ortho = glGetUniformLocation(program, uniform_name);
	if(uniform_ortho == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	uniform_name = "mvp";
	uniform_mvp = glGetUniformLocation(program, uniform_name);
	if(uniform_mvp == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	glUseProgram(program);
	glUniformMatrix4fv(uniform_ortho, 1, GL_FALSE, ortho);

}

static void on_render(GtkGLArea *area, GdkGLContext *conext) {

	g_print("on_render\n");
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  	glBindVertexArray (vao);
  	glEnableVertexAttribArray(attribute_coord2d);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
	glVertexAttribPointer(
	    attribute_coord2d,
	    2,
	    GL_FLOAT,
	    GL_FALSE,
	    0,
	    0
	);

	mat4 pos;
	vec3 t = { 320.0f, 240.0f, 0.0f };
	mat4_translate(t, pos);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, pos);

	glDrawArrays(GL_TRIANGLES, 0, num_segments * 3);
	glDisableVertexAttribArray(attribute_coord2d);

}
