#include "QuadMirror.h"
using namespace ffglex;
using namespace ffglqs;

static CFFGLPluginInfo PluginInfo(
    PluginFactory< QuadMirror >,// Create method
    "RE024",                      // Plugin unique ID of maximum length 4.
    "QuadMirror",            // Plugin name
    2,                           // API major version number
    1,                           // API minor version number
    1,                           // Plugin major version number
    0,                           // Plugin minor version number
    FF_EFFECT,                   // Plugin type
    "QuadMirror, but fast now",  // Plugin description
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
uniform vec2 MaxUV;
uniform vec2 actual_dimensions;

in vec2 uv;

out vec4 fragColor;

ivec3 get_color(float x, float y) 
{
    vec4 color = texture(InputTexture, vec2(x, y));
    if (color.a > 0.0 )
        color.rgb /= color.a;
    
    ivec3 int_color;
    int_color[0] = int(color.rgb.r * 255.0);
    int_color[1] = int(color.rgb.g * 255.0);
    int_color[2] = int(color.rgb.b * 255.0);
    //int_color[3] = int(color.a * 255.0);

    return int_color;
}

void main()
{
    vec4 color = texture( InputTexture, uv );
    //The InputTexture contains premultiplied colors, so we need to unpremultiply first to apply our effect on straight colors.
    if( color.a > 0.0 )
        color.rgb /= color.a;


    // ------------ BEGIN JOEL CODE ------------
    float rf = color.rgb.r * 255;
    float gf = color.rgb.g * 255;
    float bf = color.rgb.b * 255;
    int r = int(rf);
    int g = int(gf);
    int b = int(bf);
    ivec3 rgb = ivec3(int(rf), int(gf), int(bf));

    float x = uv.x * actual_dimensions.x;
    float y = uv.y * actual_dimensions.y;
    
    float left_x = max(0, x - 1);
    float right_x = min(actual_dimensions.x - 1, x + 1);
    float bottom_y = max(0, y - 1);
    float top_y = min(actual_dimensions.y - 1, y + 1);
    
    ivec3 left_color = get_color(left_x / float(actual_dimensions.x), uv.y);
    ivec3 right_color = get_color(right_x / float(actual_dimensions.x), uv.y);
    ivec3 bottom_color = get_color(uv.x, bottom_y / float(actual_dimensions.y));
    ivec3 top_color = get_color(uv.x, top_y / float(actual_dimensions.y));

    //r = (r + left_color[0]) & 0xFF;
    
    //rgb = rgb + left_color + right_color + bottom_color + top_color;
    rgb += left_color + right_color + bottom_color + top_color;
    rgb &= ivec3(0xFF, 0xFF, 0xFF);
    
    //if (left_color[0] <= 100) {
    //    r = 255;
    //    g = 0;    
    //    b = 0;  
    //}

    //vec4 left_color = texture(InputTexture, vec2());
    //if (left_color.a > 0.0 )
    //    left_color.rgb /= left_color.a;
    //vec4 right_color = texture(InputTexture, vec2(right_x / float(actual_dimensions.x), y / float(actual_dimensions.y)));
    //if (right_color.a > 0.0 )
    //    right_color.rgb /= left_color.a;



    //color.rgb.r = left_color.rgb.r;
    //color.rgb.g = left_color.rgb.g;
    //color.rgb.b = left_color.rgb.b;
    


    //color.a = left_color.a;

    //color.rgb.r = 0;
    //color.rgb.g = 0;
    //color.rgb.b = 0;

    
    //if (left_x <= 50) {
    //    color.rgb.r = 1.0;
    //    color.rgb.g = 0;    
    //    color.rgb.b = 0;    
    //}
    //if (left_x > 1000) {
    //    color.rgb.g = 1.0;        
    //}
    //if (y <= 50) {
    //    color.rgb.b = 1.0;    
    //}

    //color.rgb.r =  float(right_x) / float(actual_dimensions.x);
    //color.rgb.g = 0;
    //color.rgb.b = 0;

    //
    //if (uv.x < 1) {
    //    color.rgb.r = 1.0;
    //    color.rgb.g = 0;
    //    color.rgb.b = 0;
    //}  
    //if (MaxUV[0] < 1) {
    //    color.rgb.b = 1.0;
    //}  



    rf = float(rgb.r) / 255.0;
    gf = float(rgb.g) / 255.0;
    bf = float(rgb.b) / 255.0;
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

QuadMirror::QuadMirror()
{
    // Input properties
    SetMinInputs( 1 );
    SetMaxInputs( 1 );


    FFGLLog::LogToHost( "Created RGBHSV effect" );
}
QuadMirror::~QuadMirror()
{
}

FFResult QuadMirror::InitGL( const FFGLViewportStruct* vp )
{
    if( !shader.Compile( _vertexShaderCode, _fragmentShaderCode ) )
    {
        FFGLLog::LogToHost( "##### failed to compile QuadMirror!" );
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
FFResult QuadMirror::ProcessOpenGL( ProcessOpenGLStruct* pGL )
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
    //char cstr[100];
    //sprintf(cstr, "##### actual max x: %d", currentViewport.width);
    //FFGLLog::LogToHost(cstr );
    shader.Set( "actual_dimensions", currentViewport.width, currentViewport.height );

    //This takes care of sending all the parameter that the plugin registered to the shader.
    SendParams( shader );

    quad.Draw();
    
    
    return FF_SUCCESS;
}


FFResult QuadMirror::DeInitGL()
{
    shader.FreeGLResources();
    quad.Release();

    return FF_SUCCESS;
}
