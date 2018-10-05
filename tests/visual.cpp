#include <fstream>
#include <exception>
#include <iostream>
#include <memory>

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>

#include <text-gl/text.h>

using namespace TextGL;
using namespace glm;


#define CHECK_GL() { GLenum err = glGetError(); if (err != GL_NO_ERROR) throw GLError(err, __FILE__, __LINE__); }


#define VERTEX_POSITION_INDEX 0
#define VERTEX_TEXCOORDS_INDEX 1


class MessageError: public std::exception
{
    protected:
        std::string message;
    public:
        const char *what(void) const noexcept
        {
            return message.c_str();
        }
};

class ShaderError: public MessageError
{
    public:
        ShaderError(const boost::format &fmt)
        {
            message = fmt.str();
        }
};

class InitError: public MessageError
{
    public:
        InitError(const boost::format &fmt)
        {
            message = fmt.str();
        }
        InitError(const std::string &msg)
        {
            message = msg;
        }
};

class RenderError: public MessageError
{
    public:
        RenderError(const std::string & msg)
        {
            message = msg;
        }
};


GLuint CreateShader(const std::string &source, GLenum type)
{
    GLint result;
    int logLength;

    GLuint shader = glCreateShader(type);
    CHECK_GL();

    const char *pSource = source.c_str();
    glShaderSource(shader, 1, &pSource, NULL);
    CHECK_GL();

    glCompileShader(shader);
    CHECK_GL();

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    CHECK_GL();

    if (result != GL_TRUE)
    {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        CHECK_GL();

        char *errorString = new char[logLength + 1];
        glGetShaderInfoLog(shader, logLength, NULL, errorString);
        CHECK_GL();

        ShaderError error(boost::format("error while compiling shader: %1%") % errorString);
        delete[] errorString;

        glDeleteShader(shader);
        CHECK_GL();

        throw error;
    }

    return shader;
}

GLuint LinkShaderProgram(const GLuint vertexShader,
                         const GLuint fragmentShader,
                         const std::map<size_t, std::string> &vertexAttribLocations)
{
    GLint result;
    int logLength;
    GLuint program;

    program = glCreateProgram();
    CHECK_GL();

    glAttachShader(program, vertexShader);
    CHECK_GL();

    glAttachShader(program, fragmentShader);
    CHECK_GL();

    for (const auto &pair : vertexAttribLocations)
    {
        glBindAttribLocation(program, std::get<0>(pair),
                                      std::get<1>(pair).c_str());
        CHECK_GL();
    }

    glLinkProgram(program);
    CHECK_GL();

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    CHECK_GL();

    if (result != GL_TRUE)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        CHECK_GL();

        char *errorString = new char[logLength + 1];
        glGetProgramInfoLog(program, logLength, NULL, errorString);
        CHECK_GL();

        ShaderError error(boost::format("error while linking shader: %1%") % errorString);
        delete[] errorString;

        glDeleteProgram(program);
        CHECK_GL();

        throw error;
    }

    return program;
}

const char glyphVertexShaderSrc[] = R"shader(
#version 150

in vec2 position;
in vec2 texCoords;

out VertexData
{
    vec2 texCoords;
} vertexOut;

uniform mat4 projectionMatrix;

void main()
{
    gl_Position = projectionMatrix * vec4(position, 0.0, 1.0);
    vertexOut.texCoords = texCoords;
}
)shader",
         glyphFragmentShaderSrc[] = R"shader(
#version 150

uniform sampler2D tex;

in VertexData
{
    vec2 texCoords;
} vertexIn;

out vec4 fragColor;

void main()
{
    fragColor = texture(tex, vertexIn.texCoords);
}

)shader",
    selectionVertexShaderSrc[] = R"shader(
#version 150

uniform mat4 projectionMatrix;

in vec2 position;

void main()
{
    gl_Position = projectionMatrix * vec4(position, 0.0, 1.0);
}
)shader",
    selectionFragmentShaderSrc[] = R"shader(
#version 150

out vec4 fragColor;

void main()
{
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)shader";


class TextBeamTracer: public GLTextLeftToRightIterator
{
    private:
        vec3 beam[2];

        vec2 selectionQuad[4];

    protected:
        void OnGlyph(const UTF8Char c, const GlyphQuad &, const TextSelectionDetails &details)
        {
            GLfloat bottomY = details.baseY + details.descent,
                    topY = details.baseY + details.ascent;

             vec3 p0(details.startX, topY, 0.0f),
                  p1(details.endX, topY, 0.0f),
                  p2(details.endX, bottomY, 0.0f),
                  p3(details.startX, bottomY, 0.0f),
                  p;

            if (intersectLineTriangle(beam[0], beam[1] - beam[0], p0, p1, p2, p)
                || intersectLineTriangle(beam[0], beam[1] - beam[0], p0, p2, p3, p))
            {
                selectionQuad[0].x = p0.x;
                selectionQuad[0].y = p0.y;
                selectionQuad[1].x = p1.x;
                selectionQuad[1].y = p1.y;
                selectionQuad[2].x = p2.x;
                selectionQuad[2].y = p2.y;
                selectionQuad[3].x = p3.x;
                selectionQuad[3].y = p3.y;
            }
        }
    public:
        void SetBeam(const vec3 &p0, const vec3 &p1)
        {
            beam[0] = p0;
            beam[1] = p1;
        }
        const vec2 *GetQuad(void)
        {
            return selectionQuad;
        }
};


class TextRenderer: public GLTextLeftToRightIterator
{
    private:
        GLuint vboID,
               shaderProgram;
        mat4 projection;
    protected:
        void OnGlyph(const UTF8Char c, const GlyphQuad &quad, const TextSelectionDetails &details)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vboID);
            CHECK_GL();

            glEnableVertexAttribArray(VERTEX_POSITION_INDEX);
            CHECK_GL();
            glEnableVertexAttribArray(VERTEX_TEXCOORDS_INDEX);
            CHECK_GL();
            glVertexAttribPointer(VERTEX_POSITION_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphVertex), 0);
            CHECK_GL();
            glVertexAttribPointer(VERTEX_TEXCOORDS_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphVertex), (GLvoid *)(2 * sizeof(GLfloat)));
            CHECK_GL();

            // Fill the buffer.

            GlyphVertex *pVertexBuffer = (GlyphVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            CHECK_GL();

            pVertexBuffer[0] = quad.vertices[0];
            pVertexBuffer[1] = quad.vertices[1];
            pVertexBuffer[2] = quad.vertices[3];
            pVertexBuffer[3] = quad.vertices[2];

            glUnmapBuffer(GL_ARRAY_BUFFER);
            CHECK_GL();

            // Draw the buffer.

            glUseProgram(shaderProgram);
            CHECK_GL();

            GLint location = glGetUniformLocation(shaderProgram, "projectionMatrix");
            if (location < 0)
                throw RenderError("projection matrix location not found");

            glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(projection));
            CHECK_GL();

            glActiveTexture(GL_TEXTURE0);
            CHECK_GL();

            glBindTexture(GL_TEXTURE_2D, quad.texture);
            CHECK_GL();

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            CHECK_GL();

            glDisableVertexAttribArray(VERTEX_POSITION_INDEX);
            CHECK_GL();
            glDisableVertexAttribArray(VERTEX_TEXCOORDS_INDEX);
            CHECK_GL();

            glBindBuffer(GL_ARRAY_BUFFER, NULL);
            CHECK_GL();
        }
    public:
        void SetProjection(const mat4 &prj)
        {
            projection = prj;
        }

        void InitGL(void)
        {
            glGenBuffers(1, &vboID);
            CHECK_GL();

            if (vboID == NULL)
                throw InitError("No vertex buffer was generated");

            glBindBuffer(GL_ARRAY_BUFFER, vboID);
            CHECK_GL();

            glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GlyphVertex), NULL, GL_DYNAMIC_DRAW);
            CHECK_GL();

            glBindBuffer(GL_ARRAY_BUFFER, NULL);
            CHECK_GL();

            GLuint vertexShader = CreateShader(glyphVertexShaderSrc, GL_VERTEX_SHADER),
                   fragmentShader = CreateShader(glyphFragmentShaderSrc, GL_FRAGMENT_SHADER);

            std::map<size_t, std::string> vertexAttribLocations;
            vertexAttribLocations[VERTEX_POSITION_INDEX] = "position";
            vertexAttribLocations[VERTEX_TEXCOORDS_INDEX] = "texCoords";

            shaderProgram = LinkShaderProgram(vertexShader, fragmentShader, vertexAttribLocations);

            glDeleteShader(vertexShader);
            CHECK_GL();

            glDeleteShader(fragmentShader);
            CHECK_GL();
        }
        void FreeGL(void)
        {
            glDeleteProgram(shaderProgram);
            CHECK_GL();

            glDeleteBuffers(1, &vboID);
            CHECK_GL();
        }
};

typedef vec2 SelectionVertex;

class SelectionRenderer
{
    private:
        GLuint vboID,
               shaderProgram;
        mat4 projection;
    public:
        void InitGL(void)
        {
            glGenBuffers(1, &vboID);
            CHECK_GL();

            if (vboID == NULL)
                throw InitError("No vertex buffer was generated");

            glBindBuffer(GL_ARRAY_BUFFER, vboID);
            CHECK_GL();

            glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(SelectionVertex), NULL, GL_DYNAMIC_DRAW);
            CHECK_GL();

            glBindBuffer(GL_ARRAY_BUFFER, NULL);
            CHECK_GL();

            GLuint vertexShader = CreateShader(selectionVertexShaderSrc, GL_VERTEX_SHADER),
                   fragmentShader = CreateShader(selectionFragmentShaderSrc, GL_FRAGMENT_SHADER);

            std::map<size_t, std::string> vertexAttribLocations;
            vertexAttribLocations[VERTEX_POSITION_INDEX] = "position";

            shaderProgram = LinkShaderProgram(vertexShader, fragmentShader, vertexAttribLocations);

            glDeleteShader(vertexShader);
            CHECK_GL();

            glDeleteShader(fragmentShader);
            CHECK_GL();
        }
        void FreeGL(void)
        {
            glDeleteBuffers(1, &vboID);
            CHECK_GL();

            glDeleteProgram(shaderProgram);
            CHECK_GL();
        }
        void Render(const SelectionVertex *quad, const mat4 &projection)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vboID);
            CHECK_GL();

            glEnableVertexAttribArray(VERTEX_POSITION_INDEX);
            CHECK_GL();
            glVertexAttribPointer(VERTEX_POSITION_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(SelectionVertex), 0);
            CHECK_GL();

            // Fill the buffer.

            SelectionVertex *pVertexBuffer = (SelectionVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            CHECK_GL();

            pVertexBuffer[0] = quad[0];
            pVertexBuffer[1] = quad[1];
            pVertexBuffer[2] = quad[2];
            pVertexBuffer[3] = quad[3];

            glUnmapBuffer(GL_ARRAY_BUFFER);
            CHECK_GL();

            // Draw the buffer.

            glUseProgram(shaderProgram);
            CHECK_GL();

            GLint location = glGetUniformLocation(shaderProgram, "projectionMatrix");
            if (location < 0)
                throw RenderError("projection matrix location not found");

            glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(projection));
            CHECK_GL();

            glDrawArrays(GL_LINE_LOOP, 0, 4);
            CHECK_GL();

            glDisableVertexAttribArray(VERTEX_POSITION_INDEX);
            CHECK_GL();

            glBindBuffer(GL_ARRAY_BUFFER, NULL);
            CHECK_GL();
        }
};

const int8_t displayText[] = R"text(Once upon a time, there was a big man. He had very big hands and legs. He had giant eyes. However, the biggest was his chest. But his head was even bigger.

Upon a day, the big man went to the butcher. He asked: do you have eggplants? The butcher answered: "sorry, all out". And then the man became so unhappy that he cried himself to death...
And then he came back as a ghost, but he couldn't fly. So the ghost fell into the water and drowned. The end?!
)text";

class DemoApp
{
private:
    SDL_Window *mainWindow;
    SDL_GLContext mainGLContext;

    bool running;

    GLfloat angle;
    GLTextureFont *pFont;
    TextRenderer mTextRenderer;
    SelectionRenderer mSelectionRenderer;
    TextBeamTracer mBeamTracer;
    TextParams textParams;
public:
    void Init(const std::shared_ptr<ImageFont> pImageFont, const TextParams &params)
    {
        textParams = params;

        int error = SDL_Init(SDL_INIT_EVERYTHING);
        if (error != 0)
            throw InitError(boost::format("Unable to initialize SDL: %1%") % SDL_GetError());

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

        Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
        mainWindow = SDL_CreateWindow("Text Test",
                                      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                      800, 600, flags);
        if (!mainWindow)
            throw InitError(boost::format("SDL_CreateWindow failed: %1%") % SDL_GetError());

        mainGLContext = SDL_GL_CreateContext(mainWindow);
        if (!mainGLContext)
            throw InitError(boost::format("Failed to create a GL context: %1%") % SDL_GetError());

        GLenum err = glewInit();
        if (GLEW_OK != err)
            throw InitError(boost::format("glewInit failed: %1%") % glewGetErrorString(err));

        if (!GLEW_VERSION_3_2)
            throw InitError("OpenGL 3.2 is not supported");


        pFont = MakeGLTextureFont(pImageFont.get());

        mTextRenderer.InitGL();
        mSelectionRenderer.InitGL();
    }

    void Destroy(void)
    {
        mTextRenderer.FreeGL();
        mSelectionRenderer.FreeGL();

        DestroyGLTextureFont(pFont);

        SDL_GL_DeleteContext(mainGLContext);
        SDL_DestroyWindow(mainWindow);
        SDL_Quit();
    }

    void GetTextProjection(mat4 &matProject)
    {
        int screenWidth, screenHeight;
        SDL_GL_GetDrawableSize(mainWindow, &screenWidth, &screenHeight);

        matProject = perspective(pi<GLfloat>() / 4, GLfloat(screenWidth) / GLfloat(screenHeight), 0.1f, 2000.0f);
        matProject = translate(matProject, vec3(0.0f, 0.0, -1000.0f));
        matProject = rotate(matProject, angle, vec3(0.0f, 1.0f, 0.0f));
    }

    void OnMouseClick(const SDL_MouseButtonEvent &event)
    {
        GLfloat textX, textY;
        if (event.button == SDL_BUTTON_LEFT)
        {
            int screenWidth, screenHeight;
            SDL_GL_GetDrawableSize(mainWindow, &screenWidth, &screenHeight);

            vec4 viewPort;
            glGetFloatv(GL_VIEWPORT, value_ptr(viewPort));

            mat4 matProject;
            GetTextProjection(matProject);

            vec3 mouseWindowOrigin(event.x, screenHeight - event.y, 0.0f),
                 dir(0.0f, 0.0f, 10000.0f);

            vec3 mouseModelPosition0 = unProject(mouseWindowOrigin, mat4(), matProject, viewPort),
                 mouseModelPosition1 = unProject(mouseWindowOrigin + dir, mat4(), matProject, viewPort);

            mBeamTracer.SetBeam(mouseModelPosition0, mouseModelPosition1);
            mBeamTracer.IterateText(pFont, displayText, textParams);
        }
    }

    void HandleEvent(const SDL_Event &event)
    {
        if (event.type == SDL_QUIT)
            running = false;
        else if (event.type == SDL_MOUSEBUTTONDOWN)
            OnMouseClick(event.button);
    }

    void Render(void)
    {
        int screenWidth, screenHeight;
        SDL_GL_GetDrawableSize(mainWindow, &screenWidth, &screenHeight);

        glViewport(0, 0, screenWidth, screenHeight);

        mat4 matProject;
        GetTextProjection(matProject);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        CHECK_GL();

        glClear(GL_COLOR_BUFFER_BIT);
        CHECK_GL();

        glEnable(GL_BLEND);
        CHECK_GL();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        CHECK_GL();

        glDepthMask(GL_FALSE);
        CHECK_GL();

        // Render text.
        mTextRenderer.SetProjection(matProject);
        mTextRenderer.IterateText(pFont, displayText, textParams);

        // Render selection.
        mSelectionRenderer.Render(mBeamTracer.GetQuad(), matProject);
    }

    void RunDemo(const std::shared_ptr<ImageFont> pImageFont, const TextParams &params)
    {
        Init(pImageFont, params);

        boost::posix_time::ptime tStart = boost::posix_time::microsec_clock::local_time(),
                                 tNow;
        boost::posix_time::time_duration delta;
        running = true;
        while (running)
        {
            try
            {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    HandleEvent(event);
                }

                tNow = boost::posix_time::microsec_clock::local_time();
                delta = (tNow - tStart);
                angle = 0.3f * sin(float(delta.total_milliseconds()) / 1000);

                Render();

                SDL_GL_SwapWindow(mainWindow);
            }
            catch (...)
            {
                Destroy();
                std::rethrow_exception(std::current_exception());
            }
        }

        Destroy();
    }
};


int main(int argc, char **argv)
{
    FontStyle style;
    style.size = 32.0;
    style.strokeWidth = 2.0;
    style.fillColor = {1.0, 1.0, 1.0, 1.0};
    style.strokeColor = {0.0, 0.0, 0.0, 1.0};
    style.lineJoin = LINEJOIN_MITER;
    style.lineCap = LINECAP_SQUARE;

    TextParams params;
    params.startX = 0.0f;
    params.startY = 250.0f;
    params.maxWidth = 800.0f;
    params.lineSpacing = 40.0f;
    params.align = TEXTALIGN_CENTER;

    FontData fontData;
    std::shared_ptr<ImageFont> pImageFont = NULL;

    DemoApp app;

    std::ifstream is;

    if (argc < 2)
    {
        std::cerr << boost::format("Usage: %1% font_path") % argv[0] << std::endl;
        return 1;
    }

    try
    {
        is.open(argv[1]);
        if (!is.good())
        {
            std::cerr << "Error opening " << argv[1] << std::endl;
            return 1;
        }

        ParseSVGFontData(is, fontData);

        is.close();

        pImageFont = std::shared_ptr<ImageFont>(MakeImageFont(fontData, style), DestroyImageFont);

        // Put the text in the middle.
        params.startY = 0.0f + (params.lineSpacing * CountLines(pImageFont.get(), displayText, params)) / 2;

        app.RunDemo(pImageFont, params);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
