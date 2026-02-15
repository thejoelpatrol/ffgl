#pragma once
#include <string>
#include <FFGLSDK.h>

class QuadMirror : public ffglqs::Plugin
{
public:
    QuadMirror();
	~QuadMirror();

	//CFFGLPlugin
	FFResult InitGL( const FFGLViewportStruct* vp ) override;
	FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
	FFResult DeInitGL() override;

private:
	ffglex::FFGLShader shader;  //!< Utility to help us compile and link some shaders into a program.
	ffglex::FFGLScreenQuad quad;//!< Utility to help us render a full screen quad.
};
