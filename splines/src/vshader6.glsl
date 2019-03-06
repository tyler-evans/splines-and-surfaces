#version 150

in vec4 vPosition;
uniform mat4 ModelView;

void main()
{
    gl_Position = ModelView * vPosition;
}

