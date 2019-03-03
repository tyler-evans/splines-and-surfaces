// Display a cube, using glDrawElements

#include "common.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const char *WINDOW_TITLE = "Cube with Indices";
const double FRAME_RATE_MS = 1000.0/60.0;

typedef glm::vec4  color4;
typedef glm::vec4  point4;
typedef glm::vec2  point2;

int width = 640;
int height = 640;


int dragging_point_index = -1;


float bezier_array[16] = {
	-1.0, 3.0, -3.0, 1.0,
	3.0, -6.0, 3.0, 0.0,
	-3.0, 3.0, 0.0, 0.0,
	1.0, 0.0, 0.0, 0.0
};
glm::mat4 bezier_matrix = glm::make_mat4(bezier_array);


const int num_control_points = 4;
point4 control_points[num_control_points] = {
	point4(-0.5, -0.5, 0.0, 1.0),
	point4(-0.1, 0.0, 0.0, 1.0),
	point4(0.3, 0.0, 0.0, 1.0),
	point4(0.6, -0.5, 0.0, 1.0)
};


const int num_increments = 50;
float xs[num_increments];
float ys[num_increments];

point4 vertices[num_increments];

// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int      Axis = Xaxis;
GLfloat  Theta[NumAxes] = { 0.0, 0.0, 0.0 };

GLuint  ModelView, Projection;


point2 mouse_to_world(int x, int y) {
	return point2(2.0 * float(x) / width - 1.0, -2.0 * float(y) / height + 1.0);
}


float time_multiply(float t, point4 v) {
	return ((v[0] * t + v[1]) * t + v[2]) * t + v[3];
};


void draw_bezier_curve(point4 *control_points) {

	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(point4), control_points, GL_STATIC_DRAW);
	glPointSize(10.0f);
	glDrawArrays(GL_POINTS, 0, 4);

	point4 bezier_partial_result_x = bezier_matrix * point4(control_points[0][0], control_points[1][0], control_points[2][0], control_points[3][0]);
	point4 bezier_partial_result_y = bezier_matrix * point4(control_points[0][1], control_points[1][1], control_points[2][1], control_points[3][1]);

	for (int i = 0; i < num_increments; i++) {
		xs[i] = time_multiply(1.0 / (num_increments - 1) * i, bezier_partial_result_x);
		ys[i] = time_multiply(1.0 / (num_increments - 1) * i, bezier_partial_result_y);
	}


	for (int i = 0; i < num_increments; i++) {
		vertices[i] = point4(xs[i], ys[i], 0.0, 1.0);
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glDrawArrays(GL_LINE_STRIP, 0, num_increments);
};


void mouse_callback(int mouse_x, int mouse_y) {
	point2 mouse_coords = mouse_to_world(mouse_x, mouse_y);
	float x = mouse_coords.x;
	float y = mouse_coords.y;

	for (int i = 0; i < num_control_points; i++) {
		if (dragging_point_index != -1)
			i = dragging_point_index;

		if (length(control_points[i] - point4(x, y, 0.0, 1.0)) < 0.1) {
			dragging_point_index = i;
			control_points[i].x = x;
			control_points[i].y = y;
			break;
		}
		else if (dragging_point_index != -1)
			dragging_point_index = -1;
	}
};

//----------------------------------------------------------------------------

// OpenGL initialization
void
init()
{
   // Create a vertex array object
   GLuint vao = 0;
   glGenVertexArrays( 1, &vao );
   glBindVertexArray( vao );

   GLuint buffer;

   // Create and initialize a buffer object
   glGenBuffers( 1, &buffer );
   glBindBuffer( GL_ARRAY_BUFFER, buffer );
   glBufferData(GL_ARRAY_BUFFER, sizeof(control_points), control_points , GL_STATIC_DRAW);


   // Load shaders and use the resulting shader program
   GLuint program = InitShader( "vshader6.glsl", "fshader5.glsl" );
   glUseProgram( program );

   // set up vertex arrays
   GLuint vPosition = glGetAttribLocation( program, "vPosition" );
   glEnableVertexAttribArray( vPosition );
   glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

   ModelView = glGetUniformLocation( program, "ModelView" );
   Projection = glGetUniformLocation( program, "Projection" );

   glEnable( GL_DEPTH_TEST );
   glClearColor( 1.0, 1.0, 1.0, 1.0 );

   glutMotionFunc(mouse_callback);

}




//----------------------------------------------------------------------------

void
display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


   //  Generate the model-view matrix
   const glm::vec3 viewer_pos(0.0, 0.0, 1.0);
   glm::mat4 trans, rot, model_view;
   trans = glm::translate(trans, -viewer_pos);
   model_view = trans * rot;
   glUniformMatrix4fv(ModelView, 1, GL_FALSE, glm::value_ptr(model_view));


   draw_bezier_curve(control_points);
   

   glutSwapBuffers();
}

//----------------------------------------------------------------------------

void
keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
       case 033: // Escape Key
       case 'q': case 'Q':
          exit( EXIT_SUCCESS );
          break;
    }
}

//----------------------------------------------------------------------------

void
mouse( int button, int state, int x, int y )
{
	if (state == GLUT_UP)
		dragging_point_index = -1;
}

//----------------------------------------------------------------------------

void
update( void )
{
}

//----------------------------------------------------------------------------

void
reshape( int w, int h )
{
   glViewport( 0, 0, w, h );

   GLfloat aspect = GLfloat(w)/h;
   //glm::mat4  projection = glm::perspective( glm::radians(45.0f), aspect, 0.5f, 3.0f );
   glm::mat4 projection;

   glUniformMatrix4fv( Projection, 1, GL_FALSE, glm::value_ptr(projection) );

   width = w;
   height = h;
}
