#include "bitshift.h"
using namespace ffglex;
using namespace ffglqs;

static CFFGLPluginInfo PluginInfo(
	PluginFactory< BitShift >,// Create method
	"RE01777",                      // Plugin unique ID of maximum length 4.
	"BitShift Example",            // Plugin name
	2,                           // API major version number
	1,                           // API minor version number
	1,                           // Plugin major version number
	0,                           // Plugin minor version number
	FF_EFFECT,                   // Plugin type
	"bitshift, but fast now",  // Plugin description
	"mostly the Resolume FFGL Example"      // About
);

static const char _vertexShaderCode[] = R"(#version 410 core
uniform vec2 MaxUV;

layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec2 vUV;

out vec2 uv;

void main()
{
	gl_Position = vPosition;
	uv = vUV * MaxUV;
}
)";

static const char _fragmentShaderCode[] = R"(#version 410 core
uniform sampler2D InputTexture;
uniform int Bits;

in vec2 uv;

out vec4 fragColor;

int rotl(int n, int bits) {
    bits = bits & 0xFF;
    int result  = n << bits;
    int temp2 = result & 0xFF00;
    temp2 = temp2 >> 8;
    result |= temp2;
    return result & 0xFF;
}

void main()
{
	vec4 color = texture( InputTexture, uv );
	//The InputTexture contains premultiplied colors, so we need to unpremultiply first to apply our effect on straight colors.
	if( color.a > 0.0 )
		color.rgb /= color.a;

	//color.rgb += Brightness * 2. - 1.;

    // ------------ BEGIN JOEL CODE ------------
    float rf = color.rgb.r * 255;
    float gf = color.rgb.g * 255;
    float bf = color.rgb.b * 255;
    int r = int(rf);
    int g = int(gf);
    int b = int(bf);
    r = rotl(r, Bits);
    g = rotl(g, Bits);    
    b = rotl(b, Bits);
    rf = float(r) / 255.0;
    gf = float(g) / 255.0;
    bf = float(b) / 255.0;
    color.rgb.r = rf;
    color.rgb.g = gf;
    color.rgb.b = bf;
    
    // ------------ END JOEL CODE ------------


	//The plugin has to output premultiplied colors, this is how we're premultiplying our straight color while also
	//ensuring we aren't going out of the LDR the video engine is working in.
	color.rgb = clamp( color.rgb * color.a, vec3( 0.0 ), vec3( color.a ) );
	fragColor = color;
}
)";

BitShift::BitShift()
{
	// Input properties
	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	//We declare that this plugin has a Brightness parameter which is a RGB param.
	//The name here must match the one you declared in your fragment shader.
	//AddRGBColorParam( "Brightness" );
    AddParam(ParamRange::CreateInteger("Bits", 3, {0,7}) );
    

	FFGLLog::LogToHost( "Created BitShift effect" );
}
BitShift::~BitShift()
{
}

FFResult BitShift::InitGL( const FFGLViewportStruct* vp )
{
	if( !shader.Compile( _vertexShaderCode, _fragmentShaderCode ) )
	{
        FFGLLog::LogToHost( "##### failed to compile!" );
		DeInitGL();
		return FF_FAIL;
	}
	if( !quad.Initialise() )
	{
		DeInitGL();
		return FF_FAIL;
	}

	//Use base-class init as success result so that it retains the viewport.
	return CFFGLPlugin::InitGL( vp );
}
FFResult BitShift::ProcessOpenGL( ProcessOpenGLStruct* pGL )
{
	if( pGL->numInputTextures < 1 )
		return FF_FAIL;

	if( pGL->inputTextures[ 0 ] == NULL )
		return FF_FAIL;

	//FFGL requires us to leave the context in a default state on return, so use this scoped binding to help us do that.
	ScopedShaderBinding shaderBinding( shader.GetGLID() );
	//The shader's sampler is always bound to sampler index 0 so that's where we need to bind the texture.
	//Again, we're using the scoped bindings to help us keep the context in a default state.
	ScopedSamplerActivation activateSampler( 0 );
	Scoped2DTextureBinding textureBinding( pGL->inputTextures[ 0 ]->Handle );

	shader.Set( "inputTexture", 0 );

	//The input texture's dimension might change each frame and so might the content area.
	//We're adopting the texture's maxUV using a uniform because that way we dont have to update our vertex buffer each frame.
	FFGLTexCoords maxCoords = GetMaxGLTexCoords( *pGL->inputTextures[ 0 ] );
	shader.Set( "MaxUV", maxCoords.s, maxCoords.t );

	//This takes care of sending all the parameter that the plugin registered to the shader.
	SendParams( shader );

	quad.Draw();
    
    Update();
    
	return FF_SUCCESS;
}

void BitShift::Update()
{
    auto bitsParam = std::dynamic_pointer_cast< ParamRange >( GetParam( "Bits" ) );
    int bits     = bitsParam->GetValue();
    shader.Set( "Bits", bits );
}

FFResult BitShift::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();

	return FF_SUCCESS;
}
