
#include "common.h"
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const char *WINDOW_TITLE = "Teaset";
const double FRAME_RATE_MS = 1000.0/60.0;

typedef glm::vec4  color4;
typedef glm::vec4  point4;
typedef glm::vec3  point3;
typedef glm::vec2  point2;


const int num_increments = 10;
point4 line_vertices[num_increments*2];






glm::mat4 M = glm::mat4(
	-1.0, 3.0, -3.0, 1.0,
	3.0, -6.0, 3.0, 0.0,
	-3.0, 3.0, 0.0, 0.0,
	1.0, 0.0, 0.0, 0.0
);


// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int      Axis = Xaxis;
GLfloat  Theta[NumAxes] = { 0.0, 0.0, 0.0 };

GLuint  ModelView, Projection;



class PatchIndex {
public:
	std::vector<int> cp_idxs;
	PatchIndex(std::vector<int> cp_idxs_) {
		cp_idxs = cp_idxs_;
	}
};

std::vector<point3> loaded_points;
std::vector<PatchIndex> patch_indices;
void load_patch(char *filename, int *patches, int *verticies)
{
	int ii;
	float x, y, z;
	int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;

	FILE *fp;

	if (!(fp = fopen(filename, "r"))) {
		fprintf(stderr, "Load_patch: Can't open %s\n", filename);
		exit(1);
	}

	(void)fscanf(fp, "%i\n", patches);
	for (ii = 0; ii < *patches; ii++) {
		(void)fscanf(fp, "%i, %i, %i, %i,", &a, &b, &c, &d);
		(void)fscanf(fp, "%i, %i, %i, %i,", &e, &f, &g, &h);
		(void)fscanf(fp, "%i, %i, %i, %i,", &i, &j, &k, &l);
		(void)fscanf(fp, "%i, %i, %i, %i\n", &m, &n, &o, &p);
		std::vector<int> cp_indices({ a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p });
		patch_indices.push_back(PatchIndex(cp_indices));
	}
	(void)fscanf(fp, "%i\n", verticies);
	for (ii = 1; ii <= *verticies; ii++) {
		(void)fscanf(fp, "%f, %f, %f\n", &x, &y, &z);
		loaded_points.push_back(point3(x, y, z));
	}
}




class BezierPatch {
public:
	glm::mat4 MGM_x, MGM_y, MGM_z;
	BezierPatch(point3 *cps_) {
		MGM_x = BezierPatch::construct_MGM(BezierPatch::get_slice(cps_, 0));
		MGM_y = BezierPatch::construct_MGM(BezierPatch::get_slice(cps_, 1));
		MGM_z = BezierPatch::construct_MGM(BezierPatch::get_slice(cps_, 2));
	}
	glm::mat4 construct_MGM(float slice[16]) {
		glm::mat4 G = transpose(glm::make_mat4(slice));
		return M * G * transpose(M);
	}
	float *get_slice(point3 *arr, int j) {
		float *slice = new float[16];
		for (int i = 0; i < 16; i++)
			slice[i] = arr[i][j];
		return slice;
	}
	point4 patch_point(glm::mat4 MGM_x, glm::mat4 MGM_y, glm::mat4 MGM_z, float u, float v) {
		point4 U = point4(u*u*u, u*u, u, 1.0);
		point4 V = point4(v*v*v, v*v, v, 1.0);
		return point4(dot(U, MGM_x * V), dot(U, MGM_y * V), dot(U, MGM_z * V), 1.0);
	}
	void draw() {
		for (int i = 0; i < num_increments; i++) {
			for (int j = 0; j < num_increments; j++) {
				float u = 1.0 / (num_increments - 1) * i;
				float v = 1.0 / (num_increments - 1) * j;

				line_vertices[j] = BezierPatch::patch_point(MGM_x, MGM_y, MGM_z, u, v);
				line_vertices[num_increments+j] = BezierPatch::patch_point(MGM_x, MGM_y, MGM_z, v, u);
			}
			glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);
			glDrawArrays(GL_LINE_STRIP, 0, num_increments);
			glDrawArrays(GL_LINE_STRIP, num_increments, num_increments*2);
		}
	}
};


class BezierPatchCollection {
	std::vector<BezierPatch> patches;
public:
	BezierPatchCollection() {}
	void add_patch(BezierPatch patch) {
		patches.push_back(patch);
	}
	void draw() {
		for (int i = 0; i < patches.size(); i++) {
			patches[i].draw();
		}
	}
	void draw_cps() {
		glPointSize(10.0f);
		std::vector<point4> to_draw;
		for (int i = 0; i < loaded_points.size(); i++)
			to_draw.push_back(point4(loaded_points[i], 1.0));
		glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(point4), &to_draw[0], GL_STATIC_DRAW);
		//glDrawArrays(GL_POINTS, 0, 1);
	}
};
BezierPatchCollection patches;

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
   glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);


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







   std::string filename_str = "teapot";
   char *filename = &filename_str[0u];
   int num_patches = 0;
   int num_points = 0;
   load_patch(filename, &num_patches, &num_points);

   patch_indices = patch_indices;
   loaded_points = loaded_points;

   std::vector<point3> all_points;
   for (int i = 0; i < patch_indices.size(); i++) {
	   std::vector<int> cp_idxs = patch_indices[i].cp_idxs;
	   for (int j = 0; j < 16; j++)
		   all_points.push_back(loaded_points[cp_idxs[j]-1]);
   }


   for (int i = 0; i < all_points.size() / 16; i++)
	   patches.add_patch(BezierPatch(&all_points[16*i]));


   patches.draw();


}




//----------------------------------------------------------------------------








void
display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//  Generate the model-view matrix
	const glm::vec3 viewer_pos(0.0, 0.0, 0.0);
	glm::mat4 trans, rot, scale, model_view;
	rot = glm::rotate(rot, glm::radians(Theta[Xaxis]), glm::vec3(1, 0, 0));
	rot = glm::rotate(rot, glm::radians(Theta[Yaxis]), glm::vec3(0, 1, 0));
	rot = glm::rotate(rot, glm::radians(Theta[Zaxis]), glm::vec3(0, 0, 1));
	trans = glm::translate(trans, -viewer_pos);
	scale = glm::scale(scale, glm::vec3(0.3, 0.3, 0.3));
	model_view = trans * rot * scale;
	glUniformMatrix4fv(ModelView, 1, GL_FALSE, glm::value_ptr(model_view));


	//glPointSize(10.0f);
	//glDrawArrays(GL_POINTS, 0, 16);



	
	patches.draw();
	//patches.draw_cps();
	//line_vertices = line_vertices;
	//glDrawArrays(GL_LINE_STRIP, 0, num_increments);
	//glDrawArrays(GL_LINE_STRIP, num_increments, num_increments * 2);


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
mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN) {
		switch (button) {
		case GLUT_LEFT_BUTTON:    Axis = Xaxis;  break;
		case GLUT_MIDDLE_BUTTON:  Axis = Yaxis;  break;
		case GLUT_RIGHT_BUTTON:   Axis = Zaxis;  break;
		}
	}
}

//----------------------------------------------------------------------------

void
update(void)
{
	Theta[Axis] += 0.5;

	if (Theta[Axis] > 360.0) {
		Theta[Axis] -= 360.0;
	}
}


//----------------------------------------------------------------------------

void
reshape( int w, int h )
{
   glViewport( 0, 0, w, h );

   GLfloat aspect = GLfloat(w)/h;
   glm::mat4 projection;

   glUniformMatrix4fv( Projection, 1, GL_FALSE, glm::value_ptr(projection) );
}
