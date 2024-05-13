#include "framework.h"

/**
 * @brief Vertex shader in GLSL.
 *
 * @details This shader takes in vertex positions and UV coordinates, and outputs texture coordinates.
 * It also transforms the vertex positions to clipping space using the Model-View-Projection (MVP) matrix.
 */
const char *vertexSource = R"(
    #version 330
    precision highp float;

    uniform mat4 MVP;           ///< Model-View-Projection matrix in row-major format

    layout(location = 0) in vec2 vertexPosition;    ///< Attrib Array 0
    layout(location = 1) in vec2 vertexUV;          ///< Attrib Array 1

    out vec2 texCoord;                              ///< output attribute

    void main() {
        texCoord = vertexUV;                        ///< copy texture coordinates
        gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP;  ///< transform to clipping space
    }
)";

/**
 * @brief Fragment shader in GLSL.
 *
 * @details This shader takes in interpolated texture coordinates and fetches the corresponding color from the texture.
 */
const char *fragmentSource = R"(
    #version 330
    precision highp float;

    uniform sampler2D textureUnit; ///< Texture unit

    in vec2 texCoord;              ///< variable input: interpolated texture coordinates
    out vec4 fragmentColor;        ///< output that goes to the raster memory as told by glBindFragDataLocation

    void main() {
        fragmentColor = texture(textureUnit, texCoord); ///< fetch color from texture
    }
)";

/**
 * @class Camera2D
 *
 * @brief A 2D camera class.
 *
 * @details This class represents a 2D camera with a center and size in world coordinates.
 * It provides methods to get the view and projection matrices.
 */
class Camera2D {
    vec2 wCenter; ///< center in world coordinates
    vec2 wSize;   ///< width and height in world coordinates

public:
    /**
     * @brief Construct a new Camera2D object with default center and size.
     */
    Camera2D() : wCenter(20, 30), wSize(150, 150) {}

    /**
     * @brief Get the view matrix.
     *
     * @return mat4 The view matrix.
     */
    mat4 V() { return TranslateMatrix(-wCenter); }

    /**
     * @brief Get the projection matrix.
     *
     * @return mat4 The projection matrix.
     */
    mat4 P() const { return ScaleMatrix(vec2(2 / wSize.x, 2 / wSize.y)); }
};

Camera2D camera;       // 2D camera
GPUProgram gpuProgram; // vertex and fragment shaders

/**
 * @class PoincareTexture
 * @brief A class that extends the Texture class to create a Poincare texture.
 *
 * This class is used to create a Poincare texture by performing various mathematical operations.
 * It contains methods to increase the resolution of the texture, set the filtering mode, and render the texture.
 */
class PoincareTexture : public Texture {
private:
    int width, height; ///< The width and height of the texture.
    std::vector<vec3> points; ///< A vector of points.
    std::vector<vec3> centres; ///< A vector of centres.
    std::vector<vec3> hyperbolicPoints; ///< A vector of hyperbolic points.
    std::vector<vec2> poincarePoints; ///< A vector of Poincare points.
    std::vector<vec3> circles; ///< A vector of circles.

public:
    /**
     * @brief Constructor for the PoincareTexture class.
     * @param width The width of the texture.
     * @param height The height of the texture.
     */
    PoincareTexture(int width, int height) : width(width), height(height) {
        math();
        auto im = RenderToTexture(width, height);
        create(width, height, im);
    }

    /**
     * @brief Increases the resolution of the texture.
     * @param increaseBy The amount to increase the resolution by.
     */
    void increaseResolution(int increaseBy) {
        width += increaseBy;
        height += increaseBy;
        auto im = RenderToTexture(width, height);
        create(width, height, im);
    }

    /**
     * @brief Sets the filtering mode of the texture.
     * @param filteringMode The filtering mode to set.
     */
    void setFilteringMode(GLenum filteringMode) {
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(filteringMode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(filteringMode));
    }

    /**
     * @brief Calculates the iv value.
     * @param i The input value.
     * @return The calculated iv value.
     */
    static vec3 calculateIv(int i) {
        return {cosf(static_cast<float>((i * 40) * M_PI / 180)), sinf(static_cast<float>((i * 40) * M_PI / 180)), 0};
    }

    /**
     * @brief Calculates the ivv value.
     * @param iv The iv value.
     * @param dh The dh value.
     * @return The calculated ivv value.
     */
    static vec3 calculateIvv(const vec3& iv, float dh) {
        return vec3(0,0,1)* sinh(dh) + iv * cosh(dh);
    }

    /**
     * @brief Calculates the p value.
     * @param ivv The ivv value.
     * @param dh The dh value.
     * @return The calculated p value.
     */
    static vec3 calculateP(const vec3& ivv, float dh) {
        return vec3(0, 0, 1) * cosh(dh) + ivv * sinh(dh);
    }

    /**
     * @brief Calculates the h value.
     */
    void calcH(){
        int i = 0;
        while(i < 9){
            vec3 iv = calculateIv(i);
            float dh = 0.5;
            while(dh <= 5.5){
                vec3 ivv = calculateIvv(iv, dh);
                vec3 p = calculateP(ivv, dh);
                hyperbolicPoints.push_back(p);
                dh ++;
            }
            i++;
        }
    }

    /**
     * @brief Calculates the oszto value.
     * @param point The point value.
     * @return The calculated oszto value.
     */
    static float calculateOszto(const vec3& point) {
        return point.z + 1;
    }

    /**
     * @brief Adds a Poincare point.
     * @param point The point to add.
     * @param oszto The oszto value.
     */
    void addPP(const vec3& point, float oszto) {
        poincarePoints.emplace_back((point.x / oszto), (point.y / oszto));
    }

    /**
     * @brief Calculates the p value.
     */
    void calcP(){
        auto it = hyperbolicPoints.begin();
        while(it != hyperbolicPoints.end()) {
            float oszto = calculateOszto(*it);
            addPP(*it, oszto);
            ++it;
        }
    }

    /**
     * @brief Calculates the r value.
     * @param point The point value.
     * @return The calculated r value.
     */
    static float calculateR(const vec2& point) {
        return (1/(sqrtf(powf(point.x, 2)+powf(point.y, 2)))-(sqrtf(powf(point.x, 2)+powf(point.y, 2))))/2;
    }

    /**
     * @brief Calculates the c value.
     */
    void calcC(){
        auto it = poincarePoints.begin();
        while(it != poincarePoints.end()) {
            float r = calculateR(*it);
            vec2 i = normalize(*it);
            vec2 origo = *it+i*r;
            circles.emplace_back(origo.x, origo.y, r);
            ++it;
        }
    }
    /**
     * @brief Calculates the distance value.
     * @param point The point value.
     * @param circle The circle value.
     * @return The calculated distance value.
     */
    static float calculateDistance(const vec2& point, const vec3& circle) {
        return sqrtf(powf(point.x-circle.x, 2)+powf(point.y-circle.y, 2));
    }

    /**
     * @brief Counts the number of circles.
     * @param point The point value.
     * @return The number of circles.
     */
    int countCircles(const vec2& point){
        int count = 0;
        auto it = circles.begin();
        while(it != circles.end()) {
            if(calculateDistance(point, *it) <= it->z){
                count++;
            }
            ++it;
        }
        return count;
    }

    /**
     * @brief Returns the number of circles.
     * @param point The point value.
     * @return The number of circles.
     */
    int manyCircles(vec2 point){
        return countCircles(point);
    }

    /**
     * @brief Performs the math operations.
     */
    void math(){
        calcH();
        calcP();
        calcC();
    }

    /**
    * @brief Renders the texture.
    * @param textureWidth The width of the texture.
    * @param textureHeight The height of the texture.
    * @return The rendered texture.
    */
    std::vector<vec4> RenderToTexture(int textureWidth, int textureHeight) {
        std::vector<vec4> textureData;
        int yC = 0;
        while(yC < textureHeight) {
            int xC = 0;
            while(xC < textureWidth) {
                float x =(float) xC / (float)textureWidth * 2 - 1.0f;
                float y =(float) yC / (float)textureWidth * 2 - 1.0f;
                int count = manyCircles(vec2(x, y));
                if(sqrt(pow(x, 2) + pow(y, 2)) > 1) {
                    textureData.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
                }
                else if(count % 2 == 0) {
                    textureData.emplace_back(1.0f, 1.0f, 0.0f, 1.0f);
                }
                else {
                    textureData.emplace_back(0.0f, 0.0f, 1.0f, 1.0f);
                }
                xC++;
            }
            yC++;
        }
        return textureData;
    }
};

/**
 * @struct VertexData
 * @brief A structure to hold vertex data.
 *
 * This structure holds the position and texture coordinates (uv) of a vertex.
 */
struct VertexData {
    vec2 position; ///< The position of the vertex.
    vec2 uv; ///< The texture coordinates of the vertex.
};

/**
 * @class Star
 * @brief A class to represent a star with a Poincare texture.
 *
 * This class represents a star that can be drawn on the screen. The star has a Poincare texture and can be animated.
 */
class Star {
public:
    unsigned int vao{}; ///< Vertex array object.
    unsigned int vbo[2]{}; ///< Vertex buffer objects.
    VertexData data[10]; ///< Array of vertex data.
    PoincareTexture texture; ///< Poincare texture of the star.
    vec3 starCenter = vec3(50, 30, 0); ///< Center of the star.
    vec3 circleCenter = vec3(20, 30, 0.0f); ///< Center of the circle.
    float phi{}; ///< Angle for rotation.
    float selfRotation{}; ///< Angle for self rotation.

public:
    /**
     * @brief Get the Poincare texture of the star.
     *
     * @return Reference to the Poincare texture of the star.
     */
    PoincareTexture &getTexture() {
        return texture;
    }

    /**
    * @brief Constructor for the Star class.
    *
    * This constructor initializes the star with a given width and height for the Poincare texture.
    *
    * @param width Width of the Poincare texture.
    * @param height Height of the Poincare texture.
    */
    Star(int width, int height) : texture(width, height) {
        Animate(0);
        updateV();
        glGenVertexArrays(1, &vao);    // create 1 vertex array object
        glBindVertexArray(vao);        // make it active

        glGenBuffers(2, vbo);    // Generate 2 vertex buffer objects

        // vertex coordinates: vbo[0] -> Attrib Array 0 of the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData),(void *) nullptr);     // stride and offset: it is tightly packed

        // texture coordinates: vbo[1] -> Attrib Array 1 of the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData),(void *) sizeof(vec2));     // stride and offset: it is tightly packed
    }

    /**
     * @brief Update the vertex data of the star.
     */
    void updateV() {
        data[0] = {vec2(50, 30), vec2(0.5, 0.5)};
        data[1] = {vec2(70, 30), vec2(1, 0.5)};
        data[2] = {vec2(90, 70), vec2(1, 1)};
        data[3] = {vec2(50, 50), vec2(0.5, 1)};
        data[4] = {vec2(10, 70), vec2(0, 1)};
        data[5] = {vec2(30, 30), vec2(0, 0.5)};
        data[6] = {vec2(10, -10), vec2(0, 0)};
        data[7] = {vec2(50, 10), vec2(0.5, 0)};
        data[8] = {vec2(90, -10), vec2(1, 0)};
        data[9] = {vec2(70, 30), vec2(1, 0.5)};

    }

    /**
     * @brief Draw the star.
     */
    void Draw() {
        mat4 MVPTransform = M()* camera.V() * camera.P();
        gpuProgram.setUniform(MVPTransform, "MVP");
        glBindTexture(GL_TEXTURE_2D, texture.textureId);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 10);
    }

    /**
    * @brief Animate the star.
    *
    * @param t Time parameter for the animation.
    */
    void Animate(float t) {
        float rotationSpeed = 2.0f * M_PI / 10.0f;
        phi = t * rotationSpeed;
        selfRotation = t * rotationSpeed;
    }

    /**
     * @brief Get the model matrix for the star.
     *
     * @return The model matrix for the star.
     */
    mat4 M() const {
        return TranslateMatrix(-starCenter) * RotationMatrix(selfRotation, vec3(0, 0, 1)) * TranslateMatrix(starCenter) * TranslateMatrix(-circleCenter) * RotationMatrix(phi, vec3(0.0f, 0.0f, 1.0f)) * TranslateMatrix(circleCenter);
    }


    /**
     * @brief Adjust the shape of the star.
     *
     * This function adjusts the shape of the star based on a given factor.
     *
     * @param s The factor to adjust the shape of the star.
     */
    void schlankheitsfaktor(float s) {
        VertexData* it = data;
        while(it != data + 10) {
            if (it->uv.x == 0.5 && it->uv.y == 1) {
                it->position.y -= s;
            } else if (it->uv.x == 0.5 && it->uv.y == 0) {
                it->position.y += s;
            } else if (it->uv.x == 0 && it->uv.y == 0.5) {
                it->position.x += s;
            } else if (it->uv.x == 1 && it->uv.y == 0.5) {
                it->position.x -= s;
            }
            ++it;
        }
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW); // copy to that part of the memory which is modified
    }
};

Star *star;

/**
 * @brief Initializes the OpenGL viewport and creates a new Star object.
 *
 * This function sets the OpenGL viewport, creates a new Star object with a specified width and height, and creates a GPU program.
 */
void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);
    int width = 300, height = 300;
    star = new Star(width, height);
    gpuProgram.create(vertexSource, fragmentSource, "fragmentColor");
}

/**
 * @brief Handles the display event.
 *
 * This function clears the screen, draws the star, and swaps the buffers.
 */
void onDisplay() {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
    star->Draw();
    glutSwapBuffers();                                    // exchange the two buffers
}

bool isAnimating = false;
long time;

/**
 * @brief Handles the keyboard press event.
 *
 * This function handles various keyboard inputs to manipulate the star and its texture.
 *
 * @param key The ASCII code of the key pressed.
 * @param pX The x-coordinate of the mouse pointer when the key was pressed.
 * @param pY The y-coordinate of the mouse pointer when the key was pressed.
 */
void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == 'h') {
        star->schlankheitsfaktor(-10);
        glutPostRedisplay();
    } else if (key == 'H') {
        star->schlankheitsfaktor(10);
        glutPostRedisplay();
    } else if (key == 'a') {
        time = glutGet(GLUT_ELAPSED_TIME);
        isAnimating = !isAnimating;
    } else if (key == 'r') {
        star->getTexture().increaseResolution(100);
        glutPostRedisplay();
    } else if (key == 'R') {
        star->getTexture().increaseResolution(-100);
        glutPostRedisplay();}
    else if (key == 't') {
        star->getTexture().setFilteringMode(GL_NEAREST);
        glutPostRedisplay();
    } else if (key == 'T') {
        star->getTexture().setFilteringMode(GL_LINEAR);
        glutPostRedisplay();
    }

}

/**
 * @brief Handles the keyboard release event.
 *
 * @param key The ASCII code of the key released.
 * @param pX The x-coordinate of the mouse pointer when the key was released.
 * @param pY The y-coordinate of the mouse pointer when the key was released.
 */
 void onKeyboardUp(unsigned char key, int pX, int pY) {
}

bool mouseLeftPressed = false, mouseRightPressed = false;

/**
 * @brief Handles the mouse motion event.
 *
 * This function is called when the mouse is moved with a button pressed.
 *
 * @param pX The x-coordinate of the mouse pointer.
 * @param pY The y-coordinate of the mouse pointer.
 */
 void onMouseMotion(int pX, int pY) {
    if (mouseLeftPressed) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
        float cX = 2.0f * (float)pX / windowWidth - 1;    // flip y axis
        float cY = 1.0f - 2.0f * (float)pY / windowHeight;
    }
    glutPostRedisplay();
}

/**
 * @brief Handles the mouse click event.
 *
 * This function is called when a mouse button is pressed or released.
 *
 * @param button The button that was pressed or released.
 * @param state The state of the button (GLUT_DOWN or GLUT_UP).
 * @param pX The x-coordinate of the mouse pointer when the event occurred.
 * @param pY The y-coordinate of the mouse pointer when the event occurred.
 */
 void onMouse(int button, int state, int pX, int pY) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) mouseLeftPressed = true;
        else mouseLeftPressed = false;
    }
    if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) mouseRightPressed = true;
        else mouseRightPressed = false;
    }
    onMouseMotion(pX, pY);
}

/**
 * @brief Handles the idle event.
 *
 * This function is called when some time has elapsed. It is used to animate the star.
 */
 void onIdle() {
    if (isAnimating) {
        long currentTime = glutGet(GLUT_ELAPSED_TIME);
        float elapsedTime = (float) (currentTime - time) / 1000;
        star->Animate(elapsedTime);
        glutPostRedisplay();
    }
}
