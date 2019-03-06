#include "common.h"
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const char *WINDOW_TITLE = "Splines";
const double FRAME_RATE_MS = 1000.0/60.0;

typedef glm::vec4  color4;
typedef glm::vec4  point4;
typedef glm::vec2  point2;

int width = 640;
int height = 640;
int dragging_point_index = -1;

std::vector<point4> control_points;

glm::mat4 bezier_matrix = transpose(glm::mat4(
	-1.0, 3.0, -3.0, 1.0,
	3.0, -6.0, 3.0, 0.0,
	-3.0, 3.0, 0.0, 0.0,
	1.0, 0.0, 0.0, 0.0
));

// Hermite matrix * catmull-rom coefficient matrix
glm::mat4 catmull_rom_matrix = transpose(glm::mat4(
	-1.0, 3.0, -3.0, 1.0,
	2.0, -5.0, 4.0, -1.0,
	-1.0, 0.0, 1.0, 0.0,
	0.0, 2.0, 0.0, 0.0
)) / 2.0f;

glm::mat4 b_spline_matrix = transpose(glm::mat4(
	-1.0, 3.0, -3.0, 1.0,
	3.0, -6.0, 3.0, 0.0,
	-3.0, 0.0, 3.0, 0.0,
	1.0, 4.0, 1.0, 0.0
)) / 6.0f;


const int num_increments = 100;
float xs[num_increments];
float ys[num_increments];

point4 vertices[num_increments];
GLuint  ModelView, Projection;


point2 mouse_to_world(int x, int y) {
	return point2(2.0 * float(x) / width - 1.0, -2.0 * float(y) / height + 1.0);
}


float time_multiply(float t, point4 v) {
	return ((v[0] * t + v[1]) * t + v[2]) * t + v[3];
};


void mouse_callback(int mouse_x, int mouse_y) {
	point2 mouse_coords = mouse_to_world(mouse_x, mouse_y);
	float x = mouse_coords.x;
	float y = mouse_coords.y;

	if (dragging_point_index != -1) {
		control_points[dragging_point_index].x = x;
		control_points[dragging_point_index].y = y;
	}
};


class CurveSegment {
	point4 *control_points;
	glm::mat4 coeff_matrix;
public:
	CurveSegment(point4 *cps, glm::mat4 matrix) {
		control_points = cps;
		coeff_matrix = matrix;
	}
	void draw() {
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(point4), control_points, GL_STATIC_DRAW);
		glPointSize(10.0f);
		glDrawArrays(GL_POINTS, 0, 4);

		point4 partial_result_x = coeff_matrix * point4(control_points[0][0], control_points[1][0], control_points[2][0], control_points[3][0]);
		point4 partial_result_y = coeff_matrix * point4(control_points[0][1], control_points[1][1], control_points[2][1], control_points[3][1]);

		for (int i = 0; i < num_increments; i++) {
			xs[i] = time_multiply(1.0 / (num_increments - 1) * i, partial_result_x);
			ys[i] = time_multiply(1.0 / (num_increments - 1) * i, partial_result_y);
		}

		for (int i = 0; i < num_increments; i++) {
			vertices[i] = point4(xs[i], ys[i], 0.0, 1.0);
		}

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glDrawArrays(GL_LINE_STRIP, 0, num_increments);
	}

};


class Curve {
public:
	virtual void draw() = 0;
	virtual void add_control_point(point4 cp) = 0;
};


class BezierCurve : public Curve {
public:
	virtual void draw() {
		for (int i = 0; i < control_points.size() / 3; i++) {
			CurveSegment seg = CurveSegment(&control_points[3 * i], bezier_matrix);
			seg.draw();
		}
	}
	virtual void add_control_point(point4 cp) {
		point4 p2 = control_points.end()[-2];
		point4 p3 = control_points.end()[-1];
		control_points.push_back(p3 + 0.3f*(p3-p2));
		control_points.push_back(0.5f*(cp + p3));
		control_points.push_back(cp);
	}
};


class CatmullRomCurve : public Curve {
public:
	CatmullRomCurve() {};
	virtual void draw() {
		for (int i = 0; i < control_points.size() - 3; i++) {
			CurveSegment seg = CurveSegment(&control_points[i], catmull_rom_matrix);
			seg.draw();
		}
	}
	virtual void add_control_point(point4 cp) {
		control_points.push_back(cp);
	}
};

class BSplineCurve : public Curve {
public:
	virtual void draw() {
		for (int i = 0; i < control_points.size() - 3; i++) {
			CurveSegment seg = CurveSegment(&control_points[i], b_spline_matrix);
			seg.draw();
		}
	}
	virtual void add_control_point(point4 cp) {
		control_points.push_back(cp);
	}
};

BezierCurve default_curve = BezierCurve();
Curve *curve = &default_curve;

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
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices , GL_STATIC_DRAW);


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

   control_points.push_back(point4(-0.5, -0.5, 0.0, 1.0));
   control_points.push_back(point4(-0.1, 0.0, 0.0, 1.0));
   control_points.push_back(point4(0.3, 0.0, 0.0, 1.0));
   control_points.push_back(point4(0.6, -0.5, 0.0, 1.0));

   std::cout << "Defaulting to Bezier Curve\n";
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

   curve->draw();

   glutSwapBuffers();
}

//----------------------------------------------------------------------------
BezierCurve bezier_curve;
CatmullRomCurve catmull_rom_curve;
BSplineCurve b_spline_curve;
int curve_index = 0;
void
keyboard( unsigned char key, int x, int y )
{
     switch( key ) {
       case 033: // Escape Key
       case 'q': case 'Q':
          exit( EXIT_SUCCESS );
	   case ' ': //space bar
		   if (curve_index == 0) {
			   curve = &catmull_rom_curve;
			   std::cout << "Switching to Catmull-Rom Curve\n";
		   }
		   else if (curve_index == 1) {
			   curve = &b_spline_curve;
			   std::cout << "Switching to Uniform B-Spline Curve\n";
		   }
		   else {
			   curve = &bezier_curve;
			   std::cout << "Switching to Bezier Curve\n";
		   }
		   curve_index = (curve_index+1) % 3;
    }
}

//----------------------------------------------------------------------------

void
mouse(int button, int state, int mouse_x, int mouse_y)
{
	if (state == GLUT_DOWN) {
		point2 mouse_coords = mouse_to_world(mouse_x, mouse_y);
		float x = mouse_coords.x;
		float y = mouse_coords.y;
		dragging_point_index = -1;

		for (int i = 0; i < control_points.size(); i++) {
			if (length(control_points[i] - point4(x, y, 0.0, 1.0)) < 0.1) {
				dragging_point_index = i;
				break;
			}
		}
		if (dragging_point_index == -1) { // new point
			std::cout << "Adding new control point\n";
			point4 new_point = point4(x, y, 0.0, 1.0);
			curve->add_control_point(new_point);

		}
	}
	else if (state == GLUT_UP) {
		dragging_point_index = -1;
	}
}

//----------------------------------------------------------------------------
void update( void ){}
//----------------------------------------------------------------------------

void
reshape( int w, int h )
{
   glViewport( 0, 0, w, h );
   width = w;
   height = h;
}
