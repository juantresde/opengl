/**
 * As per: http://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_02
 */

#include "shader_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

static char *file_read (const char *filename)
{
  FILE *in = fopen (filename, "rb");
  if (in == NULL) return NULL;

  int res_size = BUFSIZ;
  char *res = (char *)malloc (res_size);
  int nb_read_total = 0;
  while (!feof (in) && !ferror (in))
    {
      if (nb_read_total + BUFSIZ > res_size) 
	{
	  if (res_size > 10*1024*1024)
	    break;
	  res_size = res_size * 2;
	  res = (char *)realloc (res, res_size);
	}
      char *p_res = res + nb_read_total;
      nb_read_total += fread (p_res, 1, BUFSIZ, in);
    }
  fclose (in);
  res = (char*)realloc (res, nb_read_total + 1);
  res[nb_read_total] = '\0';
  return res;
}

/**
 * Display compilation errors from the OpenGL shader compiler
 */
void print_log (GLuint object)
{
  GLint log_length = 0;
  if (glIsShader (object))
    glGetShaderiv (object, GL_INFO_LOG_LENGTH, &log_length);
  else if (glIsProgram (object))
    glGetProgramiv (object, GL_INFO_LOG_LENGTH, &log_length);
  else {
    cerr << "PRINTLOG: Not a Shader or a Program" << endl;
    return;
  }

  char *log = (char*)malloc (log_length);
  if (glIsShader (object))
    glGetShaderInfoLog (object, log_length, NULL, log);
  else if (glIsProgram (object))
    glGetProgramInfoLog (object, log_length, NULL, log);
  cerr << log << endl;
  free (log);
}

/**
 * Compile the shader from 'filename', with error handeling
 */
GLuint create_shader (const char *filename, GLenum type)
{
  const GLchar *source = file_read (filename);
  if (source == NULL)
    {
      fprintf (stderr, "Error Opening %s: ", filename);
      perror ("");
      exit (EXIT_FAILURE);
    }
  GLuint res = glCreateShader (type);
  const GLchar *sources[3] = {
    #ifdef OPENGL_ES
    "#version 100\n"
    #else
    "#version 330\n"
    #endif
    ,
    /* GLES2 precision specifiers */
    #ifdef GL_ES_VERSION_2_0
    /*  Define default float precision for fragment shaders: */
    (type == GL_FRAGMENT_SHADER) ?
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
    "precision highp float;           \n"
    "#else                            \n"
    "precision mediump float;         \n"
    "#endif                           \n"
    : ""
    /*  Note: OpengGL ES automatically defines this:
	#define GL_ES
    */
    #else
    /* Ignore GLES 2 precision specifiers: */
    "#define lowp    \n"
    "#define mediump \n"
    "#define highp   \n"
    #endif
    ,
    source };
  glShaderSource (res, 3, sources, NULL);
  free ((void*)source);
  glCompileShader (res);
  GLint compile_ok = GL_FALSE;
  glGetShaderiv (res, GL_COMPILE_STATUS, &compile_ok);
  if (compile_ok == GL_FALSE)
    {
      fprintf (stderr, "%s:", filename);
      print_log (res);
      glDeleteShader (res);
      exit (EXIT_FAILURE);
    }
  return res;
}