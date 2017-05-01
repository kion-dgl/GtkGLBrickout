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
static gboolean on_idle(gpointer data);
static gint on_destroy(GtkWidget *widget);
static gboolean on_keydown(GtkWidget *widget, GdkEventKey *event);
static gboolean on_keyup(GtkWidget *widget, GdkEventKey *event);

#define WIDTH 640.0f
#define HEIGHT 480.0f

struct {
	vec3 pos;
	vec3 color;
	GLuint vbo;
	int segments;
	float radius;
	float dx;
	float dy;
} ball;

struct {
	vec3 pos;
	vec3 color;
	GLuint vbo;
	int left_down;
	int right_down;
	float width;
	float height;
	float dx;
} paddle;

struct {
	vec3 pos[60];
	int active[30];
	vec3 color[6];
	float width;
	float height;
	float x_padding;
	float y_padding;
	GLuint vbo;
} bricks;

int init = 0;
GtkWidget *glArea;

GLuint program;
GLuint vao;
GLint attribute_coord2d, uniform_diffuse, uniform_mvp;

int main(int argc, char *argv[]) {
	
	GtkWidget *window;

	gtk_init(&argc, &argv);
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "DashGL - Brickout");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	gtk_window_set_type_hint(GTK_WINDOW(window), 5);
	g_signal_connect(GTK_WIDGET(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "key-press-event", G_CALLBACK(on_keydown), NULL);
	g_signal_connect(window, "key-release-event", G_CALLBACK(on_keyup), NULL);

	glArea = gtk_gl_area_new();
	gtk_widget_set_vexpand(glArea, TRUE);
	gtk_widget_set_hexpand(glArea, TRUE);
	g_signal_connect(glArea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glArea, "render", G_CALLBACK(on_render), NULL);
	gtk_container_add(GTK_CONTAINER(window), glArea);
	
	g_signal_connect(G_OBJECT(glArea), "destroy", G_CALLBACK(on_destroy), NULL);
	// g_signal_connect(G_OBJECT(glArea), "draw", G_CALLBACK(on_render), NULL);
	//gtk_gl_area_set_auto_render(GTK_GL_AREA(glArea), TRUE);

	g_timeout_add(20, on_idle, NULL);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;

}

static void on_realize(GtkGLArea *area) {

	int i, col, row;
	float angle, nextAngle;

	printf("Realize start\n");

	gtk_gl_area_make_current(area);
	if (gtk_gl_area_get_error (area) != NULL) {
		fprintf(stderr, "Unknown error\n");
		return;
	}

	glewExperimental = GL_TRUE;
	glewInit();

	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);
	gtk_gl_area_set_has_depth_buffer(area, TRUE);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	ball.segments = 100;
	ball.radius = 15.0f;
	ball.pos[0] = 320.0f;
	ball.pos[1] = 240.0f;
	ball.pos[2] = 0.0f;
	ball.dx = 3.0f;
	ball.dy = 2.0f;
	ball.color[0] = 1.0f;
	ball.color[1] = 1.0f;
	ball.color[2] = 1.0f;

	// Initialize Ball

	GLfloat *triangle_vertices;
	triangle_vertices = malloc(6 * ball.segments * sizeof(GLfloat));
	
	for(i = 0; i < ball.segments; i++) {

		angle = i * 2.0f * M_PI / (ball.segments - 1);	
		nextAngle = (i+1) * 2.0f * M_PI / (ball.segments - 1);

		triangle_vertices[i*6 + 0] = cos(angle) * ball.radius;
		triangle_vertices[i*6 + 1] = sin(angle) * ball.radius;
		
		triangle_vertices[i*6 + 2] = cos(nextAngle) * ball.radius;
		triangle_vertices[i*6 + 3] = sin(nextAngle) * ball.radius;
	
		triangle_vertices[i*6 + 4] = 0.0f;
		triangle_vertices[i*6 + 5] = 0.0f;

	}

	glGenBuffers(1, &ball.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ball.vbo);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    6 * ball.segments * sizeof(GLfloat),
	    triangle_vertices,
	    GL_STATIC_DRAW
	);
	
	free(triangle_vertices);

	// Initialize Paddle

	paddle.pos[0] = 320.0f;
	paddle.pos[1] = 40.0f;
	paddle.pos[2] = 0.0f;
	paddle.color[0] = 0.85f;
	paddle.color[1] = 0.85f;
	paddle.color[2] = 0.85f;
	paddle.left_down = 0;
	paddle.right_down = 0;
	paddle.width = 60.0f;
	paddle.height = 8.0f;
	paddle.dx = 3.0f;

	GLfloat paddle_vertices[] = {
		-paddle.width, -paddle.height,
		-paddle.width,  paddle.height,
		 paddle.width,  paddle.height,
		 paddle.width,  paddle.height,
		 paddle.width, -paddle.height,
		-paddle.width, -paddle.height
	};

	glGenBuffers(1, &paddle.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, paddle.vbo);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    sizeof(paddle_vertices),
	    paddle_vertices,
	    GL_STATIC_DRAW
	);

	// Initialize Bricks

	bricks.width = 61.0f;
	bricks.height = 14.0f;
	bricks.x_padding = 5.0f;
	bricks.y_padding = 2.0f;
	
	bricks.color[0][0] = 1.0f;
	bricks.color[0][1] = 0.0f;
	bricks.color[0][2] = 0.0f;

	bricks.color[1][0] = 1.0f;
	bricks.color[1][1] = 0.5f;
	bricks.color[1][2] = 0.0f;

	bricks.color[2][0] = 1.0f;
	bricks.color[2][1] = 1.0f;
	bricks.color[2][2] = 0.0f;

	bricks.color[3][0] = 0.0f;
	bricks.color[3][1] = 1.0f;
	bricks.color[3][2] = 0.0f;

	bricks.color[4][0] = 0.0f;
	bricks.color[4][1] = 0.0f;
	bricks.color[4][2] = 1.0f;

	bricks.color[5][0] = 0.8f;
	bricks.color[5][1] = 0.0f;
	bricks.color[5][2] = 1.0f;

	GLfloat brick_vertices[] = {
		-bricks.width, -bricks.height,
		-bricks.width,  bricks.height,
		 bricks.width,  bricks.height,

		 bricks.width,  bricks.height,
		 bricks.width, -bricks.height,
		-bricks.width, -bricks.height
	};

	glGenBuffers(1, &bricks.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, bricks.vbo);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    sizeof(brick_vertices),
	    brick_vertices,
	    GL_STATIC_DRAW
	);

	for(i = 0; i < 30; i++) {
		
		row = i / 5;
		col = i % 5;
		
		bricks.pos[i][0] = bricks.x_padding + bricks.width;
		bricks.pos[i][0] += (bricks.x_padding * col) + (bricks.width * 2 * col);

		bricks.pos[i][1] = bricks.y_padding + bricks.height;
		bricks.pos[i][1] += (bricks.y_padding * row) + (bricks.height * 2 * row);
		bricks.pos[i][1] = HEIGHT - bricks.pos[i][1];

		bricks.pos[i][2] = 0.0f;

	}

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
	mat4_orthographic(0, WIDTH, HEIGHT, 0, ortho);

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

	uniform_name = "diffuse";
	uniform_diffuse = glGetUniformLocation(program, uniform_name);
	if(uniform_diffuse == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	glUseProgram(program);
	glUniformMatrix4fv(uniform_ortho, 1, GL_FALSE, ortho);

	printf("On Realize end\n");
	
	init = 1;

}

static void on_render(GtkGLArea *area, GdkGLContext *conext) {

	int i, row, col;
	mat4 mvp;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  	glBindVertexArray (vao);
  	glEnableVertexAttribArray(attribute_coord2d);
	
	glBindBuffer(GL_ARRAY_BUFFER, ball.vbo);
	glVertexAttribPointer(
	    attribute_coord2d,
	    2,
	    GL_FLOAT,
	    GL_FALSE,
	    0,
	    0
	);

	mat4_translate(ball.pos, mvp);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);
	glUniform3fv(uniform_diffuse, 1, ball.color);
	glDrawArrays(GL_TRIANGLES, 0, ball.segments * 3);

	glBindBuffer(GL_ARRAY_BUFFER, paddle.vbo);
	glVertexAttribPointer(
	    attribute_coord2d,
	    2,
	    GL_FLOAT,
	    GL_FALSE,
	    0,
	    0
	);

	mat4_translate(paddle.pos, mvp);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);
	glUniform3fv(uniform_diffuse, 1, paddle.color);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	

	glBindBuffer(GL_ARRAY_BUFFER, bricks.vbo);
	glVertexAttribPointer(
	    attribute_coord2d,
	    2,
	    GL_FLOAT,
	    GL_FALSE,
	    0,
	    0
	);

	for(i = 0; i < 30; i++) {
		
		row = i / 5;
		col = i % 5;

		if(col == 0) {
			glUniform3fv(uniform_diffuse, 1, bricks.color[row]);
		}

		mat4_translate(bricks.pos[i], mvp);
		glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
	}


	glDisableVertexAttribArray(attribute_coord2d);

}

static gboolean on_idle(gpointer data) {

	if( init == 0 ) {
		return FALSE;
	}
	
	// Advance Ball

	ball.pos[0] += ball.dx;
	ball.pos[1] += ball.dy; 

	if(ball.pos[0] + ball.radius > WIDTH) {
		ball.pos[0] = WIDTH - ball.radius;
		ball.dx = -ball.dx;
	} else if(ball.pos[0] - ball.radius < 0.0f){
		ball.pos[0] = ball.radius;
		ball.dx = -ball.dx;
	}

	if(ball.pos[1] + ball.radius > HEIGHT) {
		ball.pos[1] = HEIGHT - ball.radius;
		ball.dy = -ball.dy;
	} else if(ball.pos[1] - ball.radius < 0.0f){
		ball.pos[1] = ball.radius;
		ball.dy = -ball.dy;
	}

	// Advance Paddle

	if(paddle.left_down) {
		paddle.pos[0] -= paddle.dx;
	}
	if(paddle.right_down) {
		paddle.pos[0] += paddle.dx;
	}

	if(paddle.pos[0] < 0.0f) {
		paddle.pos[0] = 0.0f;
	} else if(paddle.pos[0] > WIDTH) {
		paddle.pos[0] = WIDTH;
	}

	gtk_widget_queue_draw(glArea);

	return TRUE;

}

static gint on_destroy(GtkWidget *widget) {

	printf("Widget destroyed\n");
	
}

static gboolean on_keydown(GtkWidget *widget, GdkEventKey *event) {

	switch(event->keyval) {
		case GDK_KEY_Left:
			paddle.left_down = TRUE;
		break;
		case GDK_KEY_Right:
			paddle.right_down = TRUE;
		break;
	}

}

static gboolean on_keyup(GtkWidget *widget, GdkEventKey *event) {

	switch(event->keyval) {
		case GDK_KEY_Left:
			paddle.left_down = FALSE;
		break;
		case GDK_KEY_Right:
			paddle.right_down = FALSE;
		break;
	}

}
