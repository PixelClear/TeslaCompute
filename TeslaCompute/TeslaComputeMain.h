#ifndef TESLA_COMPUTE_H
#define TESLA_COMPUTE_H


#include <AntTweakBar.h>
#include <ImageMagick-6\Magick++.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/GLU.h>
#include <GL/freeglut.h>

GLuint compute_shader_obj;
GLuint compute_shader_prog;

GLuint input_image;
GLuint intermediate_image;
GLuint out_put_image;

static const GLchar * compute_source_edge_filter[] =
{
    "#version 430 core\n"
	"layout (local_size_x = 1024) in;\n"
	"layout (rgba32f, binding = 0) uniform image2D input_image;\n"
	"layout (rgba32f, binding = 1) uniform image2D output_image;\n"
	"shared vec4 scanline[1024];\n"
	"void main(void)\n"
	"{\n"
	"ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
	"scanline[pos.x] = imageLoad(input_image, pos);\n"
	"barrier();\n"
    "vec4 result = scanline[min(pos.x + 1, 1023)] -\n"
    "scanline[max(pos.x - 1, 0)];"
    "imageStore(output_image, pos.yx, result);"
    "}"
};



#endif