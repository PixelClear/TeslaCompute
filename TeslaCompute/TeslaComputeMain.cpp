#include "TeslaComputeMain.h"
#include <iostream>

Magick::Image m_image;
Magick::Blob m_blob;


GLuint render_vbo;
GLuint render_vao;
GLuint render_obj_vs;
GLuint render_obj_fs;
GLuint render_prog;
GLuint tex_buffer;

GLuint IBO;
GLint texture_location;

bool key_status = false;

 GLfloat vertexData[] = {
        1.0f,  -1.0f,  0.0f,
        1.0f, 1.0f, 0.0f ,
		-1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f
    }; // 4 ve

 GLfloat texData[] = {
        1.0f,  0.0f,
        1.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 1.0f
    }; // 

 static const GLushort vertex_indices[] =
{
    0,1,2,2,3,1
};

 const char* vertex_source[] ={
	    "#version 330\n"
        "layout (location = 0) in vec3 vposition;\n"
		"layout (location = 1) in vec2 TexCoord;\n"
		"out vec2 TexCoord0;\n"
		 "void main() {\n"
		 "TexCoord0     = TexCoord;\n"
		 "gl_Position = vec4(vposition,1.0);\n"
		"}\n"};
        
   const char* fragment_source[] ={
	   "#version 330\n"
	   "in vec2 TexCoord0;\n"
	   "uniform sampler2D tex;\n"
        "void main() {\n"
		"   gl_FragColor =texture(tex,TexCoord0);\n"
		"}\n"};

static int dispatch_once = 0;

void PrepareComputeTextures()
{
	glGenTextures(1,&input_image);
	glBindTexture(GL_TEXTURE_2D, input_image);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F, m_image.columns(), m_image.rows(), 0,GL_RGBA, GL_UNSIGNED_BYTE, m_blob.data());
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   

	glGenTextures(1, &intermediate_image);
	glBindTexture(GL_TEXTURE_2D, intermediate_image);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F, m_image.columns(), m_image.rows(), 0,GL_RGBA, GL_FLOAT, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   
	
	glGenTextures(1, &out_put_image);
	glBindTexture(GL_TEXTURE_2D, out_put_image);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F, m_image.columns(), m_image.rows(), 0,GL_RGBA, GL_FLOAT, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   
	
	glBindTexture(GL_TEXTURE_2D, 0);
}

bool PrepareComputeShader()
{
	compute_shader_obj = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(compute_shader_obj,1,compute_source_edge_filter,NULL);
	glCompileShader(compute_shader_obj);

	GLint success;
	glGetShaderiv(compute_shader_obj, GL_COMPILE_STATUS, &success);

	if (!success) {
		char InfoLog[1024];
		glGetShaderInfoLog(compute_shader_obj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling : '%s'\n", InfoLog);
		return false;
	}

	compute_shader_prog = glCreateProgram();
	glAttachShader(compute_shader_prog,compute_shader_obj);
	glLinkProgram(compute_shader_prog);

	glGetShaderiv(compute_shader_prog, GL_LINK_STATUS, &success);

	if (!success) {
		char InfoLog[1024];
		glGetShaderInfoLog(compute_shader_prog, 1024, NULL, InfoLog);
		fprintf(stderr, "Error linking : '%s'\n", InfoLog);
		return false;
	}

	glDetachShader(compute_shader_prog,compute_shader_obj);

	glUniform1i(glGetUniformLocation(compute_shader_prog, "input_image"), 0);
	glUniform1i(glGetUniformLocation(compute_shader_prog, "output_image"), 1);
	return true;
}

bool LoadImage_l()
{
	try {
		m_image.read("test.jpg");
		m_image.write(&m_blob, "RGBA");
	}
	catch (Magick::Error& Error) {
		std::cout << "Error loading texture Test.jpg" << "': " << Error.what() << std::endl;
		return false;
	}

	return true;
}


void PrepareVBO()
{
	glGenVertexArrays(1, &render_vao);
	glBindVertexArray(render_vao);
	
	glGenBuffers(1,&IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertex_indices), vertex_indices,GL_STATIC_DRAW);

	glGenBuffers(1, &render_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, render_vbo);
	glBufferData(GL_ARRAY_BUFFER,sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	glGenBuffers(1,&tex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buffer);
	glBufferData(GL_ARRAY_BUFFER,sizeof(texData), texData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,2,GL_FLOAT, GL_FALSE, 0, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}



bool PrepareShaders()
{
	GLint success;
	
	render_obj_vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(render_obj_vs,1,vertex_source,NULL);
	glCompileShader(render_obj_vs);

	
	glGetShaderiv(render_obj_vs, GL_COMPILE_STATUS, &success);

	if (!success) {
		char InfoLog[1024];
		glGetShaderInfoLog(render_obj_vs, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling render vs: '%s'\n", InfoLog);
		return false;
	}

	render_obj_fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(render_obj_fs,1,fragment_source,NULL);
	glCompileShader(render_obj_fs);

	
	glGetShaderiv(render_obj_fs, GL_COMPILE_STATUS, &success);

	if (!success) {
		char InfoLog[1024];
		glGetShaderInfoLog(render_obj_fs, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling : '%s'\n", InfoLog);
		return false;
	}

	render_prog = glCreateProgram();
	glAttachShader(render_prog,render_obj_vs);
	glAttachShader(render_prog,render_obj_fs);
	glLinkProgram(render_prog);

	texture_location = glGetUniformLocation(render_prog, "tex");
	glUniform1i(texture_location, 1);

	glGetShaderiv(render_prog, GL_LINK_STATUS, &success);

	if (!success) {
		char InfoLog[1024];
		glGetShaderInfoLog(render_prog, 1024, NULL, InfoLog);
		fprintf(stderr, "Error linking : '%s'\n", InfoLog);
		return false;
	}

	glDetachShader(render_prog, render_obj_vs);
	glDetachShader(render_prog,render_obj_fs);

	return true;
}

void handleResize(int w, int h) {
	
}


void KeyBoard(unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'e':
		   if(key_status == true)
		   {
			   key_status = false;
		   }
		   else
		   {
			   key_status = true;
			   dispatch_once = 0;
		   }
		break;
	}
}


void Clear()
{

	glDeleteBuffers(1, &render_vbo);
	glDeleteVertexArrays(1, &render_vao);
	glDeleteShader(render_obj_vs);
	glDeleteShader(render_obj_fs);
	glDeleteProgram(render_prog);
	glDeleteBuffers(1, &tex_buffer);
	glDeleteShader(compute_shader_obj);
	glDeleteProgram(compute_shader_prog);
	glDeleteTextures(1, &input_image);
	glDeleteTextures(1, &intermediate_image);
	glDeleteTextures(1, &out_put_image);
}

void DisplayMainWindow(void)
{
   //Trying to avoid continous spawn of workgroups
   
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glEnable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);

   if(key_status == false)
   {
	   glUseProgram(render_prog);
	   glBindVertexArray(render_vao);
	   glActiveTexture(GL_TEXTURE0);
	   glBindTexture(GL_TEXTURE_2D, input_image);

	   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	   glutSwapBuffers();   
	   	   
	   glBindTexture(GL_TEXTURE_2D, 0);
	   glBindVertexArray(0);
	   glUseProgram(0);

   }

   if(key_status == true && dispatch_once == 0)
   {
	   
	   glUseProgram(compute_shader_prog);
	   glBindImageTexture(0, input_image, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	   glBindImageTexture(1, intermediate_image, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	   //Dispatch compute shaders in Y direction and write it as x component
	   glDispatchCompute(1, 1024, 1);

	   //As work groups are reading pixels in shared memory we need them to come to common point before we
	   //Actually apply edge filter
	   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	   glBindImageTexture(0, intermediate_image, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	   glBindImageTexture(1, out_put_image, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	   //Again dispatch compute shader in Y direction and write it again as x component to get originalS image
	   glDispatchCompute(1, 1024, 1);

	   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	   glUseProgram(0);

	   // Clear, select the rendering program and draw a full screen quad
	   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	   glUseProgram(render_prog);
	   glBindVertexArray(render_vao);
	   glActiveTexture(GL_TEXTURE0);
	   glBindTexture(GL_TEXTURE_2D, out_put_image);

	   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	   glutSwapBuffers();   
	   glFinish();
	   
	   glBindTexture(GL_TEXTURE_2D, 0);
	   glBindVertexArray(0);
	   glUseProgram(0);
	   dispatch_once = 1;
   }

   
   glutPostRedisplay();

}

int main(int argc, char *argv[])
{	
    int mainWinID;

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1350, 700);
    glutCreateWindow("TeslaCompute 1.0");
    glutDisplayFunc(DisplayMainWindow);
	glutReshapeFunc(handleResize);
	glutKeyboardFunc(KeyBoard);

	glewInit();
	glClearColor(0.0,0.0,0.0,1.0);
	LoadImage_l();
	PrepareShaders();
	PrepareVBO();
	PrepareComputeTextures();
	PrepareComputeShader();

    glutMainLoop();

	Clear();
    return 0;
}

